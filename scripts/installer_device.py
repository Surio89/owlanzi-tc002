# SPDX-License-Identifier: GPL-3.0-or-later
"""Fixed operations for the TC002 installer; never accepts arbitrary shell commands."""
import hashlib
import importlib.util
import json
from pathlib import Path
import secrets
import socket
import subprocess
import time
import urllib.request
import urllib.error

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('installer_local_device',HERE/'local-device.py')
device=importlib.util.module_from_spec(spec);spec.loader.exec_module(device)
ROOT='/data/owlanzi-app/'
FILES=('lib/libzkgui.so','ui/main.ftu','ui/cacert.pem')
UPGRADER_SHA='8d5c39ef0dc598e3a00acd0cd90add79f9e1c969ed3e34d8c3e778b5ac142c96'
def sha(data):return hashlib.sha256(data).hexdigest()
def check_upgrader(d,work):
    for base in ('/lib/','/res/lib/','/usr/lib/'):
        if d.shell('test -f '+base+'libzkupgrade.so',check=False).returncode==0:
            local=Path(work)/'manufacturer-upgrader.so';d.command('pull',base+'libzkupgrade.so',str(local))
            if sha(local.read_bytes())!=UPGRADER_SHA:raise ValueError('This manufacturer upgrade version has not been verified. Installation stopped before writing.')
            return
    raise ValueError('Manufacturer upgrade library missing')
def connect(ip,adb):
    if hasattr(adb,'device'):
        d=adb.device(ip)
        if not d.reconnect():raise RuntimeError('Clock connection unavailable')
    else:
        d=device.Device(ip,str(adb))
        subprocess.run([str(adb),'connect',d.serial],capture_output=True,check=True,timeout=20)
    expected={'ro.product.model':'Zkswe_SSD21X_SPINOR','ro.product.board':'swaio','ro.hardware':'sstarsoc(flatteneddevicetree)'}
    if any(d.shell('getprop '+k).stdout.strip()!=v for k,v in expected.items()):raise ValueError('Unrecognized TC002 board')
    d.shell('test -e /dev/spidev0.0 && test -e /dev/ttyS1 && test -e /dev/input/event67 && test -e /dev/input/event68')
    if '00800000 00010000 "res"' not in d.shell('cat /proc/mtd').stdout:raise ValueError('Unsupported resource partition')
    if '<title>Ulanzi Clock' not in d.shell('cat /res/ui/web/uclockInfo.html').stdout:raise ValueError('Unrecognized manufacturer application')
    return d
def status(ip):
    request=urllib.request.Request('http://'+device.private_ip(ip)+':8080/api/status')
    with urllib.request.build_opener(urllib.request.ProxyHandler({})).open(request,timeout=3) as r:return json.load(r)
def slot_config(cfg):
    for slot in ('a','b'):
        if cfg.get('startupLibPath')==ROOT+slot+'/lib/libzkgui.so' and cfg.get('resPath')==ROOT+slot+'/ui/':return slot
    raise ValueError('Unknown Owlanzi slot configuration')
def transfer(d,local,remote,work):
    d.command('push',str(local),remote)
    check=work/'transfer-check.bin';d.command('pull',remote,str(check))
    if check.read_bytes()!=Path(local).read_bytes():raise RuntimeError('Device transfer verification failed')
def active_safe(d):
    active=d.active_config()
    if active:
        cfg=json.loads(active)
        if cfg.get('startupLibPath')=='/res/lib/libzkgui.so' and cfg.get('resPath')=='/res/ui/':return active
        if not device.own_config(active):slot_config(cfg)
        current=status(d.ip)
        if current.get('target')!='tc002' or current.get('mode')!='live' or current.get('alarm',{}).get('critical'):
            raise ValueError('Wait until Owlanzi has no critical alarm')
    return active
def start_slot(d,cfg,version,work):
    previous=d.active_config()
    local=work/'start.cfg';local.write_text(json.dumps(cfg),encoding='utf-8')
    transfer(d,local,'/tmp/owlanzi-installer-start.cfg',work)
    d.shell('setprop ctl.stop zkswe');time.sleep(1)
    d.shell('cp /tmp/owlanzi-installer-start.cfg /tmp/EasyUI.cfg')
    d.shell('rm -f /tmp/owlanzi-ota-ready');d.shell('setprop ctl.start zkswe')
    for attempt in range(40):
        time.sleep(1)
        if attempt%8==0:d.shell('setprop sys.zkapp.state running')
        try:
            ready=d.shell('cat /tmp/owlanzi-ota-ready',check=False).stdout.split()
            if ready!=[slot_config(cfg),version]:continue
            try:
                if status(d.ip).get('version')==version:return
            except urllib.error.HTTPError as error:
                if error.code==401 and 'Owlanzi TC002' in error.headers.get('WWW-Authenticate',''):return
        except (OSError,ValueError):pass
    # Restore exactly the previously active app on an unsuccessful trial.
    d.shell('setprop ctl.stop zkswe')
    if previous:
        local.write_text(previous,encoding='utf-8');transfer(d,local,'/tmp/EasyUI.cfg',work)
    else:d.shell('rm -f /tmp/EasyUI.cfg')
    d.shell('setprop ctl.start zkswe')
    raise RuntimeError('New app did not confirm startup; previous selection restored')
