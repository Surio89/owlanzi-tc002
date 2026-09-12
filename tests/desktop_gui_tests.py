# SPDX-License-Identifier: GPL-3.0-or-later
import os
os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'desktop'))
from main import Wizard
from PySide6.QtWidgets import QApplication
from PySide6.QtTest import QTest

class GuiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.app=QApplication.instance() or QApplication([])
    def setUp(self):
        self.window=Wizard(ROOT,demo=True);self.window.show();QTest.qWait(200)
    def tearDown(self):self.window.close();self.window.deleteLater();self.app.processEvents()
    def test_discovery_selection_and_account_flow(self):
        self.assertEqual(len(self.window.found),1);self.assertTrue(self.window.next.isEnabled())
        self.window.next.click();self.assertEqual(self.window.pages.currentIndex(),2)
        self.window.email.setText('example@example.test');self.window.password.setText('fixture-only')
        self.window.save.click();QTest.qWait(100)
        self.assertEqual(self.window.password.text(),'');self.assertEqual(self.window.pages.currentIndex(),3)
    def test_multiple_clocks_require_selection_and_language_keeps_it(self):
        clock=self.window.found[0];other={**clock,'ip':'192.168.1.43'}
        self.window.result('search',[clock,other]);self.assertFalse(self.window.next.isEnabled())
        self.window.clocks.setCurrentRow(1);self.window.languages.setCurrentIndex(1)
        self.assertEqual(self.window.selected['ip'],'192.168.1.43');self.assertTrue(self.window.next.isEnabled())
        self.assertIn('Continue',self.window.next.text())
    def test_optional_setup_and_offline_error_do_not_claim_connection(self):
        self.window.open_account();self.window.complete(False)
        self.assertNotIn('verbunden',self.window.status.text())
        self.window.open_account();self.window.result('account',{'error':'web_password_required'})
        self.assertEqual(self.window.pages.currentIndex(),2);self.assertIn('Web-Passwort',self.window.status.text())

if __name__=='__main__':unittest.main()
