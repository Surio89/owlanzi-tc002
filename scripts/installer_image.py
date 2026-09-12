# SPDX-License-Identifier: GPL-3.0-or-later
"""Inspect and preserve an owner's RES filesystem without distributing it."""
import hashlib
from pathlib import Path
import re
from PySquashfsImage import SquashFsImage

def layout(path):
    result={}
    with SquashFsImage.from_file(str(path)) as image:
        if image.sblk.xattr_id_table_start!=0xffffffffffffffff:raise ValueError('Resource extended attributes are not supported')
        for entry in [image.root,*list(image)]:
            name=entry.path.lstrip('/')
            if name and (not re.fullmatch(r'[A-Za-z0-9_./+-]+',name) or '..' in name.split('/')):
                raise ValueError('Unsupported resource filename')
            if not (entry.is_file or entry.is_dir) or (entry.uid,entry.gid)!=(1000,1000):
                raise ValueError('Unsupported resource type or ownership')
            data=entry.read_bytes() if entry.is_file else None
            result[name]={'mode':entry.mode&0o7777,'directory':entry.is_dir,'size':len(data) if data is not None else 0,'sha256':hashlib.sha256(data).hexdigest() if data is not None else None}
    if len(result)>4096 or sum(item['size'] for item in result.values())>32*1024*1024:raise ValueError('Unexpected resource size')
    return result

def extract_stock(original,destination):
    metadata=layout(original);destination=Path(destination).resolve()
    if destination.exists():raise ValueError('Use a new backup directory')
    destination.mkdir(parents=True)
    with SquashFsImage.from_file(str(original)) as image:
        for entry in image:
            target=destination/entry.path.lstrip('/')
            if not target.resolve().is_relative_to(destination):raise ValueError('Resource path escapes backup')
            if entry.is_dir:target.mkdir(parents=True,exist_ok=True)
            else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(entry.read_bytes())
    return metadata

def verify_preserved(original,new_image,replace_boot=False):
    before=layout(original);after=layout(new_image)
    for name,info in before.items():
        if name not in after:raise ValueError('Original resource missing: '+name)
        expected=dict(info);actual=dict(after[name])
        if name=='etc/EasyUI.cfg' or (replace_boot and name in ('bin/owlanzi-boot-control','lib/libowlanzi-boot.so')):
            for field in ('size','sha256'):expected.pop(field);actual.pop(field)
        if actual!=expected:raise ValueError('Original resource or permissions changed: '+name)
    if after['bin/owlanzi-boot-control']['mode']!=0o755:raise ValueError('Boot controller is not executable')
    return len(before)
