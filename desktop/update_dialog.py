# SPDX-License-Identifier: GPL-3.0-or-later
"""Native UI for the existing TC002 OTA protocol, including restart polling."""
import threading
import time
from PySide6.QtCore import QObject,QTimer,Signal
from PySide6.QtWidgets import QDialog,QVBoxLayout,QLabel,QLineEdit,QPushButton,QProgressBar,QMessageBox
from installer_account import AccountError
from installer_update import ClockUpdater,BUSY

class Results(QObject):
    done=Signal(str,object)

class UpdateDialog(QDialog):
    def __init__(self,clock,lang='de',parent=None,client=None):
        super().__init__(parent);self.lang=lang;self.clock=clock
        self.client=client or ClockUpdater(clock['ip'],clock['port']);self.busy=False;self.target='';self.deadline=0;self.state={}
        self.events=Results();self.events.done.connect(self.result)
        self.setWindowTitle(self.t('Owlanzi · App aktualisieren','Owlanzi · Update app'));self.resize(570,450)
        layout=QVBoxLayout(self);layout.setContentsMargins(28,24,28,24);layout.setSpacing(15)
        heading=QLabel(self.t('Deine TC002 aktuell halten','Keep your TC002 up to date'));heading.setStyleSheet('font-size:24px;font-weight:600');layout.addWidget(heading)
        layout.addWidget(QLabel(clock['name']+' · '+clock['ip']))
        note=QLabel(self.t('Die Uhr lädt Updates von owlanzi.com. WLAN und Owlet-Einstellungen bleiben erhalten.','The clock downloads updates from owlanzi.com. Wi-Fi and Owlet settings are preserved.'));note.setWordWrap(True);layout.addWidget(note)
        layout.addWidget(QLabel(self.t('Web-Passwort der Uhr (nur falls eingerichtet)','Clock web password (only if configured)')))
        self.password=QLineEdit();self.password.setEchoMode(QLineEdit.Password);layout.addWidget(self.password)
        self.versions=QLabel();self.versions.setWordWrap(True);layout.addWidget(self.versions)
        self.message=QLabel();self.message.setWordWrap(True);layout.addWidget(self.message)
        self.progress=QProgressBar();self.progress.setRange(0,0);self.progress.hide();layout.addWidget(self.progress)
        self.check=QPushButton(self.t('Nach Updates suchen','Check for updates'));self.check.clicked.connect(self.check_update);layout.addWidget(self.check)
        self.install=QPushButton(self.t('Update installieren','Install update'));self.install.clicked.connect(self.install_update);self.install.setEnabled(False);layout.addWidget(self.install)
        self.dismiss=QPushButton(self.t('Zurück','Back'));self.dismiss.clicked.connect(self.reject);layout.addWidget(self.dismiss)
        self.timer=QTimer(self);self.timer.setInterval(2000);self.timer.timeout.connect(self.poll)
        QTimer.singleShot(0,lambda:self.run('status',self.client.status))
    def t(self,de,en):return de if self.lang=='de' else en
    def run(self,kind,fn):
        if self.busy:return
        self.busy=True;self.controls();self.progress.show()
        def work():
            try:result=fn()
            except AccountError as error:result={'error':str(error)}
            except Exception:result={'error':'clock_unreachable'}
            self.events.done.emit(kind,result)
        threading.Thread(target=work,daemon=True).start()
    def credentials(self):
        if self.password.text():self.client.web_password=self.password.text();self.password.clear()
    def check_update(self):
        if self.busy or self.target:return
        self.credentials();self.deadline=time.monotonic()+60
        self.run('check',self.client.check)
    def install_update(self):
        if self.busy or self.target or not self.install.isEnabled():return
        target=self.state['latest']
        message=self.t(f'App {target} auf dieser Uhr installieren? Die Anzeige wird kurz unterbrochen. Lass die Uhr am Strom.',f'Install app {target} on this clock? Its display will pause briefly. Keep the clock powered.')
        if QMessageBox.question(self,'Owlanzi',message)!=QMessageBox.Yes:return
        self.credentials();self.target=target;self.deadline=time.monotonic()+180
        self.run('install',lambda:self.client.install(target))
    def controls(self):
        active=self.busy or bool(self.target) or self.state.get('phase') in BUSY
        self.check.setEnabled(not active)
        self.install.setEnabled(not active and bool(self.state.get('available')) and bool(self.state.get('install_supported')) and not self.state.get('blocked',True) and self.state.get('phase')=='available')
        self.password.setEnabled(not active);self.dismiss.setEnabled(not active)
    def poll(self):
        if self.busy:return
        if self.deadline and time.monotonic()>self.deadline:
            self.timer.stop();self.target='';self.state={};self.progress.hide();self.controls()
            self.message.setText(self.t('Noch nicht bestätigt. Prüfe die Uhr in ihrer Weboberfläche, bevor du erneut installierst.','Not confirmed. Check the clock in its WebUI before installing again.'));return
        self.run('status',self.client.status)
    def result(self,kind,state):
        self.busy=False
        if 'error' in state:
            self.state={}
            definitive=state['error'] in ('update_state_changed','update_unavailable','web_password_required','not_owlanzi_tc002','invalid_update_version','clock_rejected_settings')
            if self.target and not definitive:
                # A dropped install reply must never trigger another POST.
                self.message.setText(self.t('Warte auf den Neustart der Uhr …','Waiting for the clock to restart …'));self.timer.start();self.controls();return
            self.target='';self.timer.stop();self.progress.hide()
            self.message.setText(self.t('Bitte das Web-Passwort der Uhr eingeben.','Enter the clock web password.') if state['error']=='web_password_required' else self.t('Aktualisierung nicht möglich. Prüfe Netzwerk und Uhrstatus und suche erneut nach Updates.','Update unavailable. Check the network and clock status, then check for updates again.'))
            self.controls();return
        self.state=state
        self.versions.setText(self.t('Installiert: ','Installed: ')+state.get('current','—')+self.t('    ·    Verfügbar: ','    ·    Latest: ')+(state.get('latest') or '—'))
        phase=state['phase']
        if self.target and state['current']==self.target and phase not in BUSY and not state.get('rolled_back'):
            self.target='';self.timer.stop();self.progress.hide();self.message.setText(self.t('Update abgeschlossen. Die neue App läuft auf deiner Uhr.','Update complete. The new app is running on your clock.'));self.controls();return
        if phase=='error' or state.get('rolled_back'):
            self.target='';self.timer.stop();self.progress.hide();self.message.setText(self.t('Update nicht bestätigt. Die Uhr hat einen Fehler gemeldet oder ist zur vorherigen App zurückgekehrt.','Update unconfirmed. The clock reported an error or returned to its previous app.'))
        elif phase in BUSY or self.target:
            if not self.deadline:self.deadline=time.monotonic()+180
            self.timer.start();self.progress.show();self.message.setText(self.t('Die Uhr prüft oder installiert das Update. Bitte warten und am Strom lassen …','The clock is checking or installing the update. Please wait and keep it powered …'))
        else:
            self.timer.stop();self.progress.hide()
            texts={'current':('Deine App ist aktuell.','Your app is up to date.'),'available':('Ein Update ist verfügbar.','An update is available.'),'unavailable':('Diese App unterstützt keine Online-Updates. Öffne die Weboberfläche der Uhr für weitere Informationen.','This app does not support online updates. Open its WebUI for more information.')}
            message=texts.get(phase,('Wähle „Nach Updates suchen“.','Choose “Check for updates”.'))
            if state.get('blocked'):message=('Verbinde die Uhr mit dem Heim-WLAN. Während eines aktiven Alarms sind Updates gesperrt.','Connect the clock to home Wi-Fi. Updates are blocked during an active alarm.')
            elif state.get('available') and not state.get('install_supported'):message=('Ein Update ist verfügbar, aber dieser Uhr fehlt der Update-Starter. Nutze die Installationsanleitung auf owlanzi.com.','An update is available, but this clock is missing its update loader. See the installation guide on owlanzi.com.')
            self.message.setText(self.t(*message))
        self.controls()
    def reject(self):
        if self.busy or self.target or self.state.get('phase') in BUSY:return
        self.timer.stop();self.client.close();self.password.clear();super().reject()
    def closeEvent(self,event):
        if self.busy or self.target or self.state.get('phase') in BUSY:event.ignore();return
        self.timer.stop();self.client.close();self.password.clear();event.accept()
