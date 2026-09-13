# SPDX-License-Identifier: GPL-3.0-or-later
"""Build native installers with accepted boot components and a pinned app release."""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import sys
import tarfile
import urllib.request
import zipfile
from desktop_licenses import collect as collect_licenses

ROOT=Path(__file__).resolve().parents[1]
VERSION='0.1.6'
APP_VERSION='0.3.2'
BOOT_VERSION='0.3.0'
SOURCE_URL='https://owlanzi.com/downloads/owlanzi-tc002-0.3.0-windows.zip?no_stats=1'
SOURCE_SHA='23d744c889bbe9a55134d53f927643162ad4ee2edd3bf36f5703d574296e3231'
APP_SOURCE_URL='https://owlanzi.com/firmware/owlanzi-tc002-0.3.2-ota.bin?no_stats=1'
APP_SOURCE_SHA='bb1fe12cf416c509368d414150ff72db641c16cd7b8a6571a99fe88d2e67f66c'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def apply_app_update(payload,data,manifest,expected_sha):
    if hashlib.sha256(data).hexdigest()!=expected_sha or data[:8]!=b'OWLTC002' or len(data)<12:raise ValueError('App update checksum or format mismatch')
    size=struct.unpack('<I',data[8:12])[0]
    if not 1<=size<=65536 or 12+size>len(data):raise ValueError('Invalid app header')
    metadata=json.loads(data[12:12+size]);offset=12+size
    names=('lib/libzkgui.so','ui/main.ftu','ui/cacert.pem')
    if (metadata.get('schema'),metadata.get('target'),metadata.get('abi'),metadata.get('version'))!=(1,'tc002','z21-stock-1',APP_VERSION):raise ValueError('Unexpected app release')
    if [entry['path'] for entry in metadata['files']]!=list(names) or manifest.get('version')!=APP_VERSION:raise ValueError('Unexpected app files')
    if set(manifest['files'])!=set(names)|{'EasyUI.cfg'}:raise ValueError('Unexpected device manifest')
    cfg=(payload/'app/EasyUI.cfg').read_bytes()
    if manifest['files']['EasyUI.cfg']!={'sha256':hashlib.sha256(cfg).hexdigest(),'size':len(cfg)}:raise ValueError('App changes the startup configuration')
    updates={}
    for entry in metadata['files']:
        length=entry['size']
        if not isinstance(length,int) or length<=0 or offset+length>len(data):raise ValueError('Invalid app file size')
        content=data[offset:offset+length];offset+=length
        checked={'size':length,'sha256':hashlib.sha256(content).hexdigest()}
        if checked!={'size':length,'sha256':entry['sha256']} or checked!=manifest['files'][entry['path']]:raise ValueError('App manifest mismatch')
        updates[entry['path']]=content
    if offset!=len(data):raise ValueError('Unexpected app update trailer')
    # Complete validation precedes writes; boot files and EasyUI.cfg stay exact.
    for name,content in updates.items():(payload/'app'/name).write_bytes(content)
    (payload/'app/manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

def prepare_payload(output,source=None,mksquashfs=None,app_source=None):
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
        if acceptance.get('version')!=BOOT_VERSION or not acceptance.get('hardware_verified'):raise ValueError('Boot hardware acceptance missing')
        selected={name:digest for name,digest in acceptance['files'].items() if name.startswith(('app/','boot/'))}
        if len(selected)!=9:raise ValueError('Unexpected accepted app/boot files')
        for name,digest in selected.items():
            target=payload/name
            if not target.resolve().is_relative_to(payload):raise ValueError('Invalid payload path')
            data=archive.read(name)
            if hashlib.sha256(data).hexdigest()!=digest:raise ValueError('Accepted payload changed')
            target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    if app_source:data=Path(app_source).read_bytes()
    else:
        req=urllib.request.Request(APP_SOURCE_URL,headers={'DNT':'1'})
        with urllib.request.urlopen(req,timeout=60) as response:data=response.read(4194305)
    if len(data)>4194304:raise ValueError('App update exceeds device limit')
    apply_app_update(payload,data,json.loads((ROOT/'desktop/app-manifest.json').read_text()),APP_SOURCE_SHA)
    from installer_device import device
    device.verified_bundle(payload/'app')
    files={path.relative_to(payload).as_posix():sha(path) for path in payload.rglob('*') if path.is_file()}
    (payload/'package.json').write_text(json.dumps({'schema':1,'target':'tc002','version':APP_VERSION,'installer_version':VERSION,'files':files},indent=2)+'\n')
    return payload

def build(output,source=None,mksquashfs=None,app_source=None):
    output=Path(output).resolve();payload=prepare_payload(output,source,mksquashfs,app_source)
    licenses=collect_licenses(ROOT,output/'licenses',mksquashfs)
    command=[sys.executable,'-m','PyInstaller','--noconfirm','--clean','--onedir','--windowed','--name','Owlanzi Installer',
             '--workpath',str(output/'work'),'--distpath',str(output/'dist'),'--specpath',str(output),
             '--paths',str(ROOT/'scripts'),'--add-data',str(payload)+':payload',
             '--add-data',str(licenses)+':licenses',
             '--add-data',str(ROOT/'desktop/check.svg')+':.',
             '--add-data',str(ROOT/'scripts/local-device.py')+':.',
             '--add-data',str(ROOT/'scripts/permanent-image.py')+':.',
             '--hidden-import','installer_image','--hidden-import','installer_elf','--hidden-import','installer_container']
    if mksquashfs:
        # Let PyInstaller collect native runtime dependencies beside the binary.
        command+=['--add-binary',str(Path(mksquashfs).resolve())+':tools']
    if sys.platform=='darwin':command+=['--osx-bundle-identifier','com.owlanzi.installer']
    command+=[str(ROOT/'desktop/main.py')]
    subprocess.run(command,check=True,cwd=ROOT)
    if sys.platform=='darwin':
        # Rebuild from the generated spec with local-network usage text in the
        # bundle before PyInstaller performs its final ad-hoc signature.
        spec=output/'Owlanzi Installer.spec'
        data=spec.read_text()
        data=data.replace("bundle_identifier='com.owlanzi.installer',", "bundle_identifier='com.owlanzi.installer',\n    info_plist={'NSLocalNetworkUsageDescription': 'Owlanzi finds and sets up your TC002 clock on your home network.', 'CFBundleShortVersionString': '"+VERSION+"'},")
        if 'NSLocalNetworkUsageDescription' not in data:raise ValueError('Missing network privacy declaration')
        spec.write_text(data)
        subprocess.run([sys.executable,'-m','PyInstaller','--noconfirm','--workpath',str(output/'work'),'--distpath',str(output/'dist'),str(spec)],check=True,cwd=ROOT)
    os_name={'win32':'windows','darwin':'macos'}.get(sys.platform,'linux')
    arch={'AMD64':'x64','x86_64':'x64','aarch64':'arm64','arm64':'arm64'}.get(platform.machine(),platform.machine())
    label=f'owlanzi-installer-{VERSION}-{os_name}-{arch}'
    bundle=output/'dist'/('Owlanzi Installer.app' if sys.platform=='darwin' else 'Owlanzi Installer')
    release=output/'release';release.mkdir()
    executable=bundle/'Contents/MacOS/Owlanzi Installer' if sys.platform=='darwin' else bundle/('Owlanzi Installer.exe' if sys.platform=='win32' else 'Owlanzi Installer')
    smoke=release/(label+'-smoke.json')
    subprocess.run([str(executable),'--self-test',str(smoke),'--image-fixture',str(ROOT/'tests/fixtures/root-mode.squashfs')],check=True,timeout=600)
    proof=json.loads(smoke.read_text())
    if not proof.get('payload_verified') or not proof.get('native_gui_created'):raise ValueError('Packaged app did not pass its startup test')
    if not proof.get('image_build_verified') or not proof.get('root_mode_preserved'):raise ValueError('Packaged app did not preserve the filesystem fixture')
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
             'file':archive.name,'size_bytes':archive.stat().st_size,'sha256':sha(archive),'source_payload_sha256':SOURCE_SHA,'app_payload_sha256':APP_SOURCE_SHA,
             'first_install_hardware_verified':False,'codesigned':False}
    (release/(label+'.json')).write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt));return receipt

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True,type=Path);parser.add_argument('--source',type=Path);parser.add_argument('--app-source',type=Path);parser.add_argument('--mksquashfs',type=Path);parser.add_argument('--prepare-only',action='store_true');args=parser.parse_args()
    if sys.platform!='win32' and not args.mksquashfs:parser.error('Packaged macOS/Linux apps need --mksquashfs')
    (prepare_payload if args.prepare_only else build)(args.output,args.source,args.mksquashfs,args.app_source)
