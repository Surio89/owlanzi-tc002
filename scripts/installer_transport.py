# SPDX-License-Identifier: GPL-3.0-or-later
"""Native TCP ADB; no installed SDK, daemon, executable or shell on the host."""
from pathlib import Path
import subprocess
from adb_shell.adb_device import AdbDeviceTcp
from installer_device import device as legacy

class NativeDevice(legacy.Device):
    def __init__(self,ip,workspace):
        super().__init__(ip,'native-tcp');self.work_root=Path(workspace)
    def client(self):
        client=AdbDeviceTcp(self.ip,5555,default_transport_timeout_s=5,banner=b'owlanzi-installer')
        try:
            if not client.connect(auth_timeout_s=2,read_timeout_s=5):raise RuntimeError('Clock connection unavailable')
        except Exception:
            client.close();raise RuntimeError('Clock connection unavailable') from None
        return client
    def reconnect(self):
        try:self.client().close();return True
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
            if check:raise RuntimeError('Clock transfer failed; check its connection') from None
            return subprocess.CompletedProcess(args,1,'','Clock transfer failed')
        finally:client.close()

class NativeTransport:
    def __init__(self,workspace):self.workspace=Path(workspace)
    def device(self,ip):return NativeDevice(ip,self.workspace)
