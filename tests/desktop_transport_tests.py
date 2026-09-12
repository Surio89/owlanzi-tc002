# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the actual ADB wire protocol against a strict stock-style peer."""
import io
import json
from pathlib import Path
import socket
import struct
import sys
import tempfile
import threading
import unittest
from unittest.mock import Mock,patch

ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
from installer_transport import StockAdbClient,NativeDevice,NativeTransport
from installer_desktop import DesktopInstaller
from installer_server import Installer

class StockPeer:
    payload=bytes(range(256))*35
    def __init__(self):
        self.listener=socket.socket();self.listener.bind(('127.0.0.1',0));self.listener.listen(1)
        self.port=self.listener.getsockname()[1];self.errors=[];self.quits=0;self.upload=b''
        self.thread=threading.Thread(target=self.run,daemon=True);self.thread.start()
    def receive(self):
        def read(size):
            result=b''
            while len(result)<size:
                chunk=self.connection.recv(size-len(result))
                if not chunk:raise EOFError()
                result+=chunk
            return result
        command,local,remote,size,checksum,magic=struct.unpack('<6I',read(24));data=read(size)
        assert magic==command^0xffffffff and sum(data)&0xffffffff==checksum
        return struct.pack('<I',command),local,remote,data
    def send(self,command,local,remote,data=b''):
        number=struct.unpack('<I',command)[0]
        self.connection.sendall(struct.pack('<6I',number,local,remote,len(data),sum(data)&0xffffffff,number^0xffffffff)+data)
    def run(self):
        try:
            self.connection,_=self.listener.accept();self.connection.settimeout(5)
            assert self.receive()[0]==b'CNXN'
            self.send(b'CNXN',0x1000000,4096,b'device::tc002\0')
            while True:
                try:command,local,_,destination=self.receive()
                except EOFError:break
                assert command==b'OPEN';remote=local+100;self.send(b'OKAY',remote,local)
                if destination.startswith(b'shell:'):
                    self.send(b'WRTE',remote,local,b'fixture\n');assert self.receive()[0]==b'OKAY'
                    self.send(b'CLSE',remote,local);assert self.receive()[0]==b'CLSE';continue
                assert destination==b'sync:\0'
                buffer=b''
                while True:
                    command,_,_,data=self.receive()
                    if command==b'OKAY':continue
                    # Abrupt CLSE is precisely the old client's failing behavior.
                    assert command==b'WRTE','sync must receive QUIT before CLSE'
                    self.send(b'OKAY',remote,local);buffer+=data
                    quit_seen=False
                    while len(buffer)>=8:
                        kind=buffer[:4];size=struct.unpack('<I',buffer[4:8])[0]
                        length=0 if kind==b'DONE' else size
                        if len(buffer)<8+length:break
                        body=buffer[8:8+length];buffer=buffer[8+length:]
                        if kind==b'RECV':
                            wire=b'DATA'+struct.pack('<I',len(self.payload))+self.payload+b'DONE'+bytes(4)
                            # Fragment within headers and data, like a real TCP stream.
                            for start in range(0,len(wire),997):
                                self.send(b'WRTE',remote,local,wire[start:start+997]);assert self.receive()[0]==b'OKAY'
                        elif kind==b'SEND':self.upload=b''
                        elif kind==b'DATA':self.upload+=body
                        elif kind==b'DONE':
                            self.send(b'WRTE',remote,local,b'OKAY'+bytes(4));assert self.receive()[0]==b'OKAY'
                        elif kind==b'QUIT':
                            assert size==0;self.quits+=1;self.send(b'CLSE',remote,local)
                            assert self.receive()[0]==b'CLSE';quit_seen=True;break
                        else:raise AssertionError('unexpected sync request')
                    if quit_seen:break
        except Exception as error:self.errors.append(error)
        finally:
            if hasattr(self,'connection'):self.connection.close()
            self.listener.close()
    def finish(self):
        self.thread.join(6)
        if self.thread.is_alive():raise AssertionError('ADB session did not finish')
        if self.errors:raise self.errors[0]

class TransportTests(unittest.TestCase):
    def test_pull_push_quit_and_following_shell_share_healthy_session(self):
        peer=StockPeer();client=StockAdbClient('127.0.0.1',peer.port,default_transport_timeout_s=2,banner=b'test')
        try:
            self.assertTrue(client.connect(read_timeout_s=2))
            for _ in range(2):
                downloaded=io.BytesIO();client.pull('/fixture',downloaded,read_timeout_s=2)
                self.assertEqual(downloaded.getvalue(),peer.payload)
                self.assertEqual(client.shell('echo fixture',read_timeout_s=2),'fixture\n')
            client.push(io.BytesIO(peer.payload),'/fixture',read_timeout_s=2)
            self.assertEqual(client.shell('echo fixture',read_timeout_s=2),'fixture\n')
        finally:client.close();peer.finish()
        self.assertEqual(peer.quits,3);self.assertEqual(peer.upload,peer.payload)
    def test_operation_retains_connection_and_does_not_retry_failed_write(self):
        with tempfile.TemporaryDirectory() as work,patch('installer_transport.StockAdbClient') as factory:
            client=factory.return_value;client.connect.return_value=True;client.shell.return_value='ok'
            transport=NativeTransport(work);device=transport.device('192.168.1.2')
            self.assertTrue(device.reconnect());device.command('shell','first');device.command('shell','second')
            self.assertIs(transport.device('192.168.1.2'),device);factory.assert_called_once();client.close.assert_not_called()
            client.push.side_effect=OSError('private fixture text')
            with self.assertRaisesRegex(RuntimeError,'Clock transfer failed'):device.command('push','local','remote')
            client.push.assert_called_once();client.close.assert_called_once();transport.close()
    def test_failure_records_only_fixed_step_and_releases_transport(self):
        with tempfile.TemporaryDirectory() as work:
            root=Path(work);(root/'package.json').write_text(json.dumps({'schema':1,'target':'tc002','version':'0.3.0','files':{}}))
            installer=DesktopInstaller(root,root/'work');transport=Mock();installer.tools={'adb':transport}
            def fail(ip):installer.report('backup');raise OSError('private password and device response')
            with patch.object(installer,'prepare',side_effect=fail):installer.perform('prepare',{'ip':'192.168.1.2'})
            state=installer.snapshot();self.assertEqual(state['phase'],'error');self.assertEqual(state['detail']['code'],'TC002-BACKUP')
            self.assertNotIn('private',json.dumps(state));self.assertNotIn('private',(root/'work/last-error.json').read_text())
            transport.close.assert_called_once()

if __name__=='__main__':unittest.main()
