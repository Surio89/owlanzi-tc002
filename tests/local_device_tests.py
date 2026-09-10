# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import subprocess
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('local_device', Path(__file__).resolve().parents[1] / 'scripts/local-device.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class LocalDeviceTests(unittest.TestCase):
    def test_legacy_adb_shell_checks_remote_exit_status(self):
        device = module.Device('192.168.1.2', 'adb')
        for code in [0, 1, 127]:
            result = subprocess.CompletedProcess([], 0, 'output\nOWLANZI_EXIT=' + str(code) + '\n', '')
            with patch.object(device, 'command', return_value=result):
                actual = device.shell('test -f /missing', check=False)
                self.assertEqual(actual.returncode, code)
                self.assertEqual(actual.stdout, 'output')
        with patch.object(device, 'command', return_value=subprocess.CompletedProcess([], 0, 'no sentinel', '')):
            with self.assertRaises(RuntimeError): device.shell('test -f /missing')

    def test_elf_preflight_respects_weak_imports_and_versions(self):
        fixture = '''Symbol table '.dynsym' contains entries:
  1: 00000000 0 FUNC GLOBAL DEFAULT UND clock_gettime@GLIBC_2.17 (2)
  2: 00000000 0 NOTYPE WEAK DEFAULT UND optional_hook
  3: 00000001 8 FUNC GLOBAL DEFAULT 12 clock_gettime@@GLIBC_2.17
  4: 00000001 8 FUNC LOCAL DEFAULT 12 private_helper
'''
        with patch.object(module.subprocess, 'check_output', return_value=fixture):
            self.assertEqual(module.elf_symbols('fixture-readelf', Path('fixture'), True), {'clock_gettime@GLIBC_2.17'})
            self.assertEqual(module.elf_symbols('fixture-readelf', Path('fixture'), False), {'clock_gettime@GLIBC_2.17','clock_gettime'})

    def test_only_explicit_lan_addresses(self):
        self.assertEqual(module.private_ip('192.168.1.2'), '192.168.1.2')
        for address in ['8.8.8.8', '127.0.0.1', '::1', '192.168.1.2;touch bad', 'example.com']:
            with self.assertRaises(ValueError):
                module.private_ip(address)

    def test_model_match_does_not_accept_random_text(self):
        self.assertEqual(module.model_labels({'data': {'model': 'Ulanzi TC002', 'password': 'private'}}), ['Ulanzi TC002'])
        self.assertEqual(module.model_labels({'description': 'TC002', 'name': 'TC002'}), [])

    def test_restore_requires_exact_own_paths(self):
        self.assertFalse(module.own_config('{"startupLibPath":"/res/lib/libzkgui.so"}'))
        self.assertFalse(module.own_config('comment ' + module.REMOTE))
        self.assertTrue(module.own_config(json.dumps({'startupLibPath': module.REMOTE+'/lib/libzkgui.so', 'resPath':module.REMOTE+'/ui/'})))

    def test_bundle_tampering_paths_and_target(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            elf = bytearray(64)
            elf[:6] = b'\x7fELF\x01\x01'
            elf[18:20] = (40).to_bytes(2, 'little')
            data = {'EasyUI.cfg':json.dumps({'startupLibPath':module.REMOTE+'/lib/libzkgui.so','resPath':module.REMOTE+'/ui/'}).encode(),
                    'lib/libzkgui.so':bytes(elf),'ui/main.ftu':b'fixture','ui/cacert.pem':b'fixture'}
            manifest = {'schema':1,'target':'tc002','kind':'flythings-debug-app','hardware_verified':False,'files':{}}
            for relative, content in data.items():
                path = root/relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(content)
                manifest['files'][relative] = {'size':len(content),'sha256':hashlib.sha256(content).hexdigest()}
            path = root/'manifest.json'
            path.write_text(json.dumps(manifest))
            module.verified_bundle(root)
            (root/'ui/main.ftu').write_bytes(b'tampered')
            with self.assertRaises(ValueError): module.verified_bundle(root)
            (root/'ui/main.ftu').write_bytes(data['ui/main.ftu'])
            manifest['files']['../escape'] = {'size':0,'sha256':'0'*64}
            path.write_text(json.dumps(manifest))
            with self.assertRaises(ValueError): module.verified_bundle(root)
            del manifest['files']['../escape']
            manifest['target']='tc001'
            path.write_text(json.dumps(manifest))
            with self.assertRaises(ValueError): module.verified_bundle(root)


if __name__ == '__main__':
    unittest.main()
