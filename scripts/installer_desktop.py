# SPDX-License-Identifier: GPL-3.0-or-later
"""Shared desktop controller; no HTTP server is started by the native app."""
import os
from pathlib import Path
import shutil
import sys
from installer_server import Installer
from installer_transport import NativeTransport
import installer_tools

VERSION='1.0.0'

def workspace():
    if sys.platform=='win32':root=Path(os.environ.get('LOCALAPPDATA',Path.home()/'AppData/Local'))
    elif sys.platform=='darwin':root=Path.home()/'Library/Application Support'
    else:root=Path(os.environ.get('XDG_DATA_HOME',Path.home()/'.local/share'))
    target=root/'Owlanzi/Installer';target.mkdir(parents=True,exist_ok=True,mode=0o700)
    return target

class DesktopInstaller(Installer):
    def prepare_tools(self):
        native=self.root.parent/'tools'/('mksquashfs.exe' if sys.platform=='win32' else 'mksquashfs')
        if native.is_file():packers={'mksquashfs':native}
        elif sys.platform=='win32':
            packers=installer_tools.prepare(self.workspace/'tools',self.report)['packers']
        else:
            # Development source runs may use their local tool. Packaged apps
            # must carry the verified tool and never depend on Homebrew/apt.
            path=None if getattr(sys,'frozen',False) else shutil.which('mksquashfs')
            if not path:raise ValueError('The desktop package is missing its filesystem tool')
            packers={'mksquashfs':Path(path)}
        self.tools={'adb':NativeTransport(self.workspace),'packers':packers}
        with self.lock:self.state.update(phase='tools_ready',busy=False,message='tools_ready',detail={})
    def perform(self,action,body):
        if action!='tools':
            try:return super().perform(action,body)
            finally:
                if self.tools and hasattr(self.tools['adb'],'close'):self.tools['adb'].close()
        try:self.prepare_tools()
        except Exception as error:self.failure(action,error)
