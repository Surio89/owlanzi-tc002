// SPDX-License-Identifier: GPL-3.0-or-later
export const PALETTE_KEYS = Object.freeze(['clock', 'heart', 'numbers', 'waiting', 'awake', 'light_sleep', 'deep_sleep', 'unknown_sleep', 'battery', 'alarm', 'info', 'offline', 'heart_wait', 'battery_frame', 'battery_fill', 'battery_charge', 'battery_mid', 'battery_low', 'oxygen', 'charging_text', 'battery_status', 'waiting_text', 'reconnect_text', 'setup_title']);
export const ALARM_LIMITS = Object.freeze({spo2_min:[50,99],heart_rate_min:[30,150],heart_rate_max:[100,260],spo2_seconds:[5,300],heart_rate_low_seconds:[5,300],heart_rate_high_seconds:[5,300],volume:[0,6],alarm_repeat_seconds:[0,120],alarm_brightness:[0,255]});
const translations = {
  de: {
    connecting:'Verbinden …',connected:'Verbunden',disconnected:'Nicht verbunden',language:'Sprache',deviceApplication:'DEINE UHR. DEINE EINSTELLUNGEN.',title:'Ein guter Blick auf die Nacht.',intro:'Owlanzi läuft selbstständig auf deiner TC002. Hier richtest du die Uhr ein.',connect:'Verbinden',previewBright:'Vorschau aufhellen · nur im Browser',clockBrightness:'Uhrhelligkeit: ',onYourClock:'AUF DEINER UHR',displayTitle:'Die Anzeige im Blick',demoNotice:'Entwicklungsmodus · simulierte Werte. Diese Ansicht zeigt keine aktuellen Owlet-Messdaten.',heartRate:'Puls',oxygen:'Sauerstoff',sleep:'Schlafstatus',battery:'Socke · Akku',acknowledge:'Alarm bestätigen',testDisplay:'Anzeige testen:',demoVitals:'Messwerte',demoCharging:'Laden',demoOffline:'Offline',demoAlarm:'Alarm',demoWaiting:'Warten',accountTitle:'Dein Konto verbinden',accountHelp:'Die Uhr ruft deine Owlet-Daten direkt ab. Dein Computer wird danach nicht benötigt.',email:'E-Mail-Adresse',password:'Passwort',clearPassword:'Gespeichertes Passwort entfernen',region:'Owlet-Region',regionEu:'Europa',regionWorld:'International',pollInterval:'Daten abrufen',every5:'Alle 5 Sekunden',every10:'Alle 10 Sekunden',every15:'Alle 15 Sekunden',deviceSerial:'Owlet-Gerät',deviceHelp:'Nach dem Verbinden stehen deine Geräte zur Auswahl. Leer lassen, um das erste verfügbare Gerät zu verwenden.',appearance:'DARSTELLUNG',appearanceTitle:'So leuchtet deine Uhr',brightness:'Helligkeit',brightnessHelp:'0 schaltet die Matrix dunkel. Die Anwendung läuft weiter.',palette:'Farben',paletteHelp:'Die Farben werden mit deinen Einstellungen auf der Uhr gespeichert.',optional:'OPTIONAL',alarmsTitle:'Hinweise und Ton',soundTitle:'Ton und Hinweishelligkeit',enableAlarms:'Eigene Grenzwerte',alarmsHelp:'Eigene Grenzwerte sind standardmäßig aus. Owlanzi ersetzt keine medizinische Überwachung; die Originalwarnungen von Owlet bleiben maßgeblich.',spo2Min:'Sauerstoff unter (%)',delaySeconds:'Dauer (Sekunden)',heartRateMin:'Puls unter (bpm)',heartRateMax:'Puls über (bpm)',soundEnabled:'Ton aktivieren',volume:'Lautstärke (0–6)',repeatSeconds:'Ton wiederholen (Sekunden)',repeatHelp:'0 spielt den Ton einmal ab.',alarmBrightness:'Helligkeit bei Hinweisen (0–255)',saveHelp:'Die Einstellungen werden lokal auf deiner Uhr gespeichert.',save:'Einstellungen speichern',saving:'Wird gespeichert …',saved:'Einstellungen gespeichert. Die Uhr übernimmt die Änderungen.',footer:'Owlanzi für TC002 · eigenständige Geräteanwendung',baseStation:'Die Owlet-Basisstation bleibt für die Verbindung der Socke erforderlich.',passwordSaved:'Ein Passwort ist gespeichert. Leer lassen, um es beizubehalten.',passwordEmpty:'Noch kein Passwort gespeichert.',demo:'Demo · simuliert',live:'Geräteanwendung',neverUpdated:'Noch keine aktuellen Messdaten',updated:'Aktualisiert: ',fresh:'Mit Owlet verbunden',stale:'Keine aktuellen Messdaten. Bitte Konto und Verbindung prüfen.',simulated:'Simulierte Daten · nur zum Testen der Anzeige',networkError:'Die Uhr ist nicht erreichbar. Prüfe die Verbindung und versuche es erneut.',requestError:'Die Anfrage konnte nicht ausgeführt werden.',invalidConfig:'Bitte prüfe die markierten Einstellungen.',invalidEmail:'Bitte gib eine gültige E-Mail-Adresse ein.',invalidRegion:'Bitte wähle eine gültige Owlet-Region.',invalidPoll:'Das Abrufintervall muss 5, 10 oder 15 Sekunden betragen.',invalidBrightness:'Die Helligkeit muss zwischen 0 und 255 liegen.',invalidPalette:'Bitte wähle für alle Anzeigen eine gültige Farbe.',invalidAlarm:'Bitte prüfe die Grenzwerte und die Dauer der Hinweise.',invalidHeartRange:'Der untere Pulsgrenzwert muss kleiner als der obere sein.',invalidPassword:'Bitte gib ein Passwort mit höchstens 512 Zeichen ein.',invalidDevice:'Die Geräteseriennummer darf höchstens 128 Zeichen enthalten.',invalidLanguage:'Bitte wähle Deutsch oder Englisch.',invalidFlags:'Bitte prüfe die Ein/Aus-Einstellungen.',alarmActive:'Ein Hinweis ist aktiv.',alarmAcknowledged:'Hinweis bestätigt',awake:'Wach',light_sleep:'Leichter Schlaf',deep_sleep:'Tiefer Schlaf',unknown_sleep:'Unbekannt',sleeping:'Schläft',charging:'Lädt',waiting:'Warten',offline:'Offline',heart:'Herzsymbol',numbers:'Messwerte',unknown:'Unbekannt',alarm:'Hinweise',info:'Information',matrixLabel:'52 × 16 LED-Anzeige',
  },
  en: {
    connecting:'Connecting …',connected:'Connected',disconnected:'Disconnected',language:'Language',deviceApplication:'YOUR CLOCK. YOUR SETTINGS.',title:'A little clarity through the night.',intro:'Owlanzi runs independently on your TC002. Set up your clock here.',connect:'Connect',previewBright:'Brighten preview · browser only',clockBrightness:'Clock brightness: ',onYourClock:'ON YOUR CLOCK',displayTitle:'Your display at a glance',demoNotice:'Development mode · simulated values. This view does not show current Owlet readings.',heartRate:'Heart rate',oxygen:'Oxygen',sleep:'Sleep status',battery:'Sock · battery',acknowledge:'Acknowledge alert',testDisplay:'Test display:',demoVitals:'Readings',demoCharging:'Charging',demoOffline:'Offline',demoAlarm:'Alert',demoWaiting:'Waiting',accountTitle:'Connect your account',accountHelp:'The clock fetches your Owlet data directly. Your computer can be switched off after setup.',email:'Email address',password:'Password',clearPassword:'Remove saved password',region:'Owlet region',regionEu:'Europe',regionWorld:'International',pollInterval:'Fetch data',every5:'Every 5 seconds',every10:'Every 10 seconds',every15:'Every 15 seconds',deviceSerial:'Owlet device',deviceHelp:'Your devices appear after connecting. Leave blank to use the first available device.',appearance:'APPEARANCE',appearanceTitle:'Make your clock your own',brightness:'Brightness',brightnessHelp:'0 turns the matrix dark. The application keeps running.',palette:'Colors',paletteHelp:'Colors are saved with your settings on the clock.',optional:'OPTIONAL',alarmsTitle:'Alerts and sound',soundTitle:'Sound and alert brightness',enableAlarms:'Custom thresholds',alarmsHelp:'Custom thresholds are off by default. Owlanzi does not replace medical monitoring; continue to rely on the original Owlet alerts.',spo2Min:'Oxygen below (%)',delaySeconds:'Duration (seconds)',heartRateMin:'Heart rate below (bpm)',heartRateMax:'Heart rate above (bpm)',soundEnabled:'Enable sound',volume:'Volume (0–6)',repeatSeconds:'Repeat sound (seconds)',repeatHelp:'0 plays the sound once.',alarmBrightness:'Alert brightness (0–255)',saveHelp:'Settings are saved locally on your clock.',save:'Save settings',saving:'Saving …',saved:'Settings saved. The clock is applying your changes.',footer:'Owlanzi for TC002 · standalone device application',baseStation:'The Owlet base station is still required to connect the sock.',passwordSaved:'A password is saved. Leave blank to keep it.',passwordEmpty:'No password saved yet.',demo:'Demo · simulated',live:'Device application',neverUpdated:'No current readings yet',updated:'Updated: ',fresh:'Connected to Owlet',stale:'No current readings. Check your account and connection.',simulated:'Simulated data · for testing the display only',networkError:'The clock is unreachable. Check your connection and try again.',requestError:'The request could not be completed.',invalidConfig:'Please check the highlighted settings.',invalidEmail:'Please enter a valid email address.',invalidRegion:'Please select a valid Owlet region.',invalidPoll:'The polling interval must be 5, 10 or 15 seconds.',invalidBrightness:'Brightness must be between 0 and 255.',invalidPalette:'Please select a valid color for each display element.',invalidAlarm:'Please check alert thresholds and durations.',invalidHeartRange:'The lower heart rate threshold must be smaller than the upper one.',invalidPassword:'Please enter a password no longer than 512 characters.',invalidDevice:'The device serial must not exceed 128 characters.',invalidLanguage:'Please select German or English.',invalidFlags:'Please check the on/off settings.',alarmActive:'An alert is active.',alarmAcknowledged:'Alert acknowledged',awake:'Awake',light_sleep:'Light sleep',deep_sleep:'Deep sleep',unknown_sleep:'Unknown',sleeping:'Sleeping',charging:'Charging',waiting:'Waiting',offline:'Offline',heart:'Heart icon',numbers:'Readings',unknown:'Unknown',alarm:'Alerts',info:'Information',matrixLabel:'52 × 16 LED display',
  }
};

