#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Create an app-only TC002 OTA package. Never includes device settings or SDK libraries."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import zipfile

ROOT=Path(__file__).resolve().parents[1]
NAMES=('lib/libzkgui.so','ui/main.ftu','ui/cacert.pem')

def sha(data): return hashlib.sha256(data).hexdigest()

def package(bundle, destination, notes):
    spec=importlib.util.spec_from_file_location('local_device',ROOT/'scripts/local-device.py')
    device=importlib.util.module_from_spec(spec);spec.loader.exec_module(device)
    bundle,debug=device.verified_bundle(bundle)
    version=debug['version']
    if not re.fullmatch(r'(?:0|[1-9][0-9]{0,4})\.(?:0|[1-9][0-9]{0,4})\.(?:0|[1-9][0-9]{0,4})',version): raise ValueError('Invalid release version')
    files=[(name,(bundle/name).read_bytes()) for name in NAMES]
    meta={'schema':1,'target':'tc002','abi':'z21-stock-1','version':version,'files':[{'path':name,'size':len(data),'sha256':sha(data)} for name,data in files]}
    header=json.dumps(meta,separators=(',',':')).encode()
    payload=b'OWLTC002'+struct.pack('<I',len(header))+header+b''.join(data for _,data in files)
    if not 1024<=len(payload)<=4194304 or len(notes.encode())>2000: raise ValueError('Release exceeds supported limits')
    name=f'owlanzi-tc002-{version}-ota.bin'
    release={'schema':1,'target':'tc002','kind':'tc002-app-bundle','abi':'z21-stock-1','loader':1,'version':version,'file':name,'size_bytes':len(payload),'sha256':sha(payload),'notes':notes,'source':f'owlanzi-tc002-{version}-source.zip','persistent_boot':False}
    destination.mkdir(parents=True,exist_ok=True)
    (destination/name).write_bytes(payload)
    (destination/'ota-tc002.json').write_text(json.dumps(release,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    (destination/'slot-manifest.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    # Explicit source roots, including new source files not yet committed. No
    # .local, .cache, data, generated build products, credentials or attachments.
    source_files=[ROOT/name for name in ('CMakeLists.txt','LICENSE','README.md','THIRD_PARTY_NOTICES.md')]
    for folder in ('cmake','include','src','platform','scripts','tests','web','installer','vendor','docs'):
        source_files.extend(p for p in (ROOT/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts and p.suffix not in ('.pyc','.exe','.so','.a','.zip'))
    with zipfile.ZipFile(destination/release['source'],'w',zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(set(source_files)):
            if path.is_symlink() or not path.resolve().is_relative_to(ROOT): raise ValueError('Unexpected source link')
            relative=path.relative_to(ROOT).as_posix()
            if re.search(r'secret|credential|\.local|\.cache',relative,re.I): raise ValueError('Private path in source archive')
            info=zipfile.ZipInfo(f'owlanzi-tc002-{version}/'+relative,date_time=(2026,9,9,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o644<<16
            archive.writestr(info,path.read_bytes())
    print(json.dumps({'version':version,'size':len(payload),'sha256':sha(payload),'source_files':len(set(source_files))}))
    return release

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bundle',type=Path,default=ROOT/'build/tc002/device');p.add_argument('--output',type=Path,default=ROOT/'dist/ota');p.add_argument('--notes',default='TC002: OTA updates with daily update checks and preserved settings. Wi-Fi and Owlet setup, plus time zone support.');a=p.parse_args();package(a.bundle,a.output,a.notes)
