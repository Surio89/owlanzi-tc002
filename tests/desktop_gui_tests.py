# SPDX-License-Identifier: GPL-3.0-or-later
import os
os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock,patch
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'desktop'))
from main import Wizard
from update_dialog import UpdateDialog
from dialogs import confirmation_box
from PySide6.QtCore import Qt
from PySide6.QtWidgets import QMessageBox
from PySide6.QtWidgets import QApplication
from PySide6.QtTest import QTest

class GuiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.app=QApplication.instance() or QApplication([])
    def setUp(self):
        self.window=Wizard(ROOT,demo=True);self.window.show()
        self.wait_for(lambda:len(self.window.found)==1)
    def wait_for(self,condition):
        for _ in range(60):
            if condition():return
            QTest.qWait(50)
        self.assertTrue(condition(),'GUI action did not complete within three seconds')
    def tearDown(self):self.window.close();self.window.deleteLater();self.app.processEvents()
    def test_discovery_selection_and_account_flow(self):
        self.assertEqual(len(self.window.found),1);self.assertTrue(self.window.next.isEnabled())
        self.window.next.click();self.assertEqual(self.window.pages.currentIndex(),2)
        self.window.email.setText('example@example.test');self.window.password.setText('fixture-only')
        self.window.save.click();self.wait_for(lambda:self.window.pages.currentIndex()==3)
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
    def test_update_action_only_for_selected_owlanzi_clock(self):
        self.assertTrue(self.window.updates.isVisible());self.assertTrue(self.window.updates.isEnabled())
        clock={**self.window.found[0],'kind':'manufacturer','port':80}
        self.window.result('search',[clock]);self.assertFalse(self.window.updates.isVisible())
    def test_update_dialog_waits_for_actual_new_version_and_handles_lost_reply(self):
        client=Mock();state={'current':'0.2.4','latest':'0.3.0','phase':'available','available':True,'install_supported':True,'blocked':False,'rolled_back':False}
        client.status.return_value=state;client.install.return_value={**state,'phase':'queued'}
        dialog=UpdateDialog(self.window.found[0],client=client);dialog.show()
        self.wait_for(lambda:bool(dialog.state) and not dialog.busy);self.assertTrue(dialog.install.isEnabled())
        with patch('update_dialog.confirm',return_value=False):dialog.install.click()
        client.install.assert_not_called()
        with patch('update_dialog.confirm',return_value=True):dialog.install.click()
        self.wait_for(lambda:not dialog.busy);client.install.assert_called_once_with('0.3.0');self.assertEqual(dialog.target,'0.3.0')
        dialog.result('status',{'error':'clock_unreachable'});self.assertFalse(dialog.install.isEnabled());self.assertEqual(dialog.target,'0.3.0')
        dialog.result('status',{**state,'phase':'current','available':False,'current':'0.3.0'})
        self.assertEqual(dialog.target,'');self.assertIn('abgeschlossen',dialog.message.text());client.install.assert_called_once()
        dialog.reject()
    def test_update_dialog_current_version_and_error_do_not_claim_installed(self):
        client=Mock();client.status.return_value={'current':'0.3.0','latest':'0.3.0','phase':'current','available':False,'install_supported':True,'blocked':False,'rolled_back':False}
        dialog=UpdateDialog(self.window.found[0],client=client);dialog.show();self.wait_for(lambda:bool(dialog.state) and not dialog.busy)
        self.assertFalse(dialog.install.isEnabled());self.assertIn('aktuell',dialog.message.text())
        dialog.target='0.4.0';dialog.result('status',{**client.status.return_value,'phase':'error'})
        self.assertNotIn('abgeschlossen',dialog.message.text());self.assertFalse(dialog.install.isEnabled());dialog.reject()
    def test_preparation_steps_errors_and_retry_are_specific(self):
        controller=Mock();self.window.installer=controller;self.window.pages.setCurrentIndex(1)
        try:
            for step,word in [('checking','Systemdateien'),('backup','gesichert'),('preparing_image','Installationsabbild')]:
                controller.snapshot.return_value={'phase':'working','busy':True,'message':step,'detail':{}}
                self.window.refresh();self.assertIn(word,self.window.status.text());self.assertFalse(self.window.install.isVisible())
            controller.snapshot.return_value={'phase':'error','busy':False,'message':'error','detail':{'step':'preparing_image','code':'TC002-PREPARING_IMAGE'}}
            self.window.refresh();self.assertIn('TC002-PREPARING_IMAGE',self.window.status.text())
            self.assertNotIn('Netzwerkverbindung',self.window.status.text());self.assertTrue(self.window.retry.isVisible())
            self.assertFalse(self.window.install.isVisible())
            self.window.languages.setCurrentIndex(1);self.window.refresh();self.assertIn('installation image',self.window.status.text())
        finally:self.window.installer=None
    def test_install_requires_checkbox_then_explicit_confirmation(self):
        controller=Mock();controller.snapshot.return_value={'phase':'prepared','busy':False,'message':'prepared','detail':{}}
        self.window.installer=controller;self.window.demo=False;self.window.pages.setCurrentIndex(1)
        try:
            self.window.refresh();self.assertTrue(self.window.requirements.isVisible())
            self.assertFalse(self.window.install.isEnabled());self.assertTrue(self.window.install_hint.isVisible())
            with patch('main.confirm') as confirmation:self.window.start_install();confirmation.assert_not_called()
            self.window.power.setChecked(True)
            self.assertTrue(self.window.install.isEnabled());self.assertFalse(self.window.install_hint.isVisible())
            with patch('main.confirm',return_value=False):self.window.install.click()
            controller.begin.assert_not_called()
            self.window.power.setChecked(False)
            self.assertFalse(self.window.install.isEnabled());self.assertTrue(self.window.install_hint.isVisible())
            self.window.power.setChecked(True)
            with patch('main.confirm',return_value=True) as confirmation:self.window.install.click()
            self.assertEqual(confirmation.call_args.args[-2:],('Jetzt installieren','Abbrechen'))
            controller.begin.assert_called_once_with('install',{'confirm':'INSTALL OWLANZI'})
        finally:self.window.installer=None
    def test_confirmation_uses_localized_actions_and_cancels_on_enter_or_escape(self):
        for accept,cancel in [('Jetzt installieren','Abbrechen'),('Install now','Cancel')]:
            for key in (Qt.Key_Return,Qt.Key_Escape):
                box=confirmation_box(self.window,'Owlanzi','Install?',accept,cancel);box.show();self.app.processEvents()
                self.assertEqual(box.button(QMessageBox.Ok).text(),accept)
                self.assertEqual(box.button(QMessageBox.Cancel).text(),cancel)
                QTest.keyClick(box,key);self.app.processEvents()
                self.assertEqual(box.result(),QMessageBox.Cancel);box.deleteLater()

if __name__=='__main__':unittest.main()