export function translate(language, key) { return translations[language]?.[key] ?? translations.de[key] ?? key; }
export function isDemo(status) { return status?.mode === 'demo' || status?.mode === 'simulation'; }
const integerInRange = (value, min, max) => Number.isInteger(value) && value >= min && value <= max;

/** Validate the complete public configuration; no passwords or tokens enter errors. */
export function validateConfig(config) {
  const errors = [];
  if (typeof config.email !== 'string' || config.email.length > 254 || (config.email && !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(config.email))) errors.push(['email','invalidEmail']);
  if (!['eu','world'].includes(config.region)) errors.push(['region','invalidRegion']);
  if (!['de','en'].includes(config.language)) errors.push(['language','invalidLanguage']);
  if (![5,10,15].includes(config.poll_interval_seconds)) errors.push(['poll-interval','invalidPoll']);
  if (!integerInRange(config.brightness,0,255)) errors.push(['brightness','invalidBrightness']);
  if (config.preview_brightness!==undefined&&!integerInRange(config.preview_brightness,0,255)) errors.push(['preview-brightness','invalidBrightness']);
  if (config.web_password!==undefined&&(typeof config.web_password!=='string'||config.web_password.length>128||/[\r\n]/.test(config.web_password))) errors.push(['web-password','invalidPassword']);
  if (typeof config.device_serial !== 'string' || config.device_serial.length > 64 || /[^a-zA-Z0-9_-]/.test(config.device_serial)) errors.push(['device-serial','invalidDevice']);
  if (config.password !== undefined && (typeof config.password !== 'string' || config.password.length > 512)) errors.push(['password','invalidPassword']);
  if (typeof config.clear_password !== 'boolean') errors.push(['clear-password','invalidFlags']);
  for (const key of PALETTE_KEYS) if (!/^#[0-9a-f]{6}$/i.test(config.palette?.[key] ?? '')) errors.push([`color-${key}`,'invalidPalette']);
  const alarms = config.alarms ?? {};
  for (const key of ['enabled','sound_enabled']) if (typeof alarms[key] !== 'boolean') errors.push([key === 'enabled' ? 'alarms-enabled' : 'sound-enabled','invalidFlags']);
  for (const [key,[min,max]] of Object.entries(ALARM_LIMITS)) if (!integerInRange(alarms[key],min,max)) errors.push([key.replaceAll('_','-'),'invalidAlarm']);
  if (alarms.heart_rate_min >= alarms.heart_rate_max) errors.push(['heart-rate-min','invalidHeartRange']);
  return errors;
}

/** Return only documented writable fields; empty passwords are omitted. */
export function prepareConfig(raw) {
  const output = {email:String(raw.email ?? '').trim(),region:raw.region,language:raw.language,device_serial:String(raw.device_serial ?? '').trim(),poll_interval_seconds:Number(raw.poll_interval_seconds),brightness:Number(raw.brightness),palette:{},alarms:{},clear_password:raw.clear_password === true};
  for (const key of PALETTE_KEYS) output.palette[key] = raw.palette?.[key];
  for (const key of ['enabled','sound_enabled']) output.alarms[key] = raw.alarms?.[key] === true;
  for (const key of Object.keys(ALARM_LIMITS)) output.alarms[key] = Number(raw.alarms?.[key]);
  if (typeof raw.password === 'string' && raw.password.length > 0) {output.password=raw.password;output.clear_password=false;}
  if(raw.preview_brightness!==undefined)output.preview_brightness=Number(raw.preview_brightness);
  if(raw.clear_web_password===true)output.clear_web_password=true;
  if(typeof raw.web_password==='string'&&raw.web_password.length){output.web_password=raw.web_password;output.clear_web_password=false;}
  return output;
}

export function visibleReadings(status) {
  // Never surface stale cloud values, even if an older server included them.
  if (!isDemo(status) && status?.cloud_fresh !== true) return null;
  return status?.readings && typeof status.readings === 'object' ? status.readings : null;
}

export function safePixels(display) {
  if (display?.width !== 52 || display?.height !== 16 || !Array.isArray(display.pixels) || display.pixels.length !== 832) return null;
  return display.pixels.map(pixel => typeof pixel === 'string' && /^#[0-9a-f]{6}$/i.test(pixel) ? pixel : '#000000');
}

export function scaleColor(color, brightness = 255) {
  const factor=Number.isFinite(brightness)?Math.max(0,Math.min(255,brightness))/255:0;
  return '#'+[1,3,5].map(offset=>Math.round(parseInt(color.slice(offset,offset+2),16)*factor).toString(16).padStart(2,'0')).join('');
}

export function requestHeaders(method = 'GET') {
  const headers = {};
  if (method !== 'GET') {headers['Content-Type']='application/json';headers['X-Owlanzi-Request']='1';}
  return headers;
}


// Shared TC001 navigation and wording, adapted for the native TC002.
Object.assign(translations.de,{"tab0": "Status", "tab1": "Display", "tab2": "Alarme", "tab3": "System", "langEn": "EN", "langDe": "DE", "brandNote": "Dein Begleiter am Bett", "yourDevice": "Deine Owlanzi", "liveDisplay": "Live-Anzeige", "liveMirror": "Die Matrix im Blick", "mirrorNote": "Genau das, was deine Uhr gerade zeigt.", "simulateBrightness": "Helligkeit simulieren", "previewActive": "Testvorschau mit Beispielwerten · endet automatisch nach 10 Sekunden.", "backNormal": "Zur normalen Anzeige", "acknowledge": "Alarm bestätigen", "heartRate": "Puls", "oxygen": "Sauerstoff", "sockBattery": "Sockenakku", "screenLabel": "Anzeige", "reasonLabel": "Grund", "brightness": "Helligkeit", "showingTitle": "Was die Uhr anzeigt", "sleepLabel": "Schlafstatus", "chargingLabel": "Lädt", "removedLabel": "Abgenommen", "baseLabel": "Basisstation an", "hardwareLabel": "Hardware", "sockTitle": "Socke", "owletLabel": "Owlet", "lastFetch": "Letzter Abruf", "fetchCount": "Abrufe / Fehler", "uptimeLabel": "Laufzeit", "errorLabel": "Letzter Fehler", "connectionTitle": "Verbindung", "normalBrightness": "Normale Helligkeit", "alarmBrightness": "Bei einem Alarm", "manualBrightness": "Die TC002 regelt die Helligkeit über den Drehknopf oder diese Einstellung. 0 schaltet die Matrix dunkel.", "brightnessTitle": "Helligkeit", "samplePreview": "Vorschau mit Beispielwerten. Farbanpassungen erscheinen kurz auf der Uhr; gespeichert wird erst mit „Display speichern“.", "color_heart": "Herz", "color_numbers": "Zahlen", "vitalsColors": "Messwerte", "color_awake": "Wach", "color_light_sleep": "Leichter Schlaf", "color_deep_sleep": "Tiefschlaf", "color_unknown_sleep": "Unbekannt", "sleepColors": "Schlafanzeige", "color_battery_frame": "Umrandung", "color_battery_fill": "Füllung, normal", "color_battery_charge": "Füllung, lädt", "color_battery_mid": "Füllung, unter 40 %", "color_battery_low": "Füllung, unter 20 %", "color_battery": "Prozentzahl", "batteryColors": "Akku", "color_heart_wait": "Herz ohne Messwerte", "color_waiting": "Striche und Wartetext", "color_offline": "Offline", "waitingColors": "Warten und Offline", "color_alarm": "Kritischer Alarm", "color_info": "Hinweis", "alarmColors": "Alarmfarben", "previewHelp": "Beispielanzeigen für zehn Sekunden. Echte kritische Alarme haben Vorrang. Die gespeicherten Einstellungen bleiben bis zum Speichern erhalten.", "test_vitals": "Messwerte", "test_charging": "Akku", "test_waiting": "Warten", "test_offline": "Offline", "test_alarm": "Alarm", "test_info": "Hinweis", "previewBrightness": "Helligkeit während der Vorschau", "previewTitle": "Vorschau auf der Uhr", "panelHelp": "Ecken: oben links rot, oben rechts grün, unten links blau, unten rechts weiß. Lauflicht prüft alle 832 LEDs. Farben füllt die gesamte 52 × 16 Matrix.", "test_corners": "Ecken", "test_chase": "Lauflicht", "test_colors": "Farben", "panelTitle": "Display prüfen", "resetColors": "Alle Farben zurücksetzen", "saveHint": "Änderungen werden auf deiner Uhr gespeichert.", "saveDisplay": "Display speichern", "ownWarning": "Eigene Alarme ergänzen die Owlet-Basisstation. Sie ersetzen sie nicht und sind standardmäßig ausgeschaltet.", "enableAlarms": "Eigene Alarme aktivieren", "oxygenLow": "Sauerstoff unter (%)", "duration": "Dauer (Sekunden)", "heartLow": "Puls unter (bpm)", "heartHigh": "Puls über (bpm)", "ownAlarmsTitle": "Eigene Alarme", "soundEnabled": "Ton bei Alarmen aktivieren", "volume": "Lautstärke (0–6)", "repeatSeconds": "Wiederholen nach Sekunden · 0 = einmal", "soundHelp": "Ein Tontest spielt hörbar über den Lautsprecher der TC002.", "soundTest": "Testton abspielen", "soundStop": "Ton stoppen / Alarm bestätigen", "soundTitle": "Ton", "saveAlarms": "Alarme speichern", "email": "E-Mail-Adresse", "password": "Passwort", "clearPassword": "Gespeichertes Owlet-Passwort entfernen", "deviceSelect": "Owlet-Gerät", "region": "Region", "pollInterval": "Abrufintervall", "accountHelp": "Deine Zugangsdaten bleiben auf der Uhr. Die TC002 ruft Owlet direkt ab; der Computer wird danach nicht benötigt.", "accountTitle": "Owlet-Konto", "languageSetting": "Oberfläche und Anzeige", "languageTitle": "Sprache", "accessHelp": "Standardmäßig ohne Anmeldung. Du kannst optional ein Web-Passwort setzen; der Benutzername ist „owlanzi“.", "webPassword": "Optionales Web-Passwort", "clearWebPassword": "Web-Passwort entfernen", "accessTitle": "Zugriff auf die Oberfläche", "versionLabel": "App-Version", "displayLabel": "Display", "addressLabel": "Lokale Adresse", "controlsHelp": "Drehknopf: Helligkeit. Knopf drücken oder mittlere Taste: Alarm bestätigen. Links / rechts: Helligkeit ändern.", "wifiHelp": "Diese Testversion nutzt das bereits auf der Uhr eingerichtete WLAN. Die WLAN-Einrichtung bleibt vorerst in der Herstelleranwendung.", "installHelp": "Lokale Testinstallation: Nach einem vollständigen Neustart startet wieder die Hersteller-App. Einstellungen bleiben erhalten. Dauerinstallation und Online-Updates folgen nach den Gerätetests.", "tc002Title": "TC002 · Gerät und Installation", "resetHelp": "Entfernt ausschließlich deine Owlanzi-Einstellungen und Owlet-Zugangsdaten. Das WLAN und die Hersteller-App bleiben erhalten.", "resetSettings": "Owlanzi-Einstellungen zurücksetzen", "resetTitle": "Owlanzi zurücksetzen", "saveSystem": "System speichern", "intro0": "Verbindung, Messwerte und der aktuelle Zustand deiner Uhr.", "intro1": "Helligkeit, Farben und Vorschauen auf deiner TC002.", "intro2": "Eigene Grenzwerte und Töne für deine Uhr.", "intro3": "Owlet-Konto, Sprache und Geräteeinstellungen.", "yes": "Ja", "no": "Nein", "noError": "Keiner", "saving": "Speichern …", "saved": "Gespeichert.", "resetConfirm": "Alle Owlanzi-Einstellungen und Zugangsdaten löschen?", "autoDevice": "Automatisch · bei einem Gerät", "openAccess": "Ohne Passwort geöffnet.", "protectedAccess": "Web-Passwort gesetzt. Leer lassen, um es beizubehalten.", "loginRequired": "Bitte die Seite neu laden und mit deinem Web-Passwort anmelden.", "resetColorLabel": "Farbe zurücksetzen", "screen_vitals": "Messwerte", "screen_battery": "Akku", "screen_waiting": "Warten", "screen_offline": "Offline", "screen_alarm": "Alarm", "screen_test": "Testvorschau", "screen_setup": "Einrichtung", "screen_message": "Nachricht", "showPassword": "Passwort anzeigen", "hidePassword": "Passwort verbergen"});
Object.assign(translations.en,{"tab0": "Status", "tab1": "Display", "tab2": "Alarms", "tab3": "System", "langEn": "EN", "langDe": "DE", "brandNote": "Your bedside companion", "yourDevice": "Your Owlanzi", "liveDisplay": "Live display", "liveMirror": "Live mirror of the matrix", "mirrorNote": "Exactly what your clock is showing.", "simulateBrightness": "Simulate brightness", "previewActive": "Test preview with sample values · ends automatically after 10 seconds.", "backNormal": "Back to normal", "acknowledge": "Acknowledge alarm", "heartRate": "Heart rate", "oxygen": "Oxygen", "sockBattery": "Sock battery", "screenLabel": "Screen", "reasonLabel": "Reason", "brightness": "Brightness", "showingTitle": "What the clock is showing", "sleepLabel": "Sleep state", "chargingLabel": "Charging", "removedLabel": "Removed", "baseLabel": "Base station on", "hardwareLabel": "Hardware", "sockTitle": "Sock", "owletLabel": "Owlet", "lastFetch": "Last fetch", "fetchCount": "Fetches / failures", "uptimeLabel": "Uptime", "errorLabel": "Last error", "connectionTitle": "Connection", "normalBrightness": "Normal brightness", "alarmBrightness": "During an alarm", "manualBrightness": "Set TC002 brightness using the knob or this control. 0 makes the matrix dark.", "brightnessTitle": "Brightness", "samplePreview": "Preview with sample values. Color edits appear briefly on the clock; use Save display to keep them.", "color_heart": "Heart", "color_numbers": "Numbers", "vitalsColors": "Vitals", "color_awake": "Awake", "color_light_sleep": "Light sleep", "color_deep_sleep": "Deep sleep", "color_unknown_sleep": "Unknown", "sleepColors": "Sleep indicator", "color_battery_frame": "Outline", "color_battery_fill": "Fill, normal", "color_battery_charge": "Fill, charging", "color_battery_mid": "Fill, below 40%", "color_battery_low": "Fill, below 20%", "color_battery": "Percentage", "batteryColors": "Battery", "color_heart_wait": "Heart without readings", "color_waiting": "Dashes and waiting text", "color_offline": "Offline", "waitingColors": "Waiting and offline", "color_alarm": "Critical alarm", "color_info": "Notice", "alarmColors": "Alarm colors", "previewHelp": "Sample screens for ten seconds. Real critical alerts take priority. Saved settings remain unchanged until you save.", "test_vitals": "Vitals", "test_charging": "Battery", "test_waiting": "Waiting", "test_offline": "Offline", "test_alarm": "Alarm", "test_info": "Notice", "previewBrightness": "Brightness while previewing", "previewTitle": "Preview on the clock", "panelHelp": "Corners: red top left, green top right, blue bottom left, white bottom right. Chase checks all 832 LEDs. Colors fills the complete 52 × 16 matrix.", "test_corners": "Corners", "test_chase": "Chase", "test_colors": "Colors", "panelTitle": "Check the panel", "resetColors": "Reset all colors", "saveHint": "Changes are saved on your clock.", "saveDisplay": "Save display", "ownWarning": "Custom alerts complement the Owlet base station. They do not replace it and are off by default.", "enableAlarms": "Enable custom alerts", "oxygenLow": "Oxygen below (%)", "duration": "Duration (seconds)", "heartLow": "Heart rate below (bpm)", "heartHigh": "Heart rate above (bpm)", "ownAlarmsTitle": "Custom alerts", "soundEnabled": "Enable alert sound", "volume": "Volume (0–6)", "repeatSeconds": "Repeat after seconds · 0 = once", "soundHelp": "A sound test plays audibly through the TC002 speaker.", "soundTest": "Play test tone", "soundStop": "Stop sound / acknowledge alert", "soundTitle": "Sound", "saveAlarms": "Save alarms", "email": "Email address", "password": "Password", "clearPassword": "Remove saved Owlet password", "deviceSelect": "Owlet device", "region": "Region", "pollInterval": "Fetch interval", "accountHelp": "Your credentials stay on the clock. TC002 fetches Owlet directly; your computer is not needed afterwards.", "accountTitle": "Owlet account", "languageSetting": "Interface and display", "languageTitle": "Language", "accessHelp": "No login by default. You can optionally set a web password; the username is “owlanzi”.", "webPassword": "Optional web password", "clearWebPassword": "Remove web password", "accessTitle": "Access to this interface", "versionLabel": "App version", "displayLabel": "Display", "addressLabel": "Local address", "controlsHelp": "Turn the knob: brightness. Press the knob or middle button: acknowledge an alert. Left / right: adjust brightness.", "wifiHelp": "This test version uses Wi-Fi already configured on the clock. Wi-Fi setup remains in the manufacturer application for now.", "installHelp": "Local test installation: a full reboot returns to the manufacturer app. Settings are preserved. Permanent installation and online updates follow after device testing.", "tc002Title": "TC002 · Device and installation", "resetHelp": "Removes only your Owlanzi settings and Owlet credentials. Wi-Fi and the manufacturer app are kept.", "resetSettings": "Reset Owlanzi settings", "resetTitle": "Reset Owlanzi", "saveSystem": "Save system", "intro0": "Connection, readings and the current state of your clock.", "intro1": "Brightness, colors and previews on your TC002.", "intro2": "Custom thresholds and sounds for your clock.", "intro3": "Owlet account, language and device settings.", "yes": "Yes", "no": "No", "noError": "None", "saving": "Saving …", "saved": "Saved.", "resetConfirm": "Delete all Owlanzi settings and credentials?", "autoDevice": "Automatic · for one device", "openAccess": "Open without a password.", "protectedAccess": "Web password set. Leave blank to keep it.", "loginRequired": "Reload the page and sign in with your web password.", "resetColorLabel": "Reset color", "screen_vitals": "Vitals", "screen_battery": "Battery", "screen_waiting": "Waiting", "screen_offline": "Offline", "screen_alarm": "Alert", "screen_test": "Test preview", "screen_setup": "Setup", "screen_message": "Message", "showPassword": "Show password", "hidePassword": "Hide password"});

Object.assign(translations.de,{"communityTitle": "Hilfe und Community", "communityText": "Fragen zur Einrichtung oder Ideen für Owlanzi? Tausche dich auf Discord mit uns aus.", "communityLink": "Zum Discord ↗", "supportTitle": "Gefällt dir Owlanzi?", "supportText": "Du kannst die Entwicklung mit einem Kaffee oder Bier auf Ko-fi unterstützen. Freiwillig – alle Funktionen bleiben kostenlos.", "supportLink": "Owlanzi unterstützen ↗"});
Object.assign(translations.en,{"communityTitle": "Help and community", "communityText": "Questions about setup or ideas for Owlanzi? Join us on Discord.", "communityLink": "Join Discord ↗", "supportTitle": "Enjoying Owlanzi?", "supportText": "You can support development with a coffee or beer on Ko-fi. Entirely optional – all features remain free.", "supportLink": "Support Owlanzi ↗"});
Object.assign(translations.de,{loadingSettings:'Einstellungen werden geladen …',settingsFailed:'Einstellungen konnten nicht geladen werden. Der nächste Versuch erfolgt automatisch.',retrySettings:'Erneut versuchen'});
Object.assign(translations.en,{loadingSettings:'Loading settings …',settingsFailed:'Settings could not be loaded. Retrying automatically.',retrySettings:'Try again'});
Object.assign(translations.de,{"color_numbers": "Pulswert", "color_oxygen_label": "O2-Beschriftung", "color_oxygen": "Sauerstoffwert und %", "color_charging_text": "Text „LAEDT“", "color_battery_status": "Text „SOCKE AUS“", "color_waiting": "Platzhalter „--“ / „--%“", "color_waiting_text": "Text „WARTE“", "color_reconnect_text": "Text „VERBINDE“", "color_setup_title": "Einrichtung: „OWLANZI“", "color_info": "Hinweise und Einrichtungstext", "color_awake": "Wach · Text", "color_light_sleep": "Leichtschlaf · Text", "color_deep_sleep": "Tiefschlaf · Text", "color_unknown_sleep": "Unbekannt · Fragezeichen", "vitalsColors": "Puls und Sauerstoff · obere Zeile", "sleepColors": "Schlafstatus · unten links", "batteryColors": "Akku und Ladeanzeige", "alarmColors": "Alarme, Hinweise und Einrichtung", "vitalsLayout": "Oben stehen das große Herz, der Pulswert und der Sauerstoffwert mit Prozentzeichen. Puls und Sauerstoff haben eigene Farben.", "sleepLayout": "Unten links steht der Schlafzustand mittig in seiner Displayhälfte. Wähle einen Zustand für die Vorschau.", "batteryLayout": "Das Akkusymbol und die Prozentzahl erscheinen unten rechts neben den Messwerten. Beim Laden oder bei ausgeschalteter Socke zeigt die Uhr die große Akkuanzeige.", "waitingLayout": "Ohne Messwerte erscheinen Herz, Platzhalter und WARTE. Bei fehlender Verbindung stehen OFFLINE und VERBINDE auf der Uhr.", "alarmLayout": "Die gewählte Alarm- oder Hinweisfarbe gilt für beide Textzeilen. In der Einrichtung lassen sich OWLANZI und LOCAL SETUP getrennt färben.", "sockOffPreview": "Socke aus · 64 %", "batteryMidPreview": "Akku · 35 %", "batteryLowPreview": "Akku · 15 %", "setupPreview": "Einrichtung"});
Object.assign(translations.en,{"color_numbers": "Heart rate value", "color_oxygen_label": "O2 label", "color_oxygen": "Oxygen value and %", "color_charging_text": "CHARGING text", "color_battery_status": "SOCK OFF text", "color_waiting": "-- / --% placeholders", "color_waiting_text": "WAITING text", "color_reconnect_text": "RETRYING text", "color_setup_title": "Setup: OWLANZI", "color_info": "Notices and setup text", "color_awake": "Awake · text", "color_light_sleep": "Light sleep · text", "color_deep_sleep": "Deep sleep · text", "color_unknown_sleep": "Unknown · question mark", "vitalsColors": "Heart rate and oxygen · top row", "sleepColors": "Sleep state · bottom left", "batteryColors": "Battery and charging screen", "alarmColors": "Alarms, notices and setup", "vitalsLayout": "The top row shows the large heart, heart rate and oxygen percentage. Heart rate and oxygen have separate colors.", "sleepLayout": "The sleep state is centered in the bottom left half of the display. Select a state to preview it.", "batteryLayout": "The battery icon and percentage appear at the bottom right beside live readings. While charging or with the sock switched off, the clock shows the large battery screen.", "waitingLayout": "Without readings, the clock shows the heart, placeholders and WAITING. When disconnected, it shows OFFLINE and RETRYING.", "alarmLayout": "The selected alarm or notice color applies to both text rows. In setup, OWLANZI and LOCAL SETUP have separate colors.", "sockOffPreview": "Sock off · 64%", "batteryMidPreview": "Battery · 35%", "batteryLowPreview": "Battery · 15%", "setupPreview": "Setup"});
Object.assign(translations.de,{"wifiTitle": "1 · WLAN verbinden", "wifiIntro": "Wähle dein WLAN und gib das WLAN-Passwort ein. Danach kannst du das Owlet-Konto einrichten.", "wifiHotspotNote": "Du bist im Einrichtungs-WLAN „owlanzi“. Öffne bei Bedarf 192.168.4.1. Beim Verbinden endet dieser Hotspot kurzzeitig.", "wifiNetworks": "Gefundene Netzwerke", "wifiManual": "Netzwerk wählen oder Namen eingeben", "wifiScan": "Netzwerke suchen", "wifiSsid": "WLAN-Name (SSID)", "wifiPassword": "WLAN-Passwort", "wifiPasswordHelp": "Bei einem offenen Netzwerk leer lassen. WLAN-Zugangsdaten werden getrennt vom Owlet-Konto gespeichert.", "wifiConnect": "WLAN verbinden", "wifiSetupAgain": "WLAN neu einrichten", "wifiSetupHelp": "Der Hotspot „owlanzi“ wird für 5 Minuten geöffnet. Die Uhr trennt dafür das Heim-WLAN. Alternativ: mittlere Taste 6 Sekunden halten und loslassen.", "wifiOpenHotspot": "Einrichtungs-Hotspot öffnen", "wifiCancel": "Zur bisherigen Verbindung", "wifiScanCached": "Die Liste stammt vom Start des Hotspots. Im Hotspot-Modus ist keine neue Suche möglich; du kannst den WLAN-Namen auch direkt eingeben.", "wifiSupported": "Offen oder WPA/WPA2-Personal. Unternehmens-WLAN und reines WPA3 werden nicht unterstützt.", "wifiUnsupported": "WLAN-Einrichtung ist in dieser Umgebung nicht verfügbar.", "wifi_start_failed": "WLAN konnte nicht gestartet werden.", "wifi_join_failed": "Verbindung fehlgeschlagen. Bitte WLAN-Name und Passwort prüfen. Die vorherige Verbindung wurde wiederhergestellt; falls das nicht möglich ist, erneut mit „owlanzi“ verbinden.", "wifi_operation_failed": "WLAN-Aktion fehlgeschlagen. Die Uhr versucht die bisherige Verbindung wiederherzustellen.", "wifiPhase_starting": "WLAN wird gestartet …", "wifiPhase_reconnecting": "WLAN-Verbindung wird hergestellt …", "wifiPhase_connected": "WLAN verbunden", "wifiPhase_joining": "Neue WLAN-Verbindung wird geprüft …", "wifiPhase_opening_hotspot": "Einrichtungs-Hotspot wird geöffnet …", "wifiPhase_hotspot": "Einrichtungs-Hotspot aktiv", "wifiPhase_unsupported": "WLAN nicht verfügbar", "wifiPhase_error": "WLAN-Fehler", "wifiJoinStarted": "Die Uhr verbindet sich jetzt. Wechsle danach am Handy zurück ins Heim-WLAN. Die neue IP-Adresse erscheint auf der Uhr. Bei einem Fehler erneut mit „owlanzi“ verbinden.", "wifiHotspotStarted": "Verbinde dein Handy jetzt mit dem offenen WLAN „owlanzi“ und öffne 192.168.4.1.", "wifiScanStarted": "Netzwerke werden gesucht …", "wifiInvalid": "Bitte WLAN-Namen und ein Passwort mit 8–63 Zeichen eingeben; für offene Netzwerke bleibt das Passwort leer.", "wifiOpen": "offen", "wifiUnavailableNetwork": "nicht unterstützt", "screen_wifi": "WLAN-Einrichtung", "wifiHelp": "WLAN direkt oben einrichten. Ohne Verbindung öffnet Owlanzi seinen eigenen Hotspot.", "accountTitle": "2 · Owlet-Konto"});
Object.assign(translations.en,{"wifiTitle": "1 · Connect Wi-Fi", "wifiIntro": "Select your Wi-Fi and enter its password. Then set up your Owlet account.", "wifiHotspotNote": "You are using the “owlanzi” setup Wi-Fi. If needed, open 192.168.4.1. This hotspot briefly closes while connecting.", "wifiNetworks": "Available networks", "wifiManual": "Select a network or enter its name", "wifiScan": "Find networks", "wifiSsid": "Wi-Fi name (SSID)", "wifiPassword": "Wi-Fi password", "wifiPasswordHelp": "Leave blank for an open network. Wi-Fi credentials are stored separately from your Owlet account.", "wifiConnect": "Connect Wi-Fi", "wifiSetupAgain": "Set up Wi-Fi again", "wifiSetupHelp": "Opens the “owlanzi” hotspot for 5 minutes and disconnects the clock from home Wi-Fi. Alternatively, hold the middle button for 6 seconds, then release.", "wifiOpenHotspot": "Open setup hotspot", "wifiCancel": "Return to previous connection", "wifiScanCached": "This list was collected when the hotspot started. A new scan is unavailable in hotspot mode; you can also enter the network name directly.", "wifiSupported": "Open or WPA/WPA2-Personal. Enterprise Wi-Fi and WPA3-only are unsupported.", "wifiUnsupported": "Wi-Fi setup is not available in this environment.", "wifi_start_failed": "Could not start Wi-Fi.", "wifi_join_failed": "Connection failed. Check the Wi-Fi name and password. The previous connection was restored; if unavailable, reconnect to “owlanzi”.", "wifi_operation_failed": "Wi-Fi action failed. The clock is trying to restore the previous connection.", "wifiPhase_starting": "Starting Wi-Fi …", "wifiPhase_reconnecting": "Connecting to Wi-Fi …", "wifiPhase_connected": "Wi-Fi connected", "wifiPhase_joining": "Checking the new Wi-Fi connection …", "wifiPhase_opening_hotspot": "Opening setup hotspot …", "wifiPhase_hotspot": "Setup hotspot active", "wifiPhase_unsupported": "Wi-Fi unavailable", "wifiPhase_error": "Wi-Fi error", "wifiJoinStarted": "The clock is connecting. Switch your phone back to home Wi-Fi. The clock will display its new IP address. On failure, reconnect to “owlanzi”.", "wifiHotspotStarted": "Connect your phone to the open “owlanzi” Wi-Fi, then open 192.168.4.1.", "wifiScanStarted": "Searching for networks …", "wifiInvalid": "Enter a Wi-Fi name and a password of 8–63 characters; leave the password blank for an open network.", "wifiOpen": "open", "wifiUnavailableNetwork": "unsupported", "screen_wifi": "Wi-Fi setup", "wifiHelp": "Set up Wi-Fi above. Without a connection, Owlanzi opens its own setup hotspot.", "accountTitle": "2 · Owlet account"});
Object.assign(translations.de,{wifiScanDone:'Suche abgeschlossen. Wähle ein Netzwerk aus der Liste.'});
Object.assign(translations.en,{wifiScanDone:'Search complete. Select a network from the list.'});
Object.assign(translations.de,{"color_clock": "Uhrzeit unten rechts", "timeTitle": "Uhrzeit", "timeAutomatic": "Uhrzeit automatisch aus dem Internet", "timeZone": "Zeitzone", "timeDetect": "Zeitzone vom Handy übernehmen", "timeHelp": "Sommer- und Winterzeit folgen automatisch der gewählten Zeitzone. Die Uhr läuft selbstständig weiter.", "timeLocal": "Datum und Uhrzeit in dieser Zeitzone", "timePhone": "Aktuelle Uhrzeit vom Handy übernehmen", "timeManualHelp": "Ändert die Anzeigezeit. Die Zeitstempel und Altersprüfung der Owlet-Messwerte bleiben unabhängig davon.", "timeSave": "Uhrzeit speichern", "timeDetected": "Zeitzone übernommen. Zum Anwenden speichern.", "timeUnsupported": "Diese Zeitzone ist nicht verfügbar. Bitte eine aus der Liste wählen.", "time_sync_failed": "Zeitsynchronisation fehlgeschlagen; die Uhr läuft mit der zuletzt bekannten Zeit weiter.", "timeSource_ntp": "Automatisch synchronisiert", "timeSource_device": "Gerätezeit · wartet auf Synchronisation", "timeSource_demo": "Simulierte Gerätezeit", "timeSource_manual": "Manuell eingestellt", "setupFinish": "WLAN und Owlet speichern · verbinden", "setupNext": "Weiter zum Owlet-Konto", "setupHelp": "WLAN und Owlet-Zugangsdaten eingeben, dann gemeinsam speichern. Die Uhr verbindet sich anschließend selbstständig.", "setupSaved": "WLAN und Owlet sind gespeichert. Die Uhr verbindet sich. Wechsle dein Handy zurück ins Heim-WLAN und öffne die IP-Adresse auf der Uhr.", "batteryLayout": "Beim Laden oder bei ausgeschalteter Socke zeigt die Uhr die große Akkuanzeige. Neben den Messwerten steht unten rechts die Uhrzeit.", "vitalsLayout": "Oben stehen das große Herz, der Pulswert und der Sauerstoffwert mit Prozentzeichen. Unten rechts erscheint die Uhrzeit in der gewählten Zeitzone."});
Object.assign(translations.en,{"color_clock": "Time · bottom right", "timeTitle": "Time", "timeAutomatic": "Set time automatically from the internet", "timeZone": "Time zone", "timeDetect": "Use this phone’s time zone", "timeHelp": "Daylight saving follows the selected time zone automatically. The clock keeps running independently.", "timeLocal": "Date and time in this time zone", "timePhone": "Use the phone’s current time", "timeManualHelp": "Changes the display time. Owlet measurement timestamps and freshness checks stay independent.", "timeSave": "Save time", "timeDetected": "Time zone selected. Save to apply.", "timeUnsupported": "This time zone is unavailable. Please select one from the list.", "time_sync_failed": "Time synchronization failed; the clock continues using its last known time.", "timeSource_ntp": "Automatically synchronized", "timeSource_device": "Device time · awaiting synchronization", "timeSource_demo": "Simulated device time", "timeSource_manual": "Manually set", "setupFinish": "Save Wi-Fi and Owlet · connect", "setupNext": "Continue to Owlet account", "setupHelp": "Enter Wi-Fi and Owlet credentials, then save both together. The clock connects independently afterwards.", "setupSaved": "Wi-Fi and Owlet are saved. The clock is connecting. Return your phone to home Wi-Fi and open the IP address shown on the clock.", "batteryLayout": "While charging or with the sock switched off, the clock shows the large battery screen. Beside live readings, the bottom right shows the time.", "vitalsLayout": "The top row shows the large heart, heart rate and oxygen percentage. The bottom right shows the time in the selected time zone."});
Object.assign(translations.de,{
 updateTitle:'App-Updates',updateLatest:'Verfügbare Version',updateDaily:'Täglich nach Updates suchen',updatePrivacy:'Die Uhr fragt owlanzi.com einmal täglich nach einer neuen Version. Modell und installierte App-Version werden für Tageszahlen übermittelt; Abrufe und Downloads werden ohne Gerätekennung gezählt. Installiert wird erst nach deiner Bestätigung.',updateSave:'Update-Einstellung speichern',updateKeep:'WLAN, Owlet-Zugang und alle Einstellungen bleiben erhalten. Während des App-Neustarts wird die Anzeige kurz unterbrochen.',updateCheck:'Jetzt suchen',updateInstall:'Update installieren',updateRollback:'Vorherige Version wiederherstellen',updateLoader:'Für die Installation muss der OTA-Starter einmal lokal eingerichtet werden.',updateConfirm:'Update installieren und die App kurz neu starten? Deine gespeicherten Einstellungen bleiben erhalten.',updateRollbackConfirm:'Zur vorherigen App-Version wechseln? Deine gespeicherten Einstellungen bleiben erhalten.',updateUnsaved:'Bitte deine Änderungen vor dem Update speichern.',updateError:'Update fehlgeschlagen. Die bisherige App bleibt erhalten. Bitte die Verbindung prüfen und erneut versuchen.',updateRestored:'Die neue App konnte nicht starten. Die vorherige Version wurde wiederhergestellt.',updateBlocked:'Updates sind bei einem kritischen Alarm oder ohne Heim-WLAN pausiert.',updatePhase_idle:'Noch nicht nach Updates gesucht.',updatePhase_unavailable:'Updates sind in dieser Umgebung nicht verfügbar.',updatePhase_queued:'Update-Aktion startet …',updatePhase_checking:'Suche nach Updates …',updatePhase_available:'Ein Update ist verfügbar.',updatePhase_current:'Deine App ist aktuell.',updatePhase_downloading:'Update wird geladen und geprüft …',updatePhase_switching:'App wird neu gestartet. Diese Seite verbindet sich wieder …',installHelp:'Lokale Testinstallation: Nach einem vollständigen Neustart startet wieder die Hersteller-App. Einstellungen und App-Updates bleiben gespeichert; Owlanzi muss dann noch lokal gestartet werden.'
});
Object.assign(translations.en,{
 updateTitle:'App updates',updateLatest:'Available version',updateDaily:'Check for updates daily',updatePrivacy:'The clock asks owlanzi.com for a new version once a day. The model and installed app version are sent for daily counts; checks and downloads are counted without a device identifier. Installation requires your confirmation.',updateSave:'Save update setting',updateKeep:'Wi-Fi, Owlet credentials and all settings are preserved. The display pauses briefly while the app restarts.',updateCheck:'Check now',updateInstall:'Install update',updateRollback:'Restore previous version',updateLoader:'The OTA starter needs to be installed locally once before updates can be installed.',updateConfirm:'Install the update and briefly restart the app? Your saved settings are preserved.',updateRollbackConfirm:'Switch to the previous app version? Your saved settings are preserved.',updateUnsaved:'Please save your changes before updating.',updateError:'Update failed. Your previous app is preserved. Check the connection and try again.',updateRestored:'The new app could not start. The previous version was restored.',updateBlocked:'Updates pause during a critical alarm or without home Wi-Fi.',updatePhase_idle:'No update check yet.',updatePhase_unavailable:'Updates are unavailable in this environment.',updatePhase_queued:'Starting update action …',updatePhase_checking:'Checking for updates …',updatePhase_available:'An update is available.',updatePhase_current:'Your app is up to date.',updatePhase_downloading:'Downloading and verifying update …',updatePhase_switching:'Restarting the app. This page will reconnect …',installHelp:'Local test installation: a full reboot returns to the manufacturer app. Settings and app updates remain stored; Owlanzi then needs to be started locally.'
});
export function updateControls(update={},status={}){
 const busy=['queued','checking','downloading','switching'].includes(update.phase),blocked=!!status.alarm?.critical||!status.wifi?.connected||!!status.wifi?.hotspot||!!status.wifi?.busy;
 return {busy,blocked,check:!busy&&!blocked&&update.phase!=='unavailable',install:!busy&&!blocked&&!!update.install_supported&&!!update.available,rollback:!busy&&!blocked&&!!update.install_supported&&!!update.rollback_available};
}
if(typeof document!=='undefined')startApplication();
function startApplication(){
 const $=id=>document.getElementById(id);
 let language='de',config=null,defaults=null,latest=null,tab=0,polling=false,loaded=false,deviceSignature='',previewRevision=0,previewTimer=0,currentMode='vitals',previewChain=Promise.resolve(),renderChain=Promise.resolve(),previewsReady=false,previewRendering=false;
 const dirty=new Set(),t=key=>translate(language,key);
 let wifiSignature='',wifiOnboardingShown=false,wifiScanRequested=false,timeDirty=false,updateDirty=false,updateInFlight=false;
 const notice=message=>{$('notice').textContent=message;$('notice').hidden=!message;};
 let toastTimer;
 function toast(message){$('toast').textContent=message;$('toast').classList.add('on');clearTimeout(toastTimer);toastTimer=setTimeout(()=>$('toast').classList.remove('on'),3500);}
 async function api(path,method='GET',body){
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),12000);
  try{
   const response=await fetch(path,{method,headers:requestHeaders(method),credentials:'same-origin',body:body===undefined?undefined:JSON.stringify(body),cache:'no-store',signal:controller.signal});
   if(response.status===401)throw new Error(t('loginRequired'));
   const value=await response.json();if(!response.ok)throw new Error(value.error||t('requestError'));return value;
  }catch(error){if(error.name==='AbortError'||error instanceof TypeError)throw new Error(t('networkError'));throw error;}
  finally{clearTimeout(timer);}
 }
 function saveButtons(){for(const b of document.querySelectorAll('[data-save]'))b.disabled=!loaded||!dirty.has(Number(b.dataset.save));}
 function markDirty(n){dirty.add(n);saveButtons();$('result-'+n).textContent='';}
 function setLanguage(next){
  language=next;document.documentElement.lang=language;$('language').value=language;
  for(const e of document.querySelectorAll('[data-i18n]'))e.textContent=t(e.dataset.i18n);
  for(const b of document.querySelectorAll('[data-language]'))b.classList.toggle('on',b.dataset.language===language);
  for(const b of document.querySelectorAll('[data-reset-color]'))b.setAttribute('aria-label',t('resetColorLabel')+': '+t('color_'+b.dataset.resetColor));
  for(const b of document.querySelectorAll('[data-eye]')){const label=t($(b.dataset.eye).type==='password'?'showPassword':'hidePassword');b.setAttribute('aria-label',label);b.title=label;}
  $('page-title').textContent=t('tab'+tab);$('page-intro').textContent=t('intro'+tab);
  if(config){$('password-help').textContent=t(config.has_password?'passwordSaved':'passwordEmpty');$('web-password-help').textContent=t(config.has_web_password?'protectedAccess':'openAccess');}
  $('region').options[0].textContent=t('regionEu');$('region').options[1].textContent=t('regionWorld');
  if(latest)renderStatus(latest);
 }
 function chooseTab(n,focus=false){
  tab=n;for(let i=0;i<4;i++){$('s'+i).classList.toggle('on',i===n);$('s'+i).hidden=i!==n;$('tab'+i).classList.toggle('on',i===n);$('tab'+i).setAttribute('aria-selected',String(i===n));$('tab'+i).tabIndex=i===n?0:-1;}
  $('page-title').textContent=t('tab'+n);$('page-intro').textContent=t('intro'+n);if(focus)$('tab'+n).focus();
  if(n===1&&loaded)renderPreviews();
  window.scrollTo(0,0);
 }
 function updateOutputs(){for(const e of document.querySelectorAll('input[type=range]'))$(e.id+'-output').textContent=e.value;}
 function setDeviceOptions(devices,selected){
  const signature=JSON.stringify(devices)+language;if(signature===deviceSignature&&$('device-serial').value===selected)return;
  deviceSignature=signature;const select=$('device-serial');select.replaceChildren();
  const auto=document.createElement('option');auto.value='';auto.textContent=t('autoDevice');select.append(auto);
  for(const d of devices){if(typeof d.serial!=='string')continue;const o=document.createElement('option');o.value=d.serial;o.textContent=(d.name||'Owlet')+' · '+d.serial;select.append(o);}
  if(selected&&![...select.options].some(o=>o.value===selected)){const o=document.createElement('option');o.value=selected;o.textContent=selected;select.append(o);}
  select.value=selected;
 }
 function applyConfig(next,scope=null){
  config=next;
  if(scope===null||scope===1){$('brightness').value=next.brightness;$('preview-brightness').value=next.preview_brightness??defaults.preview_brightness;$('alarm-brightness').value=next.alarms.alarm_brightness;for(const k of PALETTE_KEYS)$('color-'+k).value=next.palette[k]??defaults?.palette[k]??'#000000';}
  if(scope===null||scope===2){const a=next.alarms;$('alarms-enabled').checked=a.enabled;$('sound-enabled').checked=a.sound_enabled;for(const k of Object.keys(ALARM_LIMITS))if(k!=='alarm_brightness')$(k.replaceAll('_','-')).value=a[k];$('alarm-fields').disabled=!a.enabled;}
  if(scope===null||scope===3){$('email').value=next.email;$('region').value=next.region;$('poll-interval').value=next.poll_interval_seconds;setDeviceOptions(latest?.devices??[],next.device_serial);$('password').value='';$('web-password').value='';$('clear-password').checked=false;$('clear-web-password').checked=false;setLanguage(next.language);}
  if(scope===null){$('update-daily').checked=next.auto_update_check!==false;$('time-automatic').checked=next.time.automatic;$('time-zone').value=next.time.zone;$('time-manual-fields').hidden=next.time.automatic;}
  updateOutputs();if(scope!==null)dirty.delete(scope);saveButtons();
 }
 function form(){
  const raw={email:$('email').value,password:$('password').value,clear_password:$('clear-password').checked,web_password:$('web-password').value,clear_web_password:$('clear-web-password').checked,region:$('region').value,language,device_serial:$('device-serial').value,poll_interval_seconds:$('poll-interval').value,brightness:$('brightness').value,preview_brightness:$('preview-brightness').value,palette:{},alarms:{enabled:$('alarms-enabled').checked,sound_enabled:$('sound-enabled').checked}};
  for(const k of PALETTE_KEYS)raw.palette[k]=$('color-'+k).value;
  for(const k of Object.keys(ALARM_LIMITS))raw.alarms[k]=$(k.replaceAll('_','-')).value;
  return prepareConfig(raw);
 }
 function draw(canvas,display,simulate=false){
  const ctx=canvas.getContext('2d'),pixels=safePixels(display);ctx.fillStyle='#05060a';ctx.fillRect(0,0,canvas.width,canvas.height);
  for(let y=0;y<16;y++)for(let x=0;x<52;x++){ctx.fillStyle=pixels?scaleColor(pixels[y*52+x],simulate?display.brightness:255):'#11151b';ctx.fillRect(x*12+1,y*12+1,9,9);}
 }
 function renderStatus(s){
  latest=s;const readings=visibleReadings(s),num=v=>typeof v==='number'&&Number.isFinite(v)?Math.round(v):'—',bool=v=>typeof v==='boolean'?t(v?'yes':'no'):'—';
  $('heart-value').textContent=num(readings?.heart_rate);$('oxygen-value').textContent=num(readings?.oxygen);$('battery-value').textContent=num(readings?.battery);
  $('sleep').textContent=readings?.sleep_status?t(readings.sleep_status):'—';$('charging').textContent=bool(s.sock?.charging);$('removed').textContent=bool(s.sock?.removed);$('base-on').textContent=bool(s.sock?.base_on);$('hardware').textContent=s.sock?.hardware||'—';
  $('screen').textContent=t('screen_'+s.display.screen);$('reason').textContent=s.state_message||'—';$('actual-brightness').textContent=s.display.brightness+' / 255';
  $('connection-badge').textContent=t('screen_'+s.display.screen);$('connection-badge').className='pill '+(s.alarm?.critical?'bad':s.cloud_fresh?'ok':'warn');
  $('owlet-state').textContent=t(s.cloud_fresh?'connected':'disconnected');$('owlet-state').className=s.cloud_fresh?'good':'bad';
  $('last-error').textContent=s.last_error||t('noError');$('last-error').className=s.last_error?'status-error':'';
  $('last-updated').textContent=s.last_updated?new Date(s.last_updated).toLocaleTimeString(language==='de'?'de-DE':'en-GB'):'—';
  $('fetch-count').textContent=s.poll_count+' / '+(s.failure_count??0);$('uptime').textContent=Math.floor((s.uptime_seconds??0)/3600)+' h '+Math.floor((s.uptime_seconds??0)%3600/60)+' min';
  $('alarm-box').hidden=!s.alarm.active;$('alarm-message').textContent=s.alarm.acknowledged?t('alarmAcknowledged'):s.alarm.message;$('acknowledge').hidden=s.alarm.acknowledged;
  $('device-version').textContent=s.version;$('system-version').textContent=s.version;$('system-display').textContent='52 × 16 · 832 RGB LEDs';$('system-address').textContent=location.host;
  $('preview-banner').hidden=!s.preview_active;$('mode-badge').textContent=s.mode==='demo'?t('demoNotice'):'';
  if(s.time){$('time-state').textContent=s.time.local.replace('T',' ')+' · '+t('timeSource_'+s.time.source)+(s.time.error?' · '+t(s.time.error):'');}
  renderUpdate(s);renderWifi(s.wifi??{supported:false});setDeviceOptions(s.devices??[],$('device-serial').value);draw($('mx'),s.display,$('simulate-brightness').checked);
 }
 function renderUpdate(s){
  const u=s.update??{phase:'unavailable'},c=updateControls(u,s);
  $('update-banner').hidden=!u.available||c.busy;
  $('update-current').textContent=u.current??s.version;$('update-latest').textContent=u.latest||'—';$('update-notes').textContent=u.notes||'';
  $('update-state').textContent=t(u.phase==='error'?'updateError':u.last_result==='rolled_back'?'updateRestored':c.blocked&&!c.busy?'updateBlocked':'updatePhase_'+u.phase);
  $('update-progress').hidden=u.phase!=='downloading';$('update-progress').value=u.size?Math.min(100,100*u.received/u.size):0;
  for(const action of ['check','install','rollback'])$('update-'+action).disabled=!loaded||!c[action];
  $('update-rollback').hidden=!u.rollback_available;$('update-loader').hidden=u.install_supported||u.phase==='unavailable';
  if(updateInFlight&&u.phase!=='switching'&&!c.busy&&s.version!==$('update-card').dataset.previousVersion&&!dirty.size&&!timeDirty&&!updateDirty)location.reload();
 }
 function renderWifi(w){
  $('wifi-state').textContent=w.supported?t('wifiPhase_'+w.phase)+(w.connected?' · '+w.ssid+' · '+w.ip:''):t('wifiUnsupported');
  $('wifi-hotspot-note').hidden=!w.hotspot;
  $('setup-finish').hidden=!w.hotspot;$('setup-finish').disabled=!loaded||w.busy;
  $('wifi-connect').textContent=t(w.hotspot?'setupNext':'wifiConnect');
  document.querySelector('[data-i18n=wifiIntro]').textContent=t('setupHelp');
  $('s3').querySelector('.savebar').hidden=!!w.hotspot;
  $('wifi-scan-note').textContent=t(w.scan_cached?'wifiScanCached':'wifiSupported');
  if(w.error)$('wifi-result').textContent=t(w.error);
  if(wifiScanRequested&&!w.scanning){wifiScanRequested=false;if(!w.error)$('wifi-result').textContent=t('wifiScanDone');}
  const busy=w.busy||w.phase==='joining'||w.phase==='opening_hotspot';
  for(const id of ['wifi-ssid','wifi-password','wifi-network','wifi-connect','wifi-hotspot'])$(id).disabled=!loaded||!w.supported||busy;
  $('wifi-scan').disabled=!loaded||!w.supported||busy||w.hotspot||w.scanning;$('wifi-cancel').disabled=!loaded||!w.supported||(!w.hotspot&&!busy);
  const signature=JSON.stringify(w.networks)+language;
  if(signature!==wifiSignature){
   wifiSignature=signature;const select=$('wifi-network'),selected=select.value;select.replaceChildren();
   const manual=document.createElement('option');manual.value='';manual.textContent=t('wifiManual');select.append(manual);
   for(const n of w.networks??[]){const option=document.createElement('option');option.value=n.ssid;option.textContent=n.ssid+' · '+n.rssi+' dBm'+(!n.secure?' · '+t('wifiOpen'):'')+(!n.supported?' · '+t('wifiUnavailableNetwork'):'');option.disabled=!n.supported;select.append(option);}
   select.value=selected;
  }
  if(w.hotspot&&!wifiOnboardingShown){wifiOnboardingShown=true;chooseTab(3);$('wifi-card').scrollIntoView({block:'start'});}
 }
 async function wifiAction(action){
  const body=action==='connect'?{ssid:$('wifi-ssid').value,password:$('wifi-password').value}:{};
  if(action==='connect'&&(!body.ssid||new TextEncoder().encode(body.ssid).length>32||(body.password&&(body.password.length<8||body.password.length>63)))){$('wifi-result').textContent=t('wifiInvalid');return;}
  if(action==='connect'&&latest?.wifi?.hotspot){$('account-title').closest('.card').scrollIntoView({block:'start'});$('email').focus({preventScroll:true});return;}
  try{
   await api('/api/wifi/'+action,'POST',body);
   if(action==='connect')$('wifi-password').value='';
   if(action==='scan')wifiScanRequested=true;
   $('wifi-result').textContent=t(action==='connect'?'wifiJoinStarted':action==='hotspot'?'wifiHotspotStarted':action==='scan'?'wifiScanStarted':'wifiPhase_reconnecting');
   await refresh();
  }catch(e){$('wifi-result').textContent=e.message;}
 }
 async function initializeSettings(){
  $('retry-settings').disabled=true;
  try{
   const zoneData=await api('/api/timezones');$('time-zone').replaceChildren(...zoneData.zones.map(zone=>{const o=document.createElement('option');o.value=zone;o.textContent=zone.replaceAll('_',' ');return o;}));
   // Load independently of the live status. A failed attempt must be retried,
   // never leave a working live display above permanently disabled forms.
   const next=await api('/api/config'),defs=await api('/api/defaults');
   if([next,defs].some(value=>validateConfig({...value,clear_password:false}).length))throw new Error(t('invalidConfig'));
   defaults=defs;applyConfig(next);
   for(const e of document.querySelectorAll('section input,section select,section button'))e.disabled=false;
   loaded=true;saveButtons();$('settings-loading').hidden=true;
   if(!next.has_password&&tab===0)chooseTab(3);
  }catch(e){
   $('settings-loading-message').textContent=t('settingsFailed')+' '+e.message;
   $('settings-loading').hidden=false;$('retry-settings').hidden=false;
  }finally{$('retry-settings').disabled=false;$('retry-settings').textContent=t('retrySettings');}
 }
 async function refresh(){
  if(polling||document.hidden)return;polling=true;
  try{
   if(!loaded)await initializeSettings();
   const s=await api('/api/status');renderStatus(s);
   if(loaded&&tab===1&&!previewsReady&&!previewRendering)renderPreviews();
  }
  catch(e){$('connection-badge').textContent=t('disconnected');$('connection-badge').className='pill bad';$('last-error').textContent=e.message;for(const id of ['heart-value','oxygen-value','battery-value','sleep','charging','removed','base-on'])$(id).textContent='—';}
  finally{polling=false;}
 }
 const modes={clock:'vitals',heart:'vitals',numbers:'vitals',oxygen:'vitals',awake:'sleep1',light_sleep:'sleep2',deep_sleep:'sleep3',unknown_sleep:'sleep0',battery_frame:'battery',battery_fill:'battery',battery_charge:'charging',charging_text:'charging',battery_status:'battery',battery_mid:'battery-mid',battery_low:'battery-low',battery:'battery',heart_wait:'waiting',waiting:'waiting',waiting_text:'waiting',offline:'offline',reconnect_text:'offline',alarm:'alarm',info:'info',setup_title:'setup'};
 const previewGroups={vitals:['vitals'],sleep2:['sleep0','sleep1','sleep2','sleep3'],charging:['battery','charging','battery-mid','battery-low'],waiting:['waiting','offline'],alarm:['alarm','info','setup']};
 const previewModes=Object.fromEntries(Object.keys(previewGroups).map(group=>[group,group]));
 function previewOptions(mode){return {mode,palette:form().palette,preview_brightness:Number($('preview-brightness').value),language};}
 function renderPreviews(changedMode){
  if(!loaded)return;const revision=++previewRevision;previewRendering=true;previewsReady=false;notice('');
  for(const [group,choices]of Object.entries(previewGroups))if(choices.includes(changedMode))previewModes[group]=changedMode;
  for(const button of document.querySelectorAll('[data-preview-mode]'))button.setAttribute('aria-pressed',String(previewModes[button.dataset.previewGroup]===button.dataset.previewMode));
  renderChain=renderChain.then(async()=>{
  if(revision!==previewRevision)return;
  try{for(const canvas of document.querySelectorAll('canvas[data-preview]')){
   if(revision!==previewRevision)return;
   const mode=previewModes[canvas.dataset.preview];
   const display=await api('/api/preview/render','POST',previewOptions(mode));if(revision===previewRevision)draw(canvas,display);
  }previewsReady=true;}catch(e){previewsReady=false;notice(e.message);}
  finally{if(revision===previewRevision)previewRendering=false;}
  });return renderChain;
 }
 function preview(mode){currentMode=mode;const options=previewOptions(mode);previewChain=previewChain.then(async()=>{try{await api('/api/preview','POST',options);await refresh();}catch(e){notice(e.message);}});return previewChain;}
 function schedulePreview(mode){currentMode=mode;clearTimeout(previewTimer);previewTimer=setTimeout(()=>{renderPreviews(mode);preview(mode);},240);}
 async function stopPreview(){clearTimeout(previewTimer);await previewChain;try{await api('/api/preview/stop','POST',{});await refresh();}catch(e){notice(e.message);}}
 async function save(n){
  const next=form();let patch;
  if(n===1)patch={brightness:next.brightness,preview_brightness:next.preview_brightness,palette:next.palette,alarms:{alarm_brightness:next.alarms.alarm_brightness}};
  else if(n===2){const {alarm_brightness,...alarms}=next.alarms;patch={alarms};}
  else{patch={...next};for(const key of ['brightness','preview_brightness','palette','alarms'])delete patch[key];}
  const candidate={...config,...patch,clear_password:patch.clear_password??false,alarms:{...config.alarms,...patch.alarms}};
  const errors=validateConfig(candidate);for(const e of document.querySelectorAll('[aria-invalid]'))e.removeAttribute('aria-invalid');
  if(errors.length){for(const [id]of errors)$(id)?.setAttribute('aria-invalid','true');$('result-'+n).textContent=t(errors[0][1]);$(errors[0][0])?.focus();return;}
  const b=document.querySelector('[data-save="'+n+'"]');b.disabled=true;clearTimeout(previewTimer);
  try{await previewChain;const result=await api('/api/config','POST',patch);applyConfig(result,n);$('result-'+n).textContent=t('saved');toast(t('saved'));if(patch.web_password||patch.clear_web_password){location.reload();return;}await refresh();}
  catch(e){$('result-'+n).textContent=e.message;notice(e.message);}
  finally{saveButtons();}
 }
 for(const b of document.querySelectorAll('[data-tab]')){b.addEventListener('click',()=>chooseTab(Number(b.dataset.tab)));b.addEventListener('keydown',e=>{const keys={ArrowRight:1,ArrowDown:1,ArrowLeft:-1,ArrowUp:-1};if(e.key in keys){e.preventDefault();chooseTab((tab+keys[e.key]+4)%4,true);}else if(e.key==='Home'||e.key==='End'){e.preventDefault();chooseTab(e.key==='Home'?0:3,true);}});}
 for(let n=1;n<4;n++)$('s'+n).addEventListener('input',e=>{if(e.target.closest('#wifi-card,#time-card,#update-card'))return;if(loaded)markDirty(n);updateOutputs();});
 for(const b of document.querySelectorAll('[data-save]'))b.addEventListener('click',()=>save(Number(b.dataset.save)));
 for(const b of document.querySelectorAll('[data-language]'))b.addEventListener('click',()=>{setLanguage(b.dataset.language);if(loaded)markDirty(3);});
 $('language').addEventListener('change',()=>{setLanguage($('language').value);markDirty(3);});
 $('alarms-enabled').addEventListener('change',()=>{$('alarm-fields').disabled=!$('alarms-enabled').checked;});
 for(const id of ['password','web-password']){
  const input=$(id),wrapper=document.createElement('div');wrapper.className='pw';input.before(wrapper);wrapper.append(input);
  const eye=document.createElement('button');eye.type='button';eye.dataset.eye=id;eye.setAttribute('aria-label',t('showPassword'));eye.className='eye-toggle';
  const svg=document.createElementNS('http://www.w3.org/2000/svg','svg');svg.setAttribute('viewBox','0 0 24 24');svg.setAttribute('aria-hidden','true');
  const path=document.createElementNS(svg.namespaceURI,'path');path.setAttribute('d','M1.7 12S5.3 5.6 12 5.6 22.3 12 22.3 12 18.7 18.4 12 18.4 1.7 12 1.7 12Z M15.1 12a3.1 3.1 0 1 1-6.2 0 3.1 3.1 0 0 1 6.2 0');svg.append(path);eye.append(svg);wrapper.append(eye);
  eye.addEventListener('click',()=>{input.type=input.type==='password'?'text':'password';const label=t(input.type==='password'?'showPassword':'hidePassword');eye.setAttribute('aria-label',label);eye.title=label;});
  input.addEventListener('input',()=>{if(input.value)$('clear-'+id).checked=false;});$('clear-'+id).addEventListener('change',()=>{if($('clear-'+id).checked)input.value='';});
 }
 for(const input of document.querySelectorAll('[data-color]'))input.addEventListener('input',()=>schedulePreview(modes[input.dataset.color]));
 for(const button of document.querySelectorAll('[data-preview-mode]'))button.addEventListener('click',()=>renderPreviews(button.dataset.previewMode));
 for(const b of document.querySelectorAll('[data-reset-color]'))b.addEventListener('click',()=>{const k=b.dataset.resetColor;$('color-'+k).value=defaults.palette[k];markDirty(1);schedulePreview(modes[k]);});
 $('reset-colors').addEventListener('click',()=>{for(const k of PALETTE_KEYS)$('color-'+k).value=defaults.palette[k];markDirty(1);schedulePreview(currentMode);});
 $('preview-brightness').addEventListener('input',()=>schedulePreview(currentMode));
 for(const b of document.querySelectorAll('[data-test]'))b.addEventListener('click',()=>preview(b.dataset.test));
 $('preview-stop').addEventListener('click',stopPreview);$('preview-stop-banner').addEventListener('click',stopPreview);
 $('acknowledge').addEventListener('click',async()=>{try{await api('/api/acknowledge','POST',{});await refresh();}catch(e){notice(e.message);}});
 $('sound-test').addEventListener('click',async()=>{try{await api('/api/sound','POST',{volume:Number($('volume').value)});toast(t('soundTest'));}catch(e){notice(e.message);}});
 $('sound-stop').addEventListener('click',async()=>{try{await api('/api/sound/stop','POST',{});await refresh();}catch(e){notice(e.message);}});
 $('reset-settings').addEventListener('click',async()=>{if(!confirm(t('resetConfirm')))return;try{await api('/api/reset','POST',{confirm:'RESET OWLANZI'});location.reload();}catch(e){notice(e.message);}});
 $('simulate-brightness').addEventListener('change',()=>{if(latest)draw($('mx'),latest.display,$('simulate-brightness').checked);});
 $('wifi-network').addEventListener('change',()=>{$('wifi-ssid').value=$('wifi-network').value;$('wifi-password').value='';});
 for(const action of ['scan','connect','hotspot','cancel'])$('wifi-'+action).addEventListener('click',()=>wifiAction(action));
 $('wifi-show').addEventListener('click',()=>{const input=$('wifi-password');input.type=input.type==='password'?'text':'password';$('wifi-show').textContent=t(input.type==='password'?'showPassword':'hidePassword');});

 function phoneTime(){
  const zone=$('time-zone').value,parts=new Intl.DateTimeFormat('sv-SE',{timeZone:zone,year:'numeric',month:'2-digit',day:'2-digit',hour:'2-digit',minute:'2-digit',hourCycle:'h23'}).formatToParts(new Date());
  const v=Object.fromEntries(parts.map(p=>[p.type,p.value]));return v.year+'-'+v.month+'-'+v.day+'T'+v.hour+':'+v.minute;
 }
 function timeChanged(){timeDirty=true;$('time-result').textContent='';}
 $('time-automatic').addEventListener('change',()=>{$('time-manual-fields').hidden=$('time-automatic').checked;if(!$('time-automatic').checked&&!$('time-local').value)$('time-local').value=(latest?.time?.local||phoneTime()).slice(0,16);timeChanged();});
 $('time-zone').addEventListener('change',timeChanged);$('time-local').addEventListener('input',timeChanged);
 $('time-detect').addEventListener('click',()=>{const zone=Intl.DateTimeFormat().resolvedOptions().timeZone;if([...$('time-zone').options].some(o=>o.value===zone)){$('time-zone').value=zone;timeChanged();$('time-result').textContent=t('timeDetected');}else $('time-result').textContent=t('timeUnsupported');});
 $('time-phone').addEventListener('click',()=>{$('time-local').value=phoneTime();timeChanged();});
 $('time-save').addEventListener('click',async()=>{
  $('time-save').disabled=true;
  try{const value={zone:$('time-zone').value,automatic:$('time-automatic').checked};if(!value.automatic){value.local=$('time-local').value;await api('/api/time/resolve','POST',value);}
   const next=await api('/api/config','POST',{time:value});config.time=next.time;timeDirty=false;$('time-result').textContent=t('saved');previewsReady=false;await refresh();
  }catch(e){$('time-result').textContent=e.message;}finally{$('time-save').disabled=false;}
 });
 $('setup-finish').addEventListener('click',async()=>{
  $('setup-finish').disabled=true;
  try{
   const account={email:$('email').value.trim(),password:$('password').value,region:$('region').value,device_serial:$('device-serial').value,language};
   const detected=Intl.DateTimeFormat().resolvedOptions().timeZone;
   if(!config.has_password&&[...$('time-zone').options].some(o=>o.value===detected))account.time={zone:detected,automatic:true};
   await api('/api/setup','POST',{wifi:{ssid:$('wifi-ssid').value,password:$('wifi-password').value},account});
   $('wifi-password').value='';$('password').value='';dirty.delete(3);saveButtons();$('setup-result').textContent=t('setupSaved');
  }catch(e){$('setup-result').textContent=e.message;}finally{$('setup-finish').disabled=false;}
 });
 $('update-daily').addEventListener('change',()=>{updateDirty=true;$('update-result').textContent='';});
 $('update-open').addEventListener('click',()=>{chooseTab(3);$('update-card').scrollIntoView({block:'start'});});
 $('update-save').addEventListener('click',async()=>{try{const next=await api('/api/config','POST',{auto_update_check:$('update-daily').checked});config.auto_update_check=next.auto_update_check;updateDirty=false;$('update-result').textContent=t('saved');}catch(e){$('update-result').textContent=e.message;}});
 for(const action of ['check','install','rollback'])$('update-'+action).addEventListener('click',async()=>{
  if(action!=='check'&&(dirty.size||timeDirty||updateDirty)){$('update-result').textContent=t('updateUnsaved');return;}
  if(action!=='check'&&!confirm(t(action==='rollback'?'updateRollbackConfirm':'updateConfirm')))return;
  $('update-'+action).disabled=true;
  try{await api('/api/update/'+action,'POST',action==='check'?{}:{confirm:'UPDATE OWLANZI'});$('update-result').textContent='';if(action!=='check'){updateInFlight=true;$('update-card').dataset.previousVersion=latest.version;}await refresh();}catch(e){$('update-result').textContent=e.message;}
 });
 $('retry-settings').addEventListener('click',refresh);
 document.addEventListener('visibilitychange',()=>{if(!document.hidden)refresh();});
 window.addEventListener('beforeunload',e=>{if(dirty.size||timeDirty||updateDirty){e.preventDefault();e.returnValue='';}});
 setLanguage(language);chooseTab(0);draw($('mx'),null);
 for(const e of document.querySelectorAll('section input,section select,section button'))e.disabled=true;
 refresh();
 setInterval(refresh,2000);
}
