#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Install the OTA starter on an already locally tested TC002, or resume it after power loss.

Writes only /data/owlanzi-app and the temporary FlyThings startup configuration.
Does not flash partitions and does not turn this into a persistent boot image.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import time
import urllib.request

ROOT=Path(__file__).resolve().parents[1]
REMOTE='/data/owlanzi-app'
spec=importlib.util.spec_from_file_location('device',ROOT/'scripts/local-device.py')
device=importlib.util.module_from_spec(spec);spec.loader.exec_module(device)

def managed(cfg):
    return any(cfg.get('startupLibPath')==REMOTE+'/'+slot+'/lib/libzkgui.so' and cfg.get('resPath')==REMOTE+'/'+slot+'/ui/' for slot in ('a','b'))

def status(ip):
    with urllib.request.urlopen('http://'+ip+':8080/api/status',timeout=5) as r:return json.load(r)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('action',choices=['install','resume']);p.add_argument('--ip',required=True,type=device.private_ip);p.add_argument('--adb',required=True);p.add_argument('--build',type=Path,default=ROOT/'build/tc002');a=p.parse_args()
    d=device.Device(a.ip,a.adb);subprocess.run([a.adb,'connect',d.serial],capture_output=True,check=True,timeout=15)
    # Scope to the known board and its TC002 peripherals; no generic Android/Linux target.
    assert d.shell('getprop ro.product.model').stdout.strip()=='Zkswe_SSD21X_SPINOR'
    d.shell('test -e /dev/spidev0.0 && test -e /dev/ttyS1 && test -e /dev/input/event67 && test -e /dev/input/event68')
    work=ROOT/'.local'/('device-'+a.ip)/'ota-bootstrap';work.mkdir(parents=True,exist_ok=True)
    previous=d.active_config();assert previous,'Active configuration missing'
    previous_config=json.loads(previous)
    backup=work/('previous-'+str(time.time_ns())+'.cfg');backup.write_text(previous,encoding='utf-8')
    if a.action=='install':
        assert device.own_config(previous),'First run the local development build before enabling OTA; existing slots will not be overwritten'
        s=status(a.ip);assert s['target']=='tc002' if 'target' in s else s.get('mode')!='demo'
        assert not s.get('alarm',{}).get('critical'),'Wait until the critical alarm ends'
        assert d.shell('test ! -e '+REMOTE+'/a/manifest.json',check=False).returncode==0,'OTA slot already exists; use resume'
        bundle,manifest=device.verified_bundle(a.build/'device')
        d.preflight(bundle,manifest,ROOT/'.cache/tooling/z21/bin/arm-pc-linux-gnueabihf-readelf.exe')
        helper=a.build/'platform/tc002/ota-switch'
        image=helper.read_bytes();assert image[:6]==b'\x7fELF\x01\x01' and image[18:20]==b'\x28\x00'
        available=int(d.shell('df -k /data').stdout.splitlines()[-1].split()[3])*1024
        needed=sum((bundle/name).stat().st_size for name in device.PAYLOAD)*2+len(image)+524288
        assert available>needed,'Insufficient space for two app versions'
        d.shell('mkdir -p '+REMOTE+'/a/lib '+REMOTE+'/a/ui')
        cfg=dict(previous_config);cfg.update(startupLibPath=REMOTE+'/a/lib/libzkgui.so',resPath=REMOTE+'/a/ui/',languagePath=REMOTE+'/a/tr/')
        local_cfg=work/'a.cfg';local_cfg.write_text(json.dumps(cfg),encoding='utf-8')
        files=[(bundle/name,REMOTE+'/a/'+name) for name in ('lib/libzkgui.so','ui/main.ftu','ui/cacert.pem')]+[(helper,REMOTE+'/ota-switch'),(local_cfg,REMOTE+'/a/EasyUI.cfg')]
        slot={'version':manifest['version'],'target':'tc002','abi':'z21-stock-1','files':[{'path':name,**manifest['files'][name]} for name in ('lib/libzkgui.so','ui/main.ftu','ui/cacert.pem')]}
        meta=work/'manifest.json';meta.write_text(json.dumps(slot),encoding='utf-8');files.append((meta,REMOTE+'/a/manifest.json'))
        for local,remote in files:
            d.command('push',str(local),remote);check=work/'verify.bin';d.command('pull',remote,str(check));assert check.read_bytes()==local.read_bytes(),'Device write verification failed'
        d.shell('chmod 700 '+REMOTE+'/ota-switch');selected=REMOTE+'/a/EasyUI.cfg';version=manifest['version']
    else:
        selected=REMOTE+'/current.cfg';cfg=json.loads(d.shell('cat '+selected).stdout);assert managed(cfg),'No managed app found'
        base=cfg['resPath'].removesuffix('ui/');meta=json.loads(d.shell('cat '+base+'manifest.json').stdout);version=meta['version']
        for entry in meta['files']:
            assert entry['path'] in ('lib/libzkgui.so','ui/main.ftu','ui/cacert.pem')
            check=work/'verify.bin';d.command('pull',base+entry['path'],str(check));assert hashlib.sha256(check.read_bytes()).hexdigest()==entry['sha256']
    try:
        d.shell('setprop ctl.stop zkswe');time.sleep(1);d.shell('cp '+selected+' /tmp/EasyUI.cfg');d.shell('rm -f /tmp/owlanzi-ota-ready');d.shell('setprop ctl.start zkswe')
        for _ in range(35):
            time.sleep(1)
            try:
                if status(a.ip)['version']==version and version in d.shell('cat /tmp/owlanzi-ota-ready',check=False).stdout:break
            except (OSError,ValueError):pass
        else:raise RuntimeError('New app did not confirm startup')
        d.shell('cp /tmp/EasyUI.cfg '+REMOTE+'/current.cfg');d.shell('sync')
        print(json.dumps({'version':version,'ota_ready':True,'settings_preserved':True,'persistent_boot':False}))
    except BaseException:
        d.command('push',str(backup),'/tmp/owlanzi-bootstrap-restore.cfg');d.shell('setprop ctl.stop zkswe');time.sleep(1);d.shell('cp /tmp/owlanzi-bootstrap-restore.cfg /tmp/EasyUI.cfg');d.shell('setprop ctl.start zkswe');raise

if __name__=='__main__':main()
