# SPDX-License-Identifier: GPL-3.0-or-later
"""Promote an explicitly accepted, clean candidate ZIP without rebuilding it."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import zipfile

def sha(data):return hashlib.sha256(data).hexdigest()

def promote(candidate,acceptance_path,output):
    data=Path(candidate).read_bytes();acceptance=json.loads(Path(acceptance_path).read_text())
    required=('full_install_verified','cold_start_verified','manual_manufacturer_return_verified','rotary_brightness_verified')
    if not all(acceptance.get(k) is True for k in required) or acceptance.get('candidate_sha256')!=sha(data):
        raise ValueError('Matching hardware acceptance required')
    with zipfile.ZipFile(io.BytesIO(data)) as original:
        names=original.namelist();package=json.loads(original.read('package.json'))
        if len(names)!=len(set(names)) or set(names)!=set(package['files'])|{'package.json'}:raise ValueError('Unexpected archive members')
        if acceptance['version']!=package['version'] or package.get('target')!='tc002':raise ValueError('Wrong release')
        for name,digest in package['files'].items():
            if name.startswith('/') or '\\' in name or ':' in name or any(p in ('..','backups','tools','.cache','.local') for p in name.split('/')):raise ValueError('Unsafe member')
            if sha(original.read(name))!=digest:raise ValueError('Candidate damaged')
        manifest_sha=sha(json.dumps(package['files'],sort_keys=True,separators=(',',':')).encode())
        if acceptance.get('payload_manifest_sha256')!=manifest_sha:raise ValueError('Tested payload differs')
        output=Path(output);output.mkdir(parents=True,exist_ok=True)
        filename=f"owlanzi-tc002-{package['version']}-windows.zip";target=output/filename
        if target.exists():raise ValueError('Use a fresh release output')
        package['hardware_verified']=True
        with zipfile.ZipFile(target,'w') as archive:
            for entry in original.infolist():
                body=(json.dumps(package,indent=2)+'\n').encode() if entry.filename=='package.json' else original.read(entry)
                archive.writestr(entry,body)
    with zipfile.ZipFile(target) as archive,zipfile.ZipFile(io.BytesIO(data)) as original:
        assert all(archive.read(n)==original.read(n) for n in names if n!='package.json')
    metadata={'schema':1,'target':'tc002','version':package['version'],'file':filename,'size_bytes':target.stat().st_size,'sha256':sha(target.read_bytes()),'hardware_verified':True,'payload_manifest_sha256':manifest_sha}
    (output/'tc002-windows.json').write_text(json.dumps(metadata,indent=2)+'\n')
    (output/'hardware-acceptance.json').write_text(json.dumps(acceptance,indent=2)+'\n')
    print(json.dumps(metadata));return metadata

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate',required=True,type=Path);parser.add_argument('--acceptance',required=True,type=Path);parser.add_argument('--output',required=True,type=Path)
    a=parser.parse_args();promote(a.candidate,a.acceptance,a.output)
