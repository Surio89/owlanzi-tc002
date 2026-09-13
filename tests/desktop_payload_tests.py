import hashlib,importlib.util,json,struct,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
spec=importlib.util.spec_from_file_location('packager',ROOT/'scripts/package-desktop.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)

class PayloadTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        self.files={'lib/libzkgui.so':b'new app','ui/main.ftu':b'ui','ui/cacert.pem':b'cert'}
        self.manifest={'version':p.APP_VERSION,'files':{name:{'size':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in {**self.files,'EasyUI.cfg':b'startup'}.items()}}
        self.metadata={'schema':1,'target':'tc002','abi':'z21-stock-1','version':p.APP_VERSION,'files':[{'path':name,**self.manifest['files'][name]} for name in self.files]}
        for name in self.files:
            path=self.root/'app'/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'original')
        (self.root/'app/EasyUI.cfg').write_bytes(b'startup');(self.root/'boot').mkdir();(self.root/'boot/shim').write_bytes(b'accepted boot')
    def tearDown(self):self.temp.cleanup()
    def data(self):
        header=json.dumps(self.metadata).encode();return b'OWLTC002'+struct.pack('<I',len(header))+header+b''.join(self.files.values())
    def apply(self,data):p.apply_app_update(self.root,data,self.manifest,hashlib.sha256(data).hexdigest())
    def test_app_overlay_preserves_accepted_boot_and_startup_config(self):
        self.apply(self.data())
        for name,data in self.files.items():self.assertEqual((self.root/'app'/name).read_bytes(),data)
        self.assertEqual((self.root/'boot/shim').read_bytes(),b'accepted boot');self.assertEqual((self.root/'app/EasyUI.cfg').read_bytes(),b'startup')
    def test_unexpected_version_or_path_cannot_replace_boot(self):
        self.metadata['version']='0.0.0'
        with self.assertRaises(ValueError):self.apply(self.data())
        self.metadata['version']=p.APP_VERSION;self.metadata['files'][0]['path']='../boot/shim'
        with self.assertRaises(ValueError):self.apply(self.data())
        self.assertEqual((self.root/'boot/shim').read_bytes(),b'accepted boot')
    def test_corrupt_truncated_extra_bytes_or_mismatched_config_leave_app_untouched(self):
        data=self.data()
        for broken in (data[:-1],data+b'x',data[:-1]+b'x'):
            with self.assertRaises(ValueError):self.apply(broken)
            self.assertEqual((self.root/'app/lib/libzkgui.so').read_bytes(),b'original')
        self.manifest['files']['EasyUI.cfg']['sha256']='0'*64
        with self.assertRaises(ValueError):self.apply(data)
        self.assertEqual((self.root/'app/lib/libzkgui.so').read_bytes(),b'original')

if __name__=='__main__':unittest.main()
