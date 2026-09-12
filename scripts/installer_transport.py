# SPDX-License-Identifier: GPL-3.0-or-later
"""Native TCP ADB; no installed SDK, daemon, executable or shell on the host."""
from pathlib import Path
import subprocess
import struct
from adb_shell.adb_device import AdbDeviceTcp
from adb_shell.adb_message import AdbMessage
from adb_shell import constants
from installer_device import device as legacy

class StockAdbClient(AdbDeviceTcp):
    """Close FileSync with QUIT, as the native ADB client does.

    The TC002's stock adbd resets its TCP service when sync is closed abruptly.
    adb-shell 0.4.4 sends CLSE directly, which breaks the next operation (or
    times out on this one). Await the service's orderly close before replying.
    https://android.googlesource.com/platform/system/core/+/fdebc26ad5c61dd9198960ecda65fddcadc07c80/adb/file_sync_client.cpp
    """
    def __init__(self,*args,**kwargs):
        super().__init__(*args,**kwargs);self._sync_ids=set()
    def _open(self,destination,*args):
        info=super()._open(destination,*args)
        if destination==b'sync:':self._sync_ids.add(info.local_id)
        return info
    def _clse(self,info):
        if info.local_id not in self._sync_ids:return super()._clse(info)
        self._sync_ids.discard(info.local_id)
        self._io_manager.send(AdbMessage(constants.WRTE,info.local_id,info.remote_id,b'QUIT'+struct.pack('<I',0)),info)
        self._read_until([constants.CLSE],info)
        self._io_manager.send(AdbMessage(constants.CLSE,info.local_id,info.remote_id),info)

class NativeDevice(legacy.Device):
    def __init__(self,ip,workspace):
        super().__init__(ip,'native-tcp');self.work_root=Path(workspace);self._client=None
    def close(self):
        if self._client:self._client.close();self._client=None
    def client(self):
        if self._client is not None:return self._client
        client=StockAdbClient(self.ip,5555,default_transport_timeout_s=5,banner=b'owlanzi-installer')
        try:
            if not client.connect(auth_timeout_s=2,read_timeout_s=5):raise RuntimeError('Clock connection unavailable')
        except Exception:
            client.close();raise RuntimeError('Clock connection unavailable') from None
        self._client=client;return client
    def reconnect(self):
        try:self.client();return True
        except Exception:return False
    def command(self,*args,check=True):
        client=self.client()
        try:
            if len(args)==2 and args[0]=='shell':
                output=client.shell(args[1],timeout_s=45,read_timeout_s=10)
            elif len(args)==3 and args[0]=='pull':
                client.pull(args[1],args[2],read_timeout_s=45);output=''
            elif len(args)==3 and args[0]=='push':
                client.push(args[1],args[2],read_timeout_s=45);output=''
            else:raise ValueError('Unsupported installer operation')
            return subprocess.CompletedProcess(args,0,output,'')
        except Exception:
            self.close()
            if check:raise RuntimeError('Clock transfer failed; check its connection') from None
            return subprocess.CompletedProcess(args,1,'','Clock transfer failed')

class NativeTransport:
    def __init__(self,workspace):self.workspace=Path(workspace);self.devices={}
    def device(self,ip):
        if ip not in self.devices:self.devices[ip]=NativeDevice(ip,self.workspace)
        return self.devices[ip]
    def close(self):
        for device in self.devices.values():device.close()
        self.devices.clear()