def stage_app(d,bundle,manifest,build,work,report=print):
    current=d.shell('cat '+ROOT+'current.cfg',check=False)
    if current.returncode==0:
        old=json.loads(current.stdout);slot=slot_config(old);target='b' if slot=='a' else 'a'
        (work/'previous-current.cfg').write_text(current.stdout,encoding='utf-8')
    else:old=None;target='a'
    available=int(d.shell('df -k /data').stdout.splitlines()[-1].split()[3])*1024
    # Reclaim only the inactive slot's fixed app files; never remove user settings.
    d.shell('mkdir -p '+ROOT+target+'/lib '+ROOT+target+'/ui')
    for name in FILES+('manifest.json','EasyUI.cfg'):d.shell('rm -f '+ROOT+target+'/'+name)
    available=int(d.shell('df -k /data').stdout.splitlines()[-1].split()[3])*1024
    if available<sum(manifest['files'][name]['size'] for name in FILES)+262144:raise ValueError('Not enough free app storage')
    report('Copying and verifying Owlanzi; existing settings stay on the clock')
    for name in FILES:transfer(d,bundle/name,ROOT+target+'/'+name,work)
    cfg=json.loads((bundle/'EasyUI.cfg').read_text());cfg.update(startupLibPath=ROOT+target+'/lib/libzkgui.so',resPath=ROOT+target+'/ui/',languagePath=ROOT+target+'/tr/')
    local=work/'slot.cfg';local.write_text(json.dumps(cfg),encoding='utf-8');transfer(d,local,ROOT+target+'/EasyUI.cfg',work)
    meta={'schema':1,'version':manifest['version'],'target':'tc002','abi':'z21-stock-1','files':[{'path':name,**manifest['files'][name]} for name in FILES]}
    local=work/'slot.json';local.write_text(json.dumps(meta));transfer(d,local,ROOT+target+'/manifest.json',work)
    transfer(d,Path(build)/'ota-switch',ROOT+'ota-switch',work);d.shell('chmod 700 '+ROOT+'ota-switch')
    report('Starting Owlanzi for its first verified trial')
    start_slot(d,cfg,manifest['version'],work)
    if old:d.shell('cp '+ROOT+'current.cfg '+ROOT+'previous.cfg')
    d.shell('cp /tmp/EasyUI.cfg '+ROOT+'current.cfg');d.shell('sync')
    return target

def install_app(ip,adb,bundle,build,work,readelf,report=print):
    work=Path(work);work.mkdir(parents=True,exist_ok=True)
    bundle,manifest=device.verified_bundle(bundle);d=connect(ip,adb);active_safe(d)
    d.preflight(bundle,manifest,readelf)
    target=stage_app(d,bundle,manifest,build,work,report)
    # Selecting Owlanzi in this installer is an explicit user action. Clear a
    # previous manufacturer selection only after the new Owlanzi trial passes.
    d.shell('rm -f '+ROOT+'manufacturer-selected');d.shell('sync')
    return {'version':manifest['version'],'target_slot':target}

def install_connection(d):
    """Probe/reconnect only the selected clock after the updater stops WLAN."""
    try:
        if hasattr(d,'reconnect'):return d.reconnect()
        with socket.create_connection((d.ip,5555),timeout=1.5):pass
        result=subprocess.run([str(d.adb),'connect',d.serial],capture_output=True,timeout=6)
        return result.returncode==0
    except (OSError,subprocess.TimeoutExpired):return False

def wait_install_result(d,run_id,size,work,report):
    deadline=time.monotonic()+300
    while time.monotonic()<deadline:
        time.sleep(2)
        if not install_connection(d):continue
        try:
            result=d.shell('cat /data/owlanzi-app/install-result.json',check=False)
        except (OSError,RuntimeError,subprocess.TimeoutExpired):continue
        if result.returncode:continue
        try:proof=json.loads(result.stdout)
        except (ValueError,TypeError):continue
        # A leftover receipt can never confirm this installation.
        if not isinstance(proof,dict) or proof.get('run_id')!=run_id:continue
        (work/'guard-result.json').write_text(json.dumps(proof,indent=2))
        resumed=all(proof.get(key) is True for key in ('write_verified','resources_remounted','resume_restored','app_restart_requested'))
        if proof.get('schema')!=1 or proof.get('phase')!='write_verified' or proof.get('verified_size')!=size or not resumed:
            raise RuntimeError('Clock-side write verification failed. Keep the clock powered and seek help.')
        return
    raise RuntimeError('Upgrade completion not confirmed. Keep the clock powered; do not retry blindly.')

