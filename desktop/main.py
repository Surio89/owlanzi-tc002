# SPDX-License-Identifier: GPL-3.0-or-later
"""Native TC002 setup wizard. Background work never touches Qt widgets."""
import argparse
import json
import os
from pathlib import Path
import sys
import threading
import webbrowser

if not getattr(sys,'frozen',False):sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from PySide6.QtCore import QObject,QTimer,Signal,Qt
from PySide6.QtWidgets import (QApplication,QMainWindow,QWidget,QVBoxLayout,QHBoxLayout,
    QLabel,QPushButton,QLineEdit,QComboBox,QListWidget,QCheckBox,QProgressBar,QStackedWidget,QMessageBox,QScrollArea)
from installer_desktop import DesktopInstaller,workspace,VERSION
from installer_discovery import discover,identify,private_ip,clock_address
from installer_account import ClockAccount,AccountError
from update_dialog import UpdateDialog
from dialogs import confirm,inform,light_palette

class Events(QObject):
    result=Signal(str,object)
    search_progress=Signal(int,int,int)

class Wizard(QMainWindow):
    def __init__(self,root,work=None,demo=False):
        super().__init__();self.demo=demo;self.lang='de';self.selected=None;self.found=[];self.worker_busy=False
        self.cancel=threading.Event();self.events=Events();self.events.result.connect(self.result)
        self.events.search_progress.connect(self.search_progress)
        self.installer=None if demo else DesktopInstaller(root,work or workspace())
        self.account=None;self.device_signature=None;self.last_phase=None
        self.account_checks=0
        self.setWindowTitle('Owlanzi · TC002 Setup');self.resize(800,730);self.setMinimumSize(650,650)
        outer=QWidget();self.setCentralWidget(outer);layout=QVBoxLayout(outer);layout.setContentsMargins(34,28,34,24);layout.setSpacing(16)
        row=QHBoxLayout();brand=QLabel('owlanzi');brand.setObjectName('brand');row.addWidget(brand);row.addStretch()
        self.languages=QComboBox();self.languages.addItems(['Deutsch','English']);self.languages.currentIndexChanged.connect(self.translate);row.addWidget(self.languages);layout.addLayout(row)
        self.head=QLabel();self.head.setObjectName('heading');self.head.setWordWrap(True);layout.addWidget(self.head)
        self.intro=QLabel();self.intro.setWordWrap(True);layout.addWidget(self.intro)
        self.steps=QLabel();self.steps.setObjectName('steps');layout.addWidget(self.steps)
        self.pages=QStackedWidget();layout.addWidget(self.pages,1)
        self.widgets=[]
        self.build_search();self.build_install();self.build_account();self.build_done()
        # The main action remains visible when a small display needs scrolling.
        for index,button in ((0,self.next),(0,self.updates),(2,self.save)):
            self.pages.widget(index).widget().layout().removeWidget(button);layout.addWidget(button)
        self.pages.currentChanged.connect(self.navigation)
        self.navigation(0)
        self.status=QLabel();self.status.setWordWrap(True);layout.addWidget(self.status)
        self.progress=QProgressBar();self.progress.setRange(0,0);self.progress.hide();layout.addWidget(self.progress)
        self.footer=QLabel('TC002 · Desktop '+VERSION+' · Preview'+(' · DEMO' if demo else ''));self.footer.setObjectName('muted');layout.addWidget(self.footer)
        self.setPalette(light_palette())
        check_icon=Path(__file__).resolve().with_name('check.svg').as_posix()
        self.setStyleSheet('QMainWindow,QDialog{background:#f5f8f6} QWidget{font-size:14px;color:#253431} QLabel#brand{font-size:29px;font-weight:700;color:#087f70} QLabel#heading{font-size:28px;font-weight:700} QLabel#steps{font-size:13px;color:#087f70} QLabel#muted{color:#687c74;font-size:12px} QWidget#requirements{background:#fff;border:1px solid #9dc6b8;border-radius:8px} QLabel#requirementTitle{font-weight:600;color:#075f54} QCheckBox{spacing:10px;min-height:30px;font-weight:600} QCheckBox::indicator{width:22px;height:22px} QPushButton{padding:12px 18px;border:1px solid #b9cfc4;border-radius:7px;background:#fff} QPushButton#primary{background:#087f70;color:white;font-weight:600;border:0} QPushButton:disabled,QPushButton#primary:disabled{color:#596b62;background:#dfe7e2;border:1px solid #c2d0c8} QLineEdit,QComboBox,QListWidget{background:#fff;border:1px solid #c9d8d0;border-radius:6px;padding:9px} QListWidget::item{padding:12px} QListWidget::item:selected{background:#d7eee3;color:#174437} QProgressBar{max-height:6px;border:0;background:#dce7e0} QProgressBar::chunk{background:#087f70}')
        self.setStyleSheet(self.styleSheet()+f' QCheckBox::indicator{{background:#fff;border:2px solid #6b8378;border-radius:4px}} QCheckBox::indicator:checked{{background:#087f70;border-color:#087f70;image:url("{check_icon}")}} QCheckBox::indicator:hover{{border-color:#253431}}')
        self.translate();self.timer=QTimer(self);self.timer.timeout.connect(self.refresh);self.timer.start(600)
        QTimer.singleShot(100,self.search)
    def t(self,de,en):return de if self.lang=='de' else en
    def page(self):
        widget=QWidget();widget.setObjectName('page');widget.setStyleSheet('QWidget#page{background:#f5f8f6}')
        layout=QVBoxLayout(widget);layout.setContentsMargins(0,6,8,0);layout.setSpacing(13)
        scroll=QScrollArea();scroll.setWidgetResizable(True);scroll.setFrameShape(QScrollArea.NoFrame);scroll.setWidget(widget);self.pages.addWidget(scroll);return layout
    def navigation(self,index):
        self.next.setVisible(index==0);self.save.setVisible(index==2)
        self.updates.setVisible(index==0 and bool(self.selected) and self.selected['kind']=='owlanzi')
    def text(self,layout,de,en):
        label=QLabel();label.setWordWrap(True);layout.addWidget(label);self.widgets.append((label,de,en));return label
    def button(self,layout,de,en,action,primary=False):
        button=QPushButton();button.clicked.connect(action)
        if primary:button.setObjectName('primary')
        layout.addWidget(button);self.widgets.append((button,de,en));return button
    def build_search(self):
        page=self.page();self.text(page,'Uhr und Computer müssen im selben Heimnetz sein. Die Uhr bleibt am USB-Netzteil.','Connect the clock and computer to the same home network. Keep the clock on USB power.')
        self.clocks=QListWidget();self.clocks.currentRowChanged.connect(self.select_clock);page.addWidget(self.clocks)
        self.scan=self.button(page,'Erneut suchen','Search again',self.search)
        row=QHBoxLayout();self.ip=QLineEdit();self.ip.setPlaceholderText('192.168.1.123');row.addWidget(self.ip)
        self.manual=self.button(row,'Adresse prüfen','Check address',self.check_address);page.addLayout(row)
        self.help=self.text(page,'Keine Uhr gefunden? Verbinde eine neue TC002 zunächst über „U-Clock“ mit deinem Heim-WLAN. Eine bekannte IP-Adresse kannst du oben eingeben.','No clock found? First connect a new TC002 to home Wi-Fi through “U-Clock”. You can also enter a known IP address above.')
        self.next=self.button(page,'Mit dieser Uhr fortfahren','Continue with this clock',self.continue_clock,True);self.next.setEnabled(False)
        self.updates=self.button(page,'App-Updates für diese Uhr','App updates for this clock',self.open_updates);self.updates.hide()
    def build_install(self):
        page=self.page();self.install_note=self.text(page,'Vorschau: Die Erstinstallation mit diesem neuen Desktop-Helfer ist noch nicht an echter Hardware abgenommen. Die Uhr wird geprüft und ihre ursprünglichen Dateien werden auf diesem Computer gesichert.','Preview: first installation with this new desktop helper has not yet passed real-device acceptance. The clock will be checked and its original files backed up on this computer.')
        self.requirements=QWidget();self.requirements.setObjectName('requirements');card=QVBoxLayout(self.requirements);card.setContentsMargins(16,14,16,14);card.setSpacing(8)
        self.text(card,'Vor dem Installieren bestätigen','Confirm before installing').setObjectName('requirementTitle')
        self.text(card,'Die Uhr ist am USB-Netzteil, der Computer bleibt eingeschaltet und beide sind im selben Heimnetz.','The clock is on USB power, the computer stays awake and both are on the same home network.')
        self.power=QCheckBox();self.widgets.append((self.power,'Ja, diese Voraussetzungen sind erfüllt.','Yes, these requirements are met.'));card.addWidget(self.power);page.addWidget(self.requirements)
        self.install_hint=self.text(page,'Bitte das Kästchen oben anhaken. Damit schaltest du „Owlanzi installieren“ frei.','Tick the box above to enable “Install Owlanzi”.')
        self.power.toggled.connect(self.refresh)
        self.install=self.button(page,'Owlanzi installieren','Install Owlanzi',self.start_install,True)
        self.finish=self.button(page,'Neustart ist durchgeführt · Start prüfen','Restart completed · Check startup',self.finish_install,True)
        self.retry=self.button(page,'Verbindung und Vorbereitung erneut prüfen','Check connection and preparation again',self.prepare_clock)
        page.addStretch();self.skip=self.button(page,'Weiter zur Owlet-Einrichtung','Continue to Owlet setup',self.open_account,True)
        self.backups=self.button(page,'Sicherungsordner öffnen','Open backup folder',self.open_backups)
    def build_account(self):
        page=self.page();self.text(page,'Die Daten gehen direkt an deine ausgewählte Uhr. Der Helfer speichert das Passwort nicht auf diesem Computer.','Details go directly to your selected clock. The helper does not save the password on this computer.')
        self.text(page,'Owlet-E-Mail','Owlet email');self.email=QLineEdit();page.addWidget(self.email)
        self.text(page,'Owlet-Passwort','Owlet password');self.password=QLineEdit();self.password.setEchoMode(QLineEdit.Password);page.addWidget(self.password)
        self.region=QComboBox();page.addWidget(self.region)
        self.text(page,'Web-Passwort der Uhr (nur falls eingerichtet)','Clock web password (only if configured)');self.web_password=QLineEdit();self.web_password.setEchoMode(QLineEdit.Password);page.addWidget(self.web_password)
        self.save=self.button(page,'Auf der Uhr speichern und Verbindung prüfen','Save on clock and check connection',self.save_account,True)
        self.paired=QComboBox();self.paired.hide();page.addWidget(self.paired)
        self.choose=self.button(page,'Diese Socke verwenden','Use this sock',self.choose_sock);self.choose.hide()
        self.check=self.button(page,'Bestehende Owlet-Verbindung prüfen','Check existing Owlet connection',self.check_account)
        self.later=self.button(page,'Später auf der Uhr einrichten','Set up on the clock later',lambda:self.complete(False))
    def build_done(self):
        page=self.page();self.done_text=self.text(page,'Owlanzi ist bereit. Die Uhr läuft selbstständig weiter. Du kannst den Helfer schließen.','Owlanzi is ready. The clock runs independently. You can close the helper.')
        self.button(page,'Weboberfläche meiner Uhr öffnen','Open my clock’s web interface',self.open_clock,True)
        self.button(page,'App-Updates prüfen','Check app updates',self.open_updates)
        self.button(page,'Weitere Uhr einrichten','Set up another clock',self.another_clock);page.addStretch()
    def translate(self,*args):
        self.lang='de' if self.languages.currentIndex()==0 else 'en'
        self.head.setText(self.t('Deine TC002. Einfach eingerichtet.','Your TC002. Easy to set up.'))
        self.intro.setText(self.t('Wir finden deine Uhr und begleiten dich bis zur Owlet-Verbindung.','We find your clock and guide you through connecting Owlet.'))
        self.steps.setText(self.t('1  Uhr finden     →     2  Installieren     →     3  Owlet einrichten','1  Find clock     →     2  Install     →     3  Set up Owlet'))
        for widget,de,en in self.widgets:widget.setText(self.t(de,en))
        index=self.region.currentIndex();self.region.clear();self.region.addItems([self.t('Europa','Europe'),self.t('International','International')]);self.region.setCurrentIndex(max(0,index))
    def background(self,kind,fn):
        if self.worker_busy:return
        self.worker_busy=True;self.progress.setRange(0,0);self.progress.show();self.scan.setEnabled(False);self.manual.setEnabled(False);self.next.setEnabled(False);self.updates.setEnabled(False)
        self.save.setEnabled(False);self.check.setEnabled(False);self.choose.setEnabled(False);self.later.setEnabled(False)
        def run():
            try:value=fn()
            except AccountError as error:value={'error':str(error)}
            except Exception:value={'error':'operation_failed'}
            self.events.result.emit(kind,value)
        threading.Thread(target=run,daemon=True).start()
    def search(self):
        if self.worker_busy:return
        self.cancel.clear();self.status.setText(self.t('Suche Uhren im Heimnetz …','Searching for clocks on your home network …'))
        self.background('search',lambda:[{'ip':'192.168.1.42','port':8080,'kind':'owlanzi','name':'Owlanzi TC002','version':'0.3.0','protected':False}] if self.demo else discover(self.cancel,report=lambda done,total,found:self.events.search_progress.emit(done,total,len(found))))
    def search_progress(self,done,total,found):
        self.progress.setRange(0,max(1,total));self.progress.setValue(done)
        self.status.setText(self.t(f'Prüfe Heimnetz: {done}/{total} Adressen · {found} Uhr(en) gefunden.',f'Checking home network: {done}/{total} addresses · {found} clock(s) found.'))
    def check_address(self):
        try:ip=clock_address(self.ip.text())
        except ValueError:self.status.setText(self.t('Bitte eine gültige lokale IP-Adresse eingeben.','Enter a valid local IP address.'));return
        self.background('manual',lambda:identify(ip,timeout=2))
    def select_clock(self,index):
        self.selected=self.found[index] if 0<=index<len(self.found) else None
        self.next.setEnabled(bool(self.selected) and not self.worker_busy)
        self.updates.setEnabled(bool(self.selected) and not self.worker_busy);self.navigation(self.pages.currentIndex())
    def open_updates(self):
        if not self.selected or self.worker_busy or self.demo:return
        clock={**self.selected,'port':self.selected['port'] if self.selected['kind']=='owlanzi' else 8080,'name':'Owlanzi TC002'}
        UpdateDialog(clock,self.lang,self).exec()
    def continue_clock(self):
        if not self.selected or self.worker_busy:return
        if self.selected['kind']=='owlanzi':self.open_account()
        else:self.pages.setCurrentIndex(1);self.prepare_clock()
    def prepare_clock(self):
        if self.demo:return
        self.pages.setCurrentIndex(1);self.last_phase=None
        state=self.installer.snapshot()
        if state['busy'] or state['phase']=='recovery':return
        self.installer.begin('tools',{'confirm':'DOWNLOAD TOOLS'})
    def start_install(self):
        if not self.power.isChecked() or self.demo:return
        text=self.t('Owlanzi auf dieser TC002 installieren? Die Anzeige wird unterbrochen. WLAN und Einstellungen bleiben erhalten.','Install Owlanzi on this TC002? The display will pause. Wi-Fi and settings are preserved.')
        if confirm(self,self.t('Installation bestätigen','Confirm installation'),text,self.t('Jetzt installieren','Install now'),self.t('Abbrechen','Cancel')):self.installer.begin('install',{'confirm':'INSTALL OWLANZI'})
    def finish_install(self):
        if not self.demo:self.installer.begin('finish',{})
    def open_account(self):
        self.pages.setCurrentIndex(2);self.status.setText(self.t('Owlet einrichten oder die bestehende Verbindung prüfen.','Set up Owlet or check the existing connection.'))
    def account_client(self):
        if self.account:self.account.close()
        port=self.selected['port'] if self.selected['kind']=='owlanzi' else 8080
        self.account=ClockAccount(self.selected['ip'],port,self.web_password.text())
        self.web_password.clear();return self.account
    def save_account(self):
        if self.worker_busy:return
        email,password=self.email.text(),self.password.text();region='eu' if self.region.currentIndex()==0 else 'world';language=self.lang
        self.password.clear();client=self.account_client()
        self.account_checks=20
        def save():
            if self.demo:return {'cloud_fresh':True,'devices':[]}
            client.configure(email,password,region,language)
            return client.status()
        self.status.setText(self.t('Die Uhr prüft die Owlet-Verbindung …','The clock is checking the Owlet connection …'));self.background('account',save)
    def check_account(self):
        client=self.account if self.account and not self.web_password.text() else self.account_client()
        self.background('account',lambda:{'cloud_fresh':True,'devices':[]} if self.demo else client.status())
    def choose_sock(self):
        serial=self.paired.currentData();client=self.account
        def choose():client.choose_device(serial);return client.status()
        if serial and client:self.background('account',choose)
    def complete(self,connected):
        self.pages.setCurrentIndex(3);self.password.clear();self.web_password.clear()
        if self.account:self.account.close();self.account=None
        self.status.setText(self.t('Owlet ist verbunden.','Owlet is connected.') if connected else self.t('Du kannst Owlet später in der Weboberfläche der Uhr einrichten.','You can set up Owlet later in the clock’s web interface.'))
    def another_clock(self):
        if self.account:self.account.close();self.account=None
        self.email.clear();self.password.clear();self.web_password.clear();self.power.setChecked(False)
        if not self.demo:self.installer=DesktopInstaller(self.installer.root,self.installer.workspace)
        self.last_phase=None;self.pages.setCurrentIndex(0);self.search()
    def open_clock(self):
        if self.selected:
            port=self.selected['port'] if self.selected['kind']=='owlanzi' else 8080
            webbrowser.open(f"http://{private_ip(self.selected['ip'])}:{port}/")
    def open_backups(self):
        if self.installer:
            from PySide6.QtCore import QUrl
            from PySide6.QtGui import QDesktopServices
            QDesktopServices.openUrl(QUrl.fromLocalFile(str(self.installer.workspace/'backups')))
    def result(self,kind,value):
        self.worker_busy=False;self.progress.hide();self.scan.setEnabled(True);self.manual.setEnabled(True)
        for button in (self.save,self.check,self.choose,self.later):button.setEnabled(True)
        if isinstance(value,dict) and 'error' in value:
            errors={'web_password_required':self.t('Bitte das Web-Passwort der Uhr eingeben.','Enter the clock’s web password.'),'invalid_email':self.t('Bitte die Owlet-E-Mail prüfen.','Check the Owlet email address.'),'invalid_password':self.t('Bitte das Owlet-Passwort eingeben.','Enter your Owlet password.')}
            self.status.setText(errors.get(value['error'],self.t('Das hat nicht geklappt. Prüfe Verbindung und Eingaben.','That did not work. Check the connection and your entries.')));return
        if kind in ('search','manual'):
            self.found=value if kind=='search' else [value] if value else []
            self.clocks.clear()
            for clock in self.found:self.clocks.addItem(clock['name']+'  ·  '+clock['ip']+'  ·  '+clock['version'])
            if len(self.found)==1:self.clocks.setCurrentRow(0)
            self.status.setText(self.t(f'{len(self.found)} Uhr(en) gefunden. Wähle deine Uhr aus.',f'{len(self.found)} clock(s) found. Select your clock.') if self.found else self.t('Keine Uhr gefunden. Prüfe WLAN, Gastnetz und die lokale Netzwerkfreigabe.','No clock found. Check Wi-Fi, guest-network isolation and local-network permission.'))
        elif kind=='account':
            if value.get('cloud_fresh'):self.complete(True);return
            devices=value.get('devices') or [];self.paired.clear()
            for item in devices:self.paired.addItem(item['name']+' · '+item['serial'],item['serial'])
            self.paired.setVisible(len(devices)>1);self.choose.setVisible(len(devices)>1)
            self.status.setText(self.t('Wähle deine Socke aus.','Select your sock.') if len(devices)>1 else self.t('Noch nicht verbunden. Warte kurz und prüfe erneut. Kontrolliere bei Bedarf E-Mail, Passwort und Kontoregion.','Not connected yet. Wait briefly and check again. If needed, verify email, password and account region.'))
            if self.account_checks>0 and len(devices)<=1 and not value.get('has_error'):
                self.account_checks-=1
                QTimer.singleShot(3000,lambda:self.check_account() if self.pages.currentIndex()==2 and not self.worker_busy else None)
    def refresh(self,*args):
        if not self.installer or self.pages.currentIndex()!=1:return
        state=self.installer.snapshot();phase=state['phase'];self.progress.setVisible(state['busy'])
        self.install.setVisible(phase=='prepared');self.install.setEnabled(phase=='prepared' and self.power.isChecked())
        self.requirements.setVisible(phase=='prepared');self.install_hint.setVisible(phase=='prepared' and not self.power.isChecked());self.finish.setVisible(phase=='power_cycle');self.finish.setEnabled(not state['busy'])
        self.skip.setVisible(phase=='complete');self.retry.setVisible(phase=='error');self.backups.setVisible(phase in ('prepared','power_cycle','complete','recovery'))
        phase_key=(phase,state.get('message'),self.lang,str(state.get('detail')))
        if phase_key==self.last_phase:return
        self.last_phase=phase_key
        if phase=='tools_ready':self.installer.begin('prepare',{'ip':self.selected['ip']});return
        steps={
            'tools':('Installationswerkzeuge werden geprüft …','Checking installation tools …'),
            'download':('Installationswerkzeuge werden heruntergeladen …','Downloading installation tools …'),
            'extracting_tools':('Installationswerkzeuge werden entpackt …','Extracting installation tools …'),
            'connecting':('Installationszugang zur Uhr wird geprüft …','Checking the clock’s installation connection …'),
            'checking':('Systemdateien der Uhr werden gelesen und geprüft …','Reading and checking the clock’s system files …'),
            'backup':('Originalsoftware und Einstellungen werden auf diesem Computer gesichert …','Backing up original software and settings on this computer …'),
            'preparing_image':('Das Installationsabbild wird auf diesem Computer erstellt und geprüft …','Building and checking the installation image on this computer …')}
        if phase=='working' and state.get('message') in steps:
            self.status.setText(self.t(*steps[state['message']]));return
        if phase=='error':
            detail=state.get('detail',{});step=detail.get('step','prepare')
            errors={
                'download':('Die Werkzeuge konnten nicht heruntergeladen werden. Prüfe die Internetverbindung und versuche es erneut.','Tools could not be downloaded. Check the internet connection and retry.'),
                'tools':('Die Installationswerkzeuge konnten nicht vorbereitet werden. Lade den aktuellen Installer erneut herunter.','Installation tools could not be prepared. Download the current installer again.'),
                'extracting_tools':('Die Werkzeuge konnten auf diesem Computer nicht entpackt werden. Prüfe freien Speicher und versuche es erneut.','Tools could not be extracted on this computer. Check free disk space and retry.'),
                'connecting':('Der Installationszugang der Uhr antwortet nicht. Schließe andere Installer, lass die Uhr am Strom und prüfe erneut.','The clock’s installation connection is unavailable. Close other installers, keep the clock powered and retry.'),
                'checking':('Die Systemprüfung der Uhr konnte nicht abgeschlossen werden. Lass die Uhr am Strom und versuche es erneut. Bleibt der Fehler, gib den Fehlercode beim Support an.','The clock’s system check could not finish. Keep the clock powered and retry. If it persists, give support the error code.'),
                'backup':('Die Sicherung konnte nicht fertiggestellt werden. Prüfe freien Speicher und die Verbindung zur Uhr, dann versuche es erneut.','The backup could not finish. Check free disk space and the clock’s connection, then retry.'),
                'preparing_image':('Das Installationsabbild konnte auf diesem Computer nicht erstellt oder geprüft werden. Prüfe freien Speicher. Bleibt der Fehler, gib den Fehlercode beim Support an.','The installation image could not be built or verified on this computer. Check free disk space. If it persists, give support the error code.')}
            text=self.t(*errors.get(step,('Die Vorbereitung ist fehlgeschlagen. Versuche es erneut oder gib den Fehlercode beim Support an.','Preparation failed. Retry or give support the error code.')))
            self.status.setText(text+'\n'+detail.get('code','TC002-PREPARE'));return
        descriptions={
            'working':('Die Installation wird vorbereitet. Bitte warte …','Preparing installation. Please wait …'),
            'prepared':('Prüfungen erfolgreich. Deine Sicherung ist erstellt.','Checks passed. Your backup is ready.'),
            'power_cycle':('Schreiben geprüft. Uhr am Seitenschalter ausschalten, bei dunklem Display fünf Sekunden warten, einschalten und eine Minute warten. USB-Netzteil angeschlossen lassen. Nicht den Reset-Pin drücken.','Writing verified. Turn the clock off with the side switch, wait five seconds with a dark display, turn it on and wait one minute. Leave USB power connected. Do not press the reset pin.'),
            'complete':('Owlanzi startet dauerhaft. Weiter zur Owlet-Einrichtung.','Owlanzi starts persistently. Continue to Owlet setup.'),
            'recovery':('Installation nicht bestätigt. Uhr am Strom lassen. Nicht erneut installieren. Bitte Hilfe über owlanzi.com holen.','Installation unconfirmed. Keep the clock powered. Do not install again. Get help at owlanzi.com.'),
            'error':('Die Vorbereitung konnte nicht abgeschlossen werden. Prüfe die Netzwerkverbindung und versuche es erneut.','Preparation could not finish. Check the network connection and retry.')}
        if phase=='working' and state.get('message') in ('install','installing','installing_app'):
            self.status.setText(self.t('Owlanzi wird installiert und geprüft. Uhr am Strom lassen und dieses Fenster geöffnet halten.','Installing and verifying Owlanzi. Keep the clock powered and this window open.'));return
        self.status.setText(self.t(*descriptions.get(phase,('Bitte warten …','Please wait …'))))
    def closeEvent(self,event):
        if self.installer and self.installer.snapshot()['busy']:
            inform(self,'Owlanzi',self.t('Bitte den laufenden Installationsschritt abwarten.','Please wait for the active installation step to finish.'));event.ignore();return
        self.cancel.set()
        if self.account:self.account.close()
        self.password.clear();self.web_password.clear();event.accept()

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path);parser.add_argument('--workspace',type=Path);parser.add_argument('--demo',action='store_true');parser.add_argument('--self-test',type=Path);parser.add_argument('--image-fixture',type=Path);parser.add_argument('--discover-to',type=Path);parser.add_argument('--prepare-to',type=Path,help='Read-only preparation check; never installs on the clock');parser.add_argument('--address');args=parser.parse_args()
    if args.image_fixture and not args.self_test:parser.error('--image-fixture requires --self-test')
    root=args.root or Path(getattr(sys,'_MEIPASS',Path(__file__).resolve().parent))/'payload'
    if args.prepare_to:
        if not args.address or not args.workspace:parser.error('--prepare-to requires --address and an isolated --workspace')
        controller=DesktopInstaller(root,args.workspace)
        controller.perform('tools',{})
        if controller.snapshot()['phase']=='tools_ready':controller.perform('prepare',{'ip':clock_address(args.address)})
        state=controller.snapshot()
        args.prepare_to.write_text(json.dumps({'installer_version':VERSION,'phase':state['phase'],'detail':state['detail'],'device_writes':False},indent=2)+'\n')
        return 0 if state['phase']=='prepared' else 1
    if args.discover_to:
        clocks=discover() if not args.address else [item for item in [identify(clock_address(args.address),timeout=3)] if item]
        args.discover_to.write_text(json.dumps({'installer_version':VERSION,'clocks':clocks},indent=2)+'\n');return 0
    if args.address:parser.error('--address requires --discover-to or --prepare-to')
    if args.self_test:os.environ['QT_QPA_PLATFORM']='offscreen'
    app=QApplication(sys.argv);app.setApplicationName('Owlanzi Installer');app.setOrganizationDomain('owlanzi.com')
    root=args.root or Path(getattr(sys,'_MEIPASS',Path(__file__).resolve().parent))/'payload'
    if args.self_test:
        import tempfile
        from PySide6.QtGui import QPixmap
        with tempfile.TemporaryDirectory() as work:
            controller=DesktopInstaller(root,work)
            window=Wizard(root,work,demo=True);window.show();app.processEvents()
            result={'installer_version':VERSION,'app_version':controller.info['version'],'payload_verified':True,'native_gui_created':window.isVisible(),'network_requests':0}
            result['checkbox_icon_verified']=not QPixmap(str(Path(__file__).resolve().with_name('check.svg'))).isNull()
            if not result['checkbox_icon_verified']:raise ValueError('Packaged checkbox icon cannot be rendered')
            if sys.platform!='win32' or args.image_fixture:
                controller.prepare_tools();result['filesystem_tool_present']=bool(controller.tools)
            if args.image_fixture:
                # Full packaged image build with synthetic resources. Its root
                # mode differs from normal Windows ACLs and Unix temp folders.
                from installer_image import extract_stock,layout
                import importlib.util
                original=Path(work)/'original.bin'
                fixture=args.image_fixture.read_bytes()
                if len(fixture)>0x800000:raise ValueError('Invalid image fixture')
                original.write_bytes(fixture.ljust(0x800000,b'\xff'))
                stock=Path(work)/'stock';extract_stock(original,stock)
                import installer_server
                spec=importlib.util.spec_from_file_location('image_test_packer',Path(installer_server.__file__).with_name('permanent-image.py'))
                packer=importlib.util.module_from_spec(spec);spec.loader.exec_module(packer)
                built=Path(work)/'prepared';receipt=packer.prepare(stock,original,root/'boot',controller.tools['packers'],built)
                result['image_build_verified']=receipt['preserved_resource_entries']==len(layout(original))
                result['root_mode_preserved']=layout(built/'res.squashfs')['']['mode']==layout(original)['']['mode']
            window.close();args.self_test.write_text(json.dumps(result,indent=2)+'\n')
        return 0
    try:window=Wizard(root,args.workspace,args.demo)
    except Exception:inform(None,'Owlanzi','The installer package could not be verified. Please download it again from owlanzi.com.',QMessageBox.Critical);return 1
    window.show();return app.exec()
if __name__=='__main__':raise SystemExit(main())
