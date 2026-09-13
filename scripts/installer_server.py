# SPDX-License-Identifier: GPL-3.0-or-later
"""Local Windows installer. Network/device writes require explicit UI actions."""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
import importlib.util
import json
from pathlib import Path
import secrets
import threading
import time
import urllib.parse
import webbrowser

import installer_device as device
from installer_elf import symbols
from installer_image import extract_stock
import installer_tools

class Installer:
    def __init__(self,root,workspace=None):
        self.root=Path(root).resolve();self.lock=threading.Lock();self.token=secrets.token_urlsafe(32)
        self.workspace=Path(workspace).resolve() if workspace is not None else self.root
        self.info=json.loads((self.root/'package.json').read_text(encoding='utf-8'))
        if self.info.get('schema')!=1 or self.info.get('target')!='tc002':raise ValueError('Unexpected installer package')
        self.state={'phase':'welcome','busy':False,'version':self.info['version'],'message':'welcome','detail':{},'ip':''}
        self.tools=None;self.prepared=None;self.written=None
        self.verify_package()
    def verify_package(self):
        for relative,digest in self.info['files'].items():
            path=(self.root/relative).resolve()
            if not path.is_relative_to(self.root) or path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
                raise ValueError('Installer package damaged; download it again from owlanzi.com')
    def report(self,message,detail=None):
        with self.lock:self.state.update(message=message,detail=detail or {})
    def snapshot(self):
        with self.lock:return dict(self.state)
    def failure(self,action,error):
        # Diagnostics contain fixed step codes only, never command output,
        # exception text, Wi-Fi settings, account details or local file paths.
        step=self.snapshot()['message']
        allowed={'tools','download','extracting_tools','prepare','connecting','checking','backup','preparing_image','install','installing','installing_app','finish'}
        if step not in allowed:step=action if action in allowed else 'prepare'
        code='TC002-'+step.upper()
        try:
            self.workspace.mkdir(parents=True,exist_ok=True,mode=0o700)
            (self.workspace/'last-error.json').write_text(json.dumps({'code':code,'step':step,'time':int(time.time())},indent=2)+'\n')
        except OSError:pass
        phase='power_cycle' if action=='finish' else 'recovery' if action=='install' else 'error'
        with self.lock:self.state.update(phase=phase,busy=False,message='waiting_clock' if action=='finish' else phase,detail={'code':code,'step':step})
    def begin(self,action,body):
        phases={'tools':('welcome','error'),'prepare':('tools_ready','prepared','complete','error'),'install':('prepared',),'finish':('power_cycle',)}
        with self.lock:
            if self.state['busy'] or self.state['phase'] not in phases.get(action,()):raise ValueError('Action unavailable in the current step')
            if action=='tools' and body.get('confirm')!='DOWNLOAD TOOLS':raise ValueError('Download confirmation required')
            if action=='install' and body.get('confirm')!='INSTALL OWLANZI':raise ValueError('Installation confirmation required')
            if action=='prepare':body={'ip':device.device.private_ip(body.get('ip',''))}
            self.state.update(busy=True,phase='working',message=action,detail={})
        threading.Thread(target=self.perform,args=(action,body),daemon=True).start()
    def perform(self,action,body):
        try:
            if action=='tools':
                self.tools=installer_tools.prepare(self.workspace/'tools',self.report);phase='tools_ready'
            elif action=='prepare':self.prepare(body['ip']);phase='prepared'
            elif action=='install':phase=self.install()
            else:self.finish();phase='complete'
            with self.lock:self.state.update(phase=phase,busy=False,message=phase,detail={})
        except Exception as error:
            self.failure(action,error)
    def prepare(self,ip):
        if not self.tools:raise ValueError('Download tools first')
        self.verify_package();self.prepared=None
        self.report('connecting')
        d=device.connect(ip,self.tools['adb']);device.active_safe(d)
        work=self.workspace/'backups'/(ip+'-'+str(time.time_ns()));work.mkdir(parents=True,mode=0o700)
        self.report('checking')
        bundle,manifest=device.device.verified_bundle(self.root/'app')
        exports=d.preflight(bundle,manifest,None)
        for name in ('libowlanzi-boot.so','owlanzi-boot-control','ota-switch','owlanzi-install-guard'):
            if symbols(self.root/'boot'/name,True)-exports:raise ValueError('Clock system libraries do not support this installer')
        self.report('backup')
        original=work/'res-original.bin';d.command('pull','/dev/block/mtdblock3',str(original))
        if original.stat().st_size!=0x800000:raise ValueError('Unexpected resource partition size')
        for remote,name in [('/data/owlanzi/config.json','private-owlanzi-config.json'),('/data/misc/wifi/wpa_supplicant.conf','private-wifi.conf')]:
            if d.shell('test -f '+remote,check=False).returncode==0:d.command('pull',remote,str(work/name))
        uptime=float(d.shell('cat /proc/uptime').stdout.split()[0])
        permanent=d.shell('test -x /res/bin/owlanzi-boot-control',check=False).returncode==0
        boot_current=permanent
        if permanent:
            for name,remote in [('libowlanzi-boot.so','/res/lib/libowlanzi-boot.so'),('owlanzi-boot-control','/res/bin/owlanzi-boot-control')]:
                installed=work/('installed-'+name);d.command('pull',remote,str(installed))
                if installed.read_bytes()!=(self.root/'boot'/name).read_bytes():boot_current=False
        image=None
        if not boot_current:
            device.check_upgrader(d,work)
            self.report('preparing_image');stock=work/'original-res';extract_stock(original,stock)
            spec=importlib.util.spec_from_file_location('permanent_image',Path(__file__).with_name('permanent-image.py'))
            packer=importlib.util.module_from_spec(spec);spec.loader.exec_module(packer)
            image=work/'prepared-image';packer.prepare(stock,original,self.root/'boot',self.tools['packers'],image)
        self.prepared={'ip':ip,'work':work,'image':image,'permanent':permanent,'uptime':uptime}
        with self.lock:self.state.update(ip=ip,permanent=permanent)
    def install(self):
        if not self.prepared:raise ValueError('Check the clock first')
        self.verify_package();p=self.prepared
        args=(p['ip'],self.tools['adb'],self.root/'app',self.root/'boot')
        if p['image'] is None:
            device.install_app(*args,p['work']/'app-install',None,report=lambda _:self.report('installing_app'))
            return 'complete'
        self.written=device.install(*args,p['image'],p['work']/'install',None,report=lambda _:self.report('installing'))
        if not self.written.get('write_verified'):raise RuntimeError('Writing was not verified; keep the clock powered')
        return 'power_cycle'
    def finish(self):
        if not self.written or not self.written.get('write_verified'):raise ValueError('Writing must be verified first')
        p=self.prepared;d=device.connect(p['ip'],self.tools['adb'])
        # /tmp is cleared only by a real system restart, unlike an app restart.
        if d.shell('test -e /tmp/owlanzi-permanent-install/update.img',check=False).returncode==0:
            raise ValueError('Use the side power switch to turn the clock off for five seconds, then on. Unplugging alone does not restart it')
        cfg=json.loads(d.shell('cat /tmp/EasyUI.cfg').stdout);slot=device.slot_config(cfg)
        ready=d.shell('cat /tmp/owlanzi-ota-ready').stdout.split()
        if ready!=[slot,self.info['version']]:raise RuntimeError('Owlanzi startup not confirmed yet; wait and check again')
        if d.shell('test -x /res/bin/owlanzi-boot-control',check=False).returncode:raise ValueError('Permanent start not detected')
        (p['work']/'installation-confirmed.json').write_text(json.dumps({'version':self.info['version'],'persistent_boot':True,'confirmed_at':time.time()},indent=2))

