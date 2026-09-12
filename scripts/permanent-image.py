#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare a device-specific RES image. Never contacts or writes a clock.

Manufacturer resources are read from that owner's clock, retained locally and
never included in a public Owlanzi download. Rootfs, config and data are absent.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

def sha(data): return hashlib.sha256(data).hexdigest()

def squash_size(data):
    if len(data)<96 or data[:4]!=b'hsqs' or struct.unpack_from('<HH',data,28)!=(4,0):
        raise ValueError('Expected a SquashFS 4 resource partition')
    used=struct.unpack_from('<Q',data,40)[0]
    if not 96<=used<=len(data): raise ValueError('Invalid SquashFS size')
    return used

def prepare(stock, original, build, tools, output):
    from installer_image import layout,verify_preserved
    portable=isinstance(tools,dict)
    stock,original,build,output=map(lambda p:Path(p).resolve(),(stock,original,build,output))
    if not portable:tools=Path(tools).resolve()
    if output.exists(): raise ValueError('Use a new preparation directory; preserve existing backups')
    original_bytes=original.read_bytes()
    if len(original_bytes)!=0x800000: raise ValueError('Only the checked 8 MiB TC002 RES partition is supported')
    squash_size(original_bytes)
    original_layout=layout(original)
    cfg=json.loads((stock/'etc/EasyUI.cfg').read_text())
    persistent=cfg.get('startupLibPath')=='/res/lib/libowlanzi-boot.so'
    retained=stock/'etc/owlanzi-stock.cfg'
    if cfg.get('resPath')!='/res/ui/' or (not persistent and cfg.get('startupLibPath')!='/res/lib/libzkgui.so'):
        raise ValueError('Unknown original startup configuration')
    if persistent:
        if not retained.is_file():raise ValueError('Retained manufacturer configuration missing')
        manufacturer=json.loads(retained.read_text())
        if manufacturer.get('startupLibPath')!='/res/lib/libzkgui.so' or manufacturer.get('resPath')!='/res/ui/':
            raise ValueError('Unknown retained manufacturer configuration')
        if not all(name in original_layout for name in ('bin/owlanzi-boot-control','lib/libowlanzi-boot.so')):
            raise ValueError('Incomplete permanent boot installation')
    elif retained.exists():raise ValueError('Unexpected retained manufacturer configuration')
    files=sorted(p for p in stock.rglob('*') if p.is_file())
    if any(p.is_symlink() for p in stock.rglob('*')): raise ValueError('Unexpected local symlink in stock backup')
    if not (stock/'lib/libzkgui.so').is_file(): raise ValueError('Original app missing')
    output.mkdir(parents=True)
    copied=output/'res'
    shutil.copytree(stock,copied)
    if not persistent:(copied/'etc/owlanzi-stock.cfg').write_text(json.dumps(cfg)+'\n',encoding='utf-8')
    cfg['startupLibPath']='/res/lib/libowlanzi-boot.so'
    (copied/'etc/EasyUI.cfg').write_text(json.dumps(cfg)+'\n',encoding='utf-8')
    for source,target in [('libowlanzi-boot.so','lib/libowlanzi-boot.so'),('owlanzi-boot-control','bin/owlanzi-boot-control')]:
        data=(build/source).read_bytes()
        if data[:6]!=b'\x7fELF\x01\x01' or data[18:20]!=b'\x28\x00': raise ValueError('Not an ARM boot component')
        (copied/target).write_bytes(data)
    # The manufacturer's packer is acquired separately; its code is not distributed.
    def run(args): subprocess.run([str(a) for a in args],cwd=output,check=True,capture_output=True,timeout=180,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    modes=''.join(name+' m '+format(item['mode'],'04o')+' 1000 1000\n' for name,item in original_layout.items() if name)
    (output/'resource-modes.txt').write_text(modes+('' if persistent else 'bin/owlanzi-boot-control m 0755 1000 1000\n'),encoding='ascii')
    # The legacy Windows packer needs a relative source path to retain root
    # permissions correctly. Explicit pseudo modes preserve every other inode.
    squash=Path(tools['mksquashfs']) if portable else tools/'zkswe_mkimg.exe'
    options=['-root-mode',format(original_layout['']['mode'],'04o'),'-no-xattrs'] if portable else []
    run([squash,'res','res.squashfs','-noappend','-force-uid','1000','-force-gid','1000','-comp','xz','-processors','2','-pf','resource-modes.txt',*options])
    payload=(output/'res.squashfs').read_bytes()
    if squash_size(payload)>0x800000 or len(payload)>0x800000: raise ValueError('New RES exceeds the physical partition')
    preserved_entries=verify_preserved(original,output/'res.squashfs',replace_boot=persistent)
    # Z21 platform identifier and RES-only target from the manufacturer's packer.
    shutil.copyfile(original,output/'stock-res.squashfs')
    if portable:
        from installer_container import wrap_res,unwrap_res
        for name,data in (('update.img',payload),('restore.img',original_bytes)):
            image=wrap_res(data)
            if unwrap_res(image)!=data:raise ValueError('RES container validation failed')
            (output/name).write_bytes(image)
    else:
        run([tools/'fsimg.exe','-i','res:res.squashfs','-o','update.img','-p','0x000606'])
        run([tools/'fsimg.exe','-i','res:stock-res.squashfs','-o','restore.img','-p','0x000606'])
    preserved={p.relative_to(stock).as_posix():sha(p.read_bytes()) for p in files if p.relative_to(stock).as_posix() not in ({'etc/EasyUI.cfg','bin/owlanzi-boot-control','lib/libowlanzi-boot.so'} if persistent else {'etc/EasyUI.cfg'})}
    if any(sha((copied/p).read_bytes())!=digest for p,digest in preserved.items()): raise ValueError('Original resource changed')
    receipt={'schema':1,'target':'tc002','platform':'Z21','partition':'res','partition_size':0x800000,
             'stock_partition_sha256':sha(original_bytes),'stock_files_preserved':preserved,
             'new_squashfs_size':len(payload),'new_squashfs_sha256':sha(payload),
             'image_sha256':sha((output/'update.img').read_bytes()),'restore_sha256':sha((output/'restore.img').read_bytes()),
             'preserved_resource_entries':preserved_entries,'source_boot_mode':'persistent' if persistent else 'manufacturer','persistent_boot':True,'hardware_verified':False}
    (output/'preparation.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k!='stock_files_preserved'}))
    return receipt

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['stock','original','build','tools','output']:p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args();prepare(a.stock,a.original,a.build,a.tools,a.output)
