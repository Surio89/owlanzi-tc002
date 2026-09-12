# SPDX-License-Identifier: GPL-3.0-or-later
"""Build on each native OS, using only the immutable accepted TC002 payload."""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile

ROOT=Path(__file__).resolve().parents[1]
VERSION='0.1.0'
APP_VERSION='0.3.0'
SOURCE_URL='https://owlanzi.com/downloads/owlanzi-tc002-0.3.0-windows.zip?no_stats=1'
SOURCE_SHA='23d744c889bbe9a55134d53f927643162ad4ee2edd3bf36f5703d574296e3231'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def prepare_payload(output,source=None,mksquashfs=None):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Use a new package work directory')
    output.mkdir(parents=True)
    source=Path(source) if source else output/'accepted-installer.zip'
    if not source.is_file():
        req=urllib.request.Request(SOURCE_URL,headers={'DNT':'1'})
        with urllib.request.urlopen(req,timeout=60) as response:source.write_bytes(response.read(20*1024*1024))
    if sha(source)!=SOURCE_SHA:raise ValueError('Accepted app package checksum mismatch')
    payload=output/'payload';payload.mkdir()
    with zipfile.ZipFile(source) as archive:
        acceptance=json.loads(archive.read('package.json'))
        if acceptance.get('version')!=APP_VERSION or not acceptance.get('hardware_verified'):raise ValueError('Hardware acceptance missing')
        selected={name:digest for name,digest in acceptance['files'].items() if name.startswith(('app/','boot/'))}
        if len(selected)!=9:raise ValueError('Unexpected accepted app/boot files')
        for name,digest in selected.items():
            target=payload/name
            if not target.resolve().is_relative_to(payload):raise ValueError('Invalid payload path')
            data=archive.read(name)
            if hashlib.sha256(data).hexdigest()!=digest:raise ValueError('Accepted payload changed')
            target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    files={path.relative_to(payload).as_posix():sha(path) for path in payload.rglob('*') if path.is_file()}
    (payload/'package.json').write_text(json.dumps({'schema':1,'target':'tc002','version':APP_VERSION,'installer_version':VERSION,'files':files},indent=2)+'\n')
    return payload

def build(output,source=None,mksquashfs=None):
    output=Path(output).resolve();payload=prepare_payload(output,source,mksquashfs)
    command=[sys.executable,'-m','PyInstaller','--noconfirm','--clean','--onedir','--windowed','--name','Owlanzi Installer',
             '--workpath',str(output/'work'),'--distpath',str(output/'dist'),'--specpath',str(output),
             '--paths',str(ROOT/'scripts'),'--add-data',str(payload)+':payload',
             '--add-data',str(ROOT/'scripts/local-device.py')+':.',
             '--add-data',str(ROOT/'scripts/permanent-image.py')+':.',
             '--hidden-import','installer_image','--hidden-import','installer_elf','--hidden-import','installer_container']
    if mksquashfs:
        # Let PyInstaller collect native runtime dependencies beside the binary.
        command+=['--add-binary',str(Path(mksquashfs).resolve())+':tools']
    if sys.platform=='darwin':command+=['--osx-bundle-identifier','com.owlanzi.installer']
    command+=[str(ROOT/'desktop/main.py')]
    subprocess.run(command,check=True,cwd=ROOT)
    os_name={'win32':'windows','darwin':'macos'}.get(sys.platform,'linux')
    arch={'AMD64':'x64','x86_64':'x64','aarch64':'arm64','arm64':'arm64'}.get(platform.machine(),platform.machine())
    label=f'owlanzi-installer-{VERSION}-{os_name}-{arch}'
    bundle=output/'dist'/('Owlanzi Installer.app' if sys.platform=='darwin' else 'Owlanzi Installer')
    release=output/'release';release.mkdir()
    executable=bundle/'Contents/MacOS/Owlanzi Installer' if sys.platform=='darwin' else bundle/('Owlanzi Installer.exe' if sys.platform=='win32' else 'Owlanzi Installer')
    smoke=release/(label+'-smoke.json')
    subprocess.run([str(executable),'--self-test',str(smoke)],check=True,timeout=45)
    proof=json.loads(smoke.read_text())
    if not proof.get('payload_verified') or not proof.get('native_gui_created'):raise ValueError('Packaged app did not pass its startup test')
    for name in ('LICENSE','THIRD_PARTY_NOTICES.md'):shutil.copy2(ROOT/name,output/'dist'/name)
    if os_name=='linux':
        archive=release/(label+'.tar.gz')
        with tarfile.open(archive,'w:gz') as packed:
            packed.add(bundle,arcname=bundle.name)
            for name in ('LICENSE','THIRD_PARTY_NOTICES.md'):packed.add(ROOT/name,arcname=name)
    elif os_name=='macos':
        archive=release/(label+'.zip')
        subprocess.run(['ditto','-c','-k','--sequesterRsrc','--keepParent',str(bundle),str(archive)],check=True)
    else:
        archive=release/(label+'.zip')
        with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as packed:
            for file in bundle.rglob('*'):
                if file.is_file():packed.write(file,file.relative_to(bundle.parent))
            for name in ('LICENSE','THIRD_PARTY_NOTICES.md'):packed.write(ROOT/name,name)
    receipt={'schema':1,'target':'tc002','installer_version':VERSION,'app_version':APP_VERSION,'os':os_name,'arch':arch,
             'file':archive.name,'size_bytes':archive.stat().st_size,'sha256':sha(archive),'source_payload_sha256':SOURCE_SHA,
             'first_install_hardware_verified':False,'codesigned':False}
    (release/(label+'.json')).write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt));return receipt

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True,type=Path);parser.add_argument('--source',type=Path);parser.add_argument('--mksquashfs',type=Path);parser.add_argument('--prepare-only',action='store_true');args=parser.parse_args()
    if sys.platform!='win32' and not args.mksquashfs:parser.error('Packaged macOS/Linux apps need --mksquashfs')
    (prepare_payload if args.prepare_only else build)(args.output,args.source,args.mksquashfs)