def server_for(installer):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def reply(self,code,value,content_type='application/json; charset=utf-8'):
            data=value if isinstance(value,bytes) else json.dumps(value,ensure_ascii=False).encode()
            self.send_response(code);self.send_header('Content-Type',content_type);self.send_header('Content-Length',str(len(data)))
            self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff');self.send_header('Referrer-Policy','no-referrer')
            self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'none'")
            self.end_headers();self.wfile.write(data)
        def host_ok(self):return self.headers.get('Host')==f'127.0.0.1:{self.server.server_port}'
        def authenticated(self):return secrets.compare_digest(self.headers.get('X-Owlanzi-Installer',''),installer.token)
        def do_GET(self):
            if not self.host_ok():return self.reply(403,{'error':'Invalid host'})
            if self.path=='/api/state':
                if not self.authenticated():return self.reply(403,{'error':'Open the address from the starter window'})
                return self.reply(200,installer.snapshot())
            files={'/':('index.html','text/html; charset=utf-8'),'/app.js':('app.js','text/javascript; charset=utf-8'),'/style.css':('style.css','text/css; charset=utf-8')}
            if self.path not in files:return self.reply(404,{'error':'Not found'})
            name,mime=files[self.path];self.reply(200,(installer.root/'installer'/name).read_bytes(),mime)
        def do_POST(self):
            origin=f'http://127.0.0.1:{self.server.server_port}'
            if not self.host_ok() or not self.authenticated() or self.headers.get('Origin')!=origin:return self.reply(403,{'error':'Local setup session required'})
            if self.headers.get('Content-Type')!='application/json':return self.reply(415,{'error':'JSON required'})
            try:
                length=int(self.headers.get('Content-Length','0'))
                if not 1<=length<=2048:raise ValueError('Invalid request size')
                body=json.loads(self.rfile.read(length))
                if not isinstance(body,dict):raise ValueError('Invalid request')
                action=self.path.removeprefix('/api/')
                if self.path!='/api/'+action or action not in ('tools','prepare','install','finish'):return self.reply(404,{'error':'Not found'})
                installer.begin(action,body);self.reply(202,{'accepted':True})
            except (ValueError,TypeError):self.reply(400,{'error':'Check the current step and your entries'})
    return ThreadingHTTPServer(('127.0.0.1',0),Handler)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--root',required=True,type=Path);parser.add_argument('--no-browser',action='store_true');args=parser.parse_args()
    installer=Installer(args.root);server=server_for(installer);url=f'http://127.0.0.1:{server.server_port}/#'+installer.token
    print('Owlanzi TC002 setup. Keep this window open.\n'+url,flush=True)
    if not args.no_browser:webbrowser.open(url)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()