def install(ip,adb,bundle,build,image,work,readelf,report=print):
    work=Path(work);work.mkdir(parents=True,exist_ok=True)
    bundle,manifest=device.verified_bundle(bundle);d=connect(ip,adb);active_safe(d)
    report('Checking the app and device libraries')
    check_upgrader(d,work)
    d.preflight(bundle,manifest,readelf)
    receipt=json.loads((Path(image)/'preparation.json').read_text())
    original=work/'res-before.bin';d.command('pull','/dev/block/mtdblock3',str(original))
    if sha(original.read_bytes())!=receipt['stock_partition_sha256']:raise ValueError('Clock changed since image preparation')
    for filename,key in [('update.img','image_sha256'),('restore.img','restore_sha256')]:
        if sha((Path(image)/filename).read_bytes())!=receipt[key]:raise ValueError('Prepared image changed')
    expected=(Path(image)/'res.squashfs').read_bytes()
    if not 96<=len(expected)<=0x800000 or sha(expected)!=receipt['new_squashfs_sha256']:
        raise ValueError('Prepared resource image changed')
    target=stage_app(d,bundle,manifest,build,work,report)
    report('Preparing the permanent manufacturer-format app installation')
    d.shell('mkdir -p /tmp/owlanzi-permanent-install')
    transfer(d,Path(image)/'update.img','/tmp/owlanzi-permanent-install/update.img',work)
    # Official FlyThings option: -1 prevents the stock warm reboot, which does
    # not reliably return on the tested TC002. Verify the written partition
    # before asking the owner to power-cycle; a timeout is never success.
    delay=work/'zkrebootdelay';delay.write_text('-1\n',encoding='ascii')
    transfer(d,delay,'/tmp/owlanzi-permanent-install/zkrebootdelay',work)
    transfer(d,Path(image)/'res.squashfs','/tmp/owlanzi-permanent-install/res.squashfs',work)
    transfer(d,Path(build)/'owlanzi-install-guard','/tmp/owlanzi-permanent-install/install-guard',work)
    d.shell('chmod 700 /tmp/owlanzi-permanent-install/install-guard')
    run_id=secrets.token_hex(16);run_file=work/'run-id';run_file.write_text(run_id,encoding='ascii')
    transfer(d,run_file,'/tmp/owlanzi-permanent-install/run-id',work)
    # These are the exact manufacturer-documented upgrade controls. The image
    # contains RES only, retaining the complete original application resources.
    d.shell('setprop sys.zkupgrade.state -1')
    d.shell('rm -f /tmp/owlanzi-permanent-install/guard-ready')
    d.shell('/tmp/owlanzi-permanent-install/install-guard --watch')
    for attempt in range(20):
        time.sleep(.1)
        if d.shell('cat /tmp/owlanzi-permanent-install/guard-ready',check=False).stdout==run_id:break
    else:raise RuntimeError('Clock-side verifier did not start; upgrade was not triggered.')
    d.shell('setprop sys.zkupgrade.flag 255')
    d.shell('setprop sys.zkupgrade.dir /tmp/owlanzi-permanent-install')
    (work/'upgrade-triggered.json').write_text(json.dumps({'version':manifest['version'],'image_sha256':receipt['image_sha256'],'run_id':run_id,'at':time.time()}))
    try:d.shell('setprop ctl.restart zkswe')
    except (OSError,RuntimeError,subprocess.TimeoutExpired):
        # The updater can stop WLAN before ADB returns its shell status. Never
        # repeat an uncertain trigger; only the matching local proof can pass.
        pass
    report('The manufacturer updater temporarily stops WLAN. The clock verifies its write locally, then restores the connection. Keep it powered.')
    wait_install_result(d,run_id,len(expected),work,report)
    readback=work/'res-written.bin';d.command('pull','/dev/block/mtdblock3',str(readback))
    written=readback.read_bytes()
    if len(written)!=0x800000 or written[:len(expected)]!=expected:
        raise RuntimeError('Resource write verification failed. Keep the clock powered and seek help.')
    proof={'version':manifest['version'],'target_slot':target,'image_sha256':receipt['image_sha256'],'write_verified':True,'power_cycle_required':True}
    (work/'write-verified.json').write_text(json.dumps(proof,indent=2))
    report('Writing is complete and verified. Use the side power switch to turn the clock off for five seconds, then on. Unplugging alone does not stop the battery-powered clock. Wait for Owlanzi.')
    return proof
