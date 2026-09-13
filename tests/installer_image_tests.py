# SPDX-License-Identifier: GPL-3.0-or-later
"""Resource upgrade boundaries: retained manufacturer bytes cannot be replaced."""
from copy import deepcopy
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'scripts'),str(ROOT/'.cache/image-reader')]
import installer_image as images
class PreservationTests(unittest.TestCase):
    def setUp(self):
        self.before={name:{'mode':0o755 if name.startswith('bin/') else 0o770,'size':100,'sha256':'original','directory':False}
          for name in ('etc/EasyUI.cfg','etc/owlanzi-stock.cfg','lib/libzkgui.so','ui/main.ftu','bin/owlanzi-boot-control','lib/libowlanzi-boot.so')}
        self.after=deepcopy(self.before)
        for name in ('bin/owlanzi-boot-control','lib/libowlanzi-boot.so'):
            self.after[name].update(size=200,sha256='new-owlanzi-code')
    def verify(self,replace=True):
        with patch.object(images,'layout',side_effect=[self.before,self.after]):return images.verify_preserved('before','after',replace_boot=replace)
    def test_explicit_own_boot_update_retains_manufacturer_resources(self):
        self.assertEqual(self.verify(),len(self.before))
        with self.assertRaises(ValueError):self.verify(False)
    def test_retained_manufacturer_code_config_and_permissions_cannot_change(self):
        for name in ('etc/owlanzi-stock.cfg','lib/libzkgui.so','ui/main.ftu'):
            for field,value in [('sha256','changed'),('mode',0o777)]:
                with self.subTest(name=name,field=field):
                    original=self.after[name][field];self.after[name][field]=value
                    with self.assertRaises(ValueError):self.verify()
                    self.after[name][field]=original
    def test_missing_original_or_non_executable_boot_is_rejected(self):
        del self.after['ui/main.ftu']
        with self.assertRaises(ValueError):self.verify()
        self.after['ui/main.ftu']=self.before['ui/main.ftu'];self.after['bin/owlanzi-boot-control']['mode']=0o644
        with self.assertRaises(ValueError):self.verify()
if __name__=='__main__':unittest.main()
