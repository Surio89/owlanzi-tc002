# SPDX-License-Identifier: GPL-3.0-or-later
"""Acquire pinned manufacturer tools locally; never install the IDE."""
import hashlib
from pathlib import Path
import stat
import subprocess
import urllib.request
import zipfile

IDE_URL='https://download.s21i.co99.net/14731609/0/0/ABUIABBPGAAglMLczgYo0Mjk3AU.zip?f=flythings-ide-win32-win32-x86-zkswe-setup.zip&v=1775706403'
IDE_SHA='e85c2f3cf92fca95907315da78d82cb5cacbb8c85a7f703624103b0ce8ef4753'
INNO_URL='https://constexpr.org/innoextract/files/innoextract-1.9/innoextract-1.9-windows.zip'
INNO_SHA='6989342c9b026a00a72a38f23b62a8e6a22cc5de69805cf47d68ac2fec993065'
PLUGIN='app/bin/plugins/com.zkswe.ide.editor_2.0.0.202604031130.jar'
ADB='app/sdk/platform-tools/adb/'
PACKERS=('zkswe_mkimg.exe','fsimg.exe','cygwin1.dll','cyggcc_s-1.dll','cyglz4-1.dll','cyglzma-5.dll','cyglzo2-2.dll','cygz.dll')

def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def download(url,destination,expected,report):
    destination=Path(destination);destination.parent.mkdir(parents=True,exist_ok=True)
    if destination.is_file() and digest(destination)==expected:return destination
    partial=destination.with_suffix('.partial')
    with urllib.request.urlopen(url,timeout=30) as source,partial.open('wb') as target:
        if not source.url.startswith('https://'):raise ValueError('Download must retain HTTPS')
        count=0;next_notice=0
        while chunk:=source.read(1024*1024):
            count+=len(chunk)
            if count>250*1024*1024:raise ValueError('Unexpected download size')
            target.write(chunk)
            if count>=next_notice:report('download',{'megabytes':count//1048576});next_notice=count+10*1048576
    if digest(partial)!=expected:raise ValueError('Tool download checksum mismatch; please retry later')
    partial.replace(destination);return destination

def extract(archive,destination,names=None):
    destination=Path(destination).resolve();destination.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(archive) as source:
        total=0
        for info in source.infolist():
            if names is not None and info.filename not in names:continue
            total+=info.file_size
            path=destination/info.filename
            if total>700*1024*1024 or '\\' in info.filename or ':' in info.filename or not path.resolve().is_relative_to(destination) or stat.S_ISLNK(info.external_attr>>16):
                raise ValueError('Unexpected archive path or size')
            if info.is_dir():path.mkdir(parents=True,exist_ok=True)
            else:path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(source.read(info))

def prepare(directory,report=lambda *args:None):
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=True)
    ide=download(IDE_URL,directory/'flythings-ide.zip',IDE_SHA,report)
    inno=download(INNO_URL,directory/'innoextract.zip',INNO_SHA,report)
    # Verify the complete pinned archives on every run before executing tools.
    extract(inno,directory/'innoextract')
    setup_name='flythings-ide-win32-win32-x86-zkswe-setup.exe'
    extract(ide,directory/'ide',{setup_name})
    selected=[PLUGIN]+[ADB+name for name in ('adb.exe','AdbWinApi.dll','AdbWinUsbApi.dll')]
    args=[str((directory/'innoextract/innoextract.exe').resolve()),'--silent','--extract','--output-dir',str((directory/'vendor').resolve())]
    for name in selected:args+=['--include',str(Path(name))]
    args+=[str((directory/'ide'/setup_name).resolve())]
    subprocess.run(args,capture_output=True,check=True,timeout=180)
    with zipfile.ZipFile(directory/'vendor'/PLUGIN) as archive:
        packer_dir=directory/'packers';packer_dir.mkdir(exist_ok=True)
        for name in PACKERS:(packer_dir/name).write_bytes(archive.read('bundle/bin/'+name))
    adb=directory/'vendor'/ADB/'adb.exe'
    if not adb.is_file():raise ValueError('Manufacturer ADB tool missing')
    return {'adb':adb.resolve(),'packers':packer_dir.resolve()}
