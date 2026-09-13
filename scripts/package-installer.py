# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the portable Windows helper; no device operations or publication."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

ROOT=Path(__file__).resolve().parents[1]
PYTHON_SHA='d1f04d990aee1253d8569e8e5104e30fa9f5fa830899f14843448872d936a2cf'
SCRIPTS=('installer_server.py','installer_device.py','installer_tools.py','installer_elf.py','installer_image.py','local-device.py','permanent-image.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def package(destination):
    destination=Path(destination).resolve()
    if destination.exists():raise ValueError('Use a new output directory')
    manifest=json.loads((ROOT/'build/tc002/device/manifest.json').read_text())
    version=manifest['version'];destination.mkdir(parents=True)
    runtime_archive=ROOT/'.cache/python-3.13.15-embed-amd64.zip'
    if sha(runtime_archive)!=PYTHON_SHA:raise ValueError('Python runtime checksum mismatch')
    with zipfile.ZipFile(runtime_archive) as archive:archive.extractall(destination/'runtime')
    (destination/'runtime/python313._pth').write_text('python313.zip\n.\n../scripts\n../python-libs\n',encoding='ascii')
    for folder in ('scripts','boot','python-libs'): (destination/folder).mkdir()
    for name in SCRIPTS:shutil.copyfile(ROOT/'scripts'/name,destination/'scripts'/name)
    shutil.copytree(ROOT/'installer',destination/'installer')
    shutil.copytree(ROOT/'build/tc002/device',destination/'app')
    for name in ('ota-switch','owlanzi-boot-control','libowlanzi-boot.so','owlanzi-install-guard'):shutil.copyfile(ROOT/'build/tc002/platform/tc002'/name,destination/'boot'/name)
    for source in (ROOT/'.cache/installer-python/elftools',ROOT/'.cache/installer-python/pyelftools-0.33.dist-info',ROOT/'.cache/image-reader/PySquashfsImage',ROOT/'.cache/image-reader/PySquashfsImage-0.9.0.dist-info'):
        shutil.copytree(source,destination/'python-libs'/source.name,ignore=shutil.ignore_patterns('__pycache__','*.pyc','RECORD','INSTALLER','REQUESTED'))
    for name in ('LICENSE','THIRD_PARTY_NOTICES.md'):shutil.copyfile(ROOT/name,destination/name)
    (destination/'START-OWLANZI.cmd').write_text('@echo off\r\ncd /d "%~dp0"\r\n"%~dp0runtime\\python.exe" "%~dp0scripts\\installer_server.py" --root "%~dp0."\r\nif errorlevel 1 pause\r\n',encoding='ascii',newline='')
    (destination/'README.txt').write_text('Owlanzi TC002 '+version+'\n\nDeutsch: ZIP vollstaendig entpacken. START-OWLANZI.cmd doppelklicken.\nEnglish: Extract the complete ZIP. Double-click START-OWLANZI.cmd.\n\nGuide: https://owlanzi.com/setup-tc002.html\nDeutsch: https://owlanzi.com/setup-tc002.de.html\n\nThe helper runs on this computer. It contacts only the clock address you enter.\nManufacturer tools are downloaded separately after your confirmation.\nKeep the backups folder private; it can contain your clock settings.\n\nPython 3.13.15: https://www.python.org/downloads/release/python-31315/\nPython licence: runtime/LICENSE.txt\npyelftools 0.33: public domain, see python-libs/pyelftools-0.33.dist-info/licenses/LICENSE\nPySquashfsImage 0.9.0: LGPL 2.1, replaceable Python source and licence in python-libs.\nhttps://github.com/matteomattei/PySquashfsImage\n',encoding='utf-8')
    files={p.relative_to(destination).as_posix():sha(p) for p in destination.rglob('*') if p.is_file()}
    info={'schema':1,'target':'tc002','version':version,'files':files,'hardware_verified':False}
    (destination/'package.json').write_text(json.dumps(info,indent=2)+'\n',encoding='utf-8')
    archive_path=destination.parent/('owlanzi-tc002-'+version+'-windows.zip')
    with zipfile.ZipFile(archive_path,'w',zipfile.ZIP_DEFLATED) as archive:
        for path in destination.rglob('*'):
            if path.is_file():archive.write(path,path.relative_to(destination))
    receipt={'schema':1,'target':'tc002','version':version,'file':archive_path.name,'size_bytes':archive_path.stat().st_size,'sha256':sha(archive_path),'hardware_verified':False}
    (destination.parent/'tc002-windows.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt));return receipt
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();package(a.output)
