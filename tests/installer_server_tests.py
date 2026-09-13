# SPDX-License-Identifier: GPL-3.0-or-later
"""Local helper authorization and phase checks; never contacts a clock."""
import hashlib
import http.client
import json
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'scripts'),str(ROOT/'.cache/installer-python'),str(ROOT/'.cache/image-reader')]
from installer_server import Installer,server_for

class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        (self.root/'installer').mkdir();(self.root/'installer/index.html').write_text('fixture')
        digest=hashlib.sha256(b'fixture').hexdigest()
        (self.root/'package.json').write_text(json.dumps({'schema':1,'target':'tc002','version':'0.3.0','files':{'installer/index.html':digest}}))
        self.app=Installer(self.root);self.server=server_for(self.app)
        self.worker=threading.Thread(target=self.server.serve_forever,daemon=True);self.worker.start()
        self.origin=f'http://127.0.0.1:{self.server.server_port}'
    def tearDown(self):
        self.server.shutdown();self.server.server_close();self.worker.join();self.temp.cleanup()
    def call(self,path,body=None,token=True,origin=True,host=None):
        headers={'Content-Type':'application/json'}
        if token:headers['X-Owlanzi-Installer']=self.app.token
        if origin:headers['Origin']=self.origin
        if host:headers['Host']=host
        client=http.client.HTTPConnection('127.0.0.1',self.server.server_port,timeout=3)
        try:
            client.request('POST' if body is not None else 'GET',path,body=None if body is None else json.dumps(body),headers=headers)
            response=client.getresponse();data=response.read();return response.status,data,response.headers
        finally:client.close()
    def test_local_access_and_no_path_traversal(self):
        self.assertEqual(self.call('/')[0],200)
        self.assertEqual(self.call('/api/state',token=False)[0],403)
        self.assertEqual(self.call('/api/state',host='external.example')[0],403)
        self.assertEqual(self.call('/../package.json')[0],404)
        code,data,headers=self.call('/api/state')
        self.assertEqual(code,200);self.assertNotIn(self.app.token,data.decode())
        self.assertEqual(headers['Cache-Control'],'no-store')
        self.assertIn("frame-ancestors 'none'",headers['Content-Security-Policy'])
    def test_mutation_requires_origin_token_phase_and_confirmation(self):
        for token,origin in [(False,True),(True,False)]:
            self.assertEqual(self.call('/api/tools',{'confirm':'DOWNLOAD TOOLS'},token=token,origin=origin)[0],403)
        self.assertEqual(self.call('/api/tools',{})[0],400)
        self.assertEqual(self.call('/api/install',{'confirm':'INSTALL OWLANZI'})[0],400)
        self.app.state['phase']='tools_ready'
        for ip in ['8.8.8.8','127.0.0.1','example.com','192.168.1.2;reboot']:
            self.assertEqual(self.call('/api/prepare',{'ip':ip})[0],400)
        self.app.state['phase']='prepared'
        with patch.object(self.app,'perform') as perform:
            self.assertEqual(self.call('/api/install',{})[0],400)
            self.assertEqual(self.call('/api/install',{'confirm':'INSTALL OWLANZI'})[0],202)
            self.assertEqual(self.call('/api/install',{'confirm':'INSTALL OWLANZI'})[0],400)
            perform.assert_called_once_with('install',{'confirm':'INSTALL OWLANZI'})
    def test_damaged_package_stops_before_device_work(self):
        (self.root/'installer/index.html').write_text('changed')
        with self.assertRaises(ValueError):self.app.verify_package()
    def test_existing_permanent_boot_is_updated_only_when_an_image_was_prepared(self):
        self.app.tools={'adb':'fake-adb'}
        self.app.prepared={'ip':'192.168.1.2','work':self.root,'permanent':True,'image':None}
        with patch('installer_server.device.install_app') as app, patch('installer_server.device.install') as boot:
            self.assertEqual(self.app.install(),'complete')
            app.assert_called_once();boot.assert_not_called()
        self.app.prepared['image']=self.root/'prepared-boot-update'
        with patch('installer_server.device.install_app') as app, patch('installer_server.device.install',return_value={'write_verified':True}) as boot:
            self.assertEqual(self.app.install(),'power_cycle')
            boot.assert_called_once();app.assert_not_called()

    def test_failed_write_blocks_reinstall_but_start_check_is_retryable(self):
        with patch.object(self.app,'install',side_effect=RuntimeError('fixture')):
            self.app.perform('install',{})
        self.assertEqual(self.app.state['phase'],'recovery')
        self.assertEqual(self.call('/api/install',{'confirm':'INSTALL OWLANZI'})[0],400)
        self.assertEqual(self.call('/api/prepare',{'ip':'192.168.1.2'})[0],400)
        with patch.object(self.app,'finish',side_effect=RuntimeError('fixture')):self.app.perform('finish',{})
        self.assertEqual(self.app.state['phase'],'power_cycle')

if __name__=='__main__':unittest.main()
