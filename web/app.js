// SPDX-License-Identifier: GPL-3.0-or-later
export const PALETTE_KEYS = Object.freeze(['heart', 'numbers', 'separator', 'waiting', 'awake', 'light_sleep', 'deep_sleep', 'unknown_sleep', 'battery', 'alarm', 'info', 'offline']);
export const ALARM_LIMITS = Object.freeze({spo2_min:[50,99],heart_rate_min:[30,150],heart_rate_max:[100,260],spo2_seconds:[5,300],heart_rate_low_seconds:[5,300],heart_rate_high_seconds:[5,300],volume:[0,6],alarm_repeat_seconds:[0,120],alarm_brightness:[0,255]});
const translations = {
  de: {
    connecting:'Verbinden …',connected:'Verbunden',disconnected:'Nicht verbunden',language:'Sprache',deviceApplication:'DEINE UHR. DEINE EINSTELLUNGEN.',title:'Ein guter Blick auf die Nacht.',intro:'Owlanzi läuft selbstständig auf deiner TC002. Hier richtest du die Uhr ein.',pairTitle:'Mit deiner Uhr verbinden',pairHelp:'Gib den Einrichtungsschlüssel aus dem lokalen Installer oder dem Startprotokoll ein. Er bleibt nur für diese Sitzung im Browser gespeichert.',pairToken:'Einrichtungsschlüssel',connect:'Verbinden',previewBright:'Vorschau aufhellen · nur im Browser',clockBrightness:'Uhrhelligkeit: ',onYourClock:'AUF DEINER UHR',displayTitle:'Die Anzeige im Blick',demoNotice:'Entwicklungsmodus · simulierte Werte. Diese Ansicht zeigt keine aktuellen Owlet-Messdaten.',heartRate:'Puls',oxygen:'Sauerstoff',sleep:'Schlafstatus',battery:'Socke · Akku',acknowledge:'Alarm bestätigen',testDisplay:'Anzeige testen:',demoVitals:'Messwerte',demoCharging:'Laden',demoOffline:'Offline',demoAlarm:'Alarm',demoWaiting:'Warten',accountTitle:'Dein Konto verbinden',accountHelp:'Die Uhr ruft deine Owlet-Daten direkt ab. Dein Computer wird danach nicht benötigt.',email:'E-Mail-Adresse',password:'Passwort',clearPassword:'Gespeichertes Passwort entfernen',region:'Owlet-Region',regionEu:'Europa',regionWorld:'International',pollInterval:'Daten abrufen',every5:'Alle 5 Sekunden',every10:'Alle 10 Sekunden',every15:'Alle 15 Sekunden',deviceSerial:'Owlet-Gerät',deviceHelp:'Nach dem Verbinden stehen deine Geräte zur Auswahl. Leer lassen, um das erste verfügbare Gerät zu verwenden.',appearance:'DARSTELLUNG',appearanceTitle:'So leuchtet deine Uhr',brightness:'Helligkeit',brightnessHelp:'0 schaltet die Matrix dunkel. Die Anwendung läuft weiter.',palette:'Farben',paletteHelp:'Die Farben werden mit deinen Einstellungen auf der Uhr gespeichert.',optional:'OPTIONAL',alarmsTitle:'Hinweise und Ton',soundTitle:'Ton und Hinweishelligkeit',enableAlarms:'Eigene Grenzwerte',alarmsHelp:'Eigene Grenzwerte sind standardmäßig aus. Owlanzi ersetzt keine medizinische Überwachung; die Originalwarnungen von Owlet bleiben maßgeblich.',spo2Min:'Sauerstoff unter (%)',delaySeconds:'Dauer (Sekunden)',heartRateMin:'Puls unter (bpm)',heartRateMax:'Puls über (bpm)',soundEnabled:'Ton aktivieren',volume:'Lautstärke (0–6)',repeatSeconds:'Ton wiederholen (Sekunden)',repeatHelp:'0 spielt den Ton einmal ab.',alarmBrightness:'Helligkeit bei Hinweisen (0–255)',saveHelp:'Die Einstellungen werden lokal auf deiner Uhr gespeichert.',save:'Einstellungen speichern',saving:'Wird gespeichert …',saved:'Einstellungen gespeichert. Die Uhr übernimmt die Änderungen.',footer:'Owlanzi für TC002 · eigenständige Geräteanwendung',disconnect:'Browser trennen',baseStation:'Die Owlet-Basisstation bleibt für die Verbindung der Socke erforderlich.',passwordSaved:'Ein Passwort ist gespeichert. Leer lassen, um es beizubehalten.',passwordEmpty:'Noch kein Passwort gespeichert.',demo:'Demo · simuliert',live:'Geräteanwendung',neverUpdated:'Noch keine aktuellen Messdaten',updated:'Aktualisiert: ',fresh:'Mit Owlet verbunden',stale:'Keine aktuellen Messdaten. Bitte Konto und Verbindung prüfen.',simulated:'Simulierte Daten · nur zum Testen der Anzeige',pairInvalid:'Der Einrichtungsschlüssel ist ungültig. Bitte erneut eingeben.',networkError:'Die Uhr ist nicht erreichbar. Prüfe die Verbindung und versuche es erneut.',requestError:'Die Anfrage konnte nicht ausgeführt werden.',invalidConfig:'Bitte prüfe die markierten Einstellungen.',invalidEmail:'Bitte gib eine gültige E-Mail-Adresse ein.',invalidRegion:'Bitte wähle eine gültige Owlet-Region.',invalidPoll:'Das Abrufintervall muss 5, 10 oder 15 Sekunden betragen.',invalidBrightness:'Die Helligkeit muss zwischen 0 und 255 liegen.',invalidPalette:'Bitte wähle für alle Anzeigen eine gültige Farbe.',invalidAlarm:'Bitte prüfe die Grenzwerte und die Dauer der Hinweise.',invalidHeartRange:'Der untere Pulsgrenzwert muss kleiner als der obere sein.',invalidPassword:'Bitte gib ein Passwort mit höchstens 512 Zeichen ein.',invalidDevice:'Die Geräteseriennummer darf höchstens 128 Zeichen enthalten.',invalidLanguage:'Bitte wähle Deutsch oder Englisch.',invalidFlags:'Bitte prüfe die Ein/Aus-Einstellungen.',alarmActive:'Ein Hinweis ist aktiv.',alarmAcknowledged:'Hinweis bestätigt',awake:'Wach',light_sleep:'Leichter Schlaf',deep_sleep:'Tiefer Schlaf',unknown_sleep:'Unbekannt',sleeping:'Schläft',charging:'Lädt',waiting:'Warten',offline:'Offline',heart:'Herzsymbol',numbers:'Messwerte',separator:'Trennzeichen',unknown:'Unbekannt',alarm:'Hinweise',info:'Information',matrixLabel:'52 × 16 LED-Anzeige',sessionUnavailable:'Der Browser erlaubt keine Sitzungsspeicherung. Der Schlüssel bleibt nur bis zum Neuladen im Arbeitsspeicher.'
  },
  en: {
    connecting:'Connecting …',connected:'Connected',disconnected:'Disconnected',language:'Language',deviceApplication:'YOUR CLOCK. YOUR SETTINGS.',title:'A little clarity through the night.',intro:'Owlanzi runs independently on your TC002. Set up your clock here.',pairTitle:'Connect to your clock',pairHelp:'Enter the setup key from the local installer or startup log. It is only kept in this browser session.',pairToken:'Setup key',connect:'Connect',previewBright:'Brighten preview · browser only',clockBrightness:'Clock brightness: ',onYourClock:'ON YOUR CLOCK',displayTitle:'Your display at a glance',demoNotice:'Development mode · simulated values. This view does not show current Owlet readings.',heartRate:'Heart rate',oxygen:'Oxygen',sleep:'Sleep status',battery:'Sock · battery',acknowledge:'Acknowledge alert',testDisplay:'Test display:',demoVitals:'Readings',demoCharging:'Charging',demoOffline:'Offline',demoAlarm:'Alert',demoWaiting:'Waiting',accountTitle:'Connect your account',accountHelp:'The clock fetches your Owlet data directly. Your computer can be switched off after setup.',email:'Email address',password:'Password',clearPassword:'Remove saved password',region:'Owlet region',regionEu:'Europe',regionWorld:'International',pollInterval:'Fetch data',every5:'Every 5 seconds',every10:'Every 10 seconds',every15:'Every 15 seconds',deviceSerial:'Owlet device',deviceHelp:'Your devices appear after connecting. Leave blank to use the first available device.',appearance:'APPEARANCE',appearanceTitle:'Make your clock your own',brightness:'Brightness',brightnessHelp:'0 turns the matrix dark. The application keeps running.',palette:'Colors',paletteHelp:'Colors are saved with your settings on the clock.',optional:'OPTIONAL',alarmsTitle:'Alerts and sound',soundTitle:'Sound and alert brightness',enableAlarms:'Custom thresholds',alarmsHelp:'Custom thresholds are off by default. Owlanzi does not replace medical monitoring; continue to rely on the original Owlet alerts.',spo2Min:'Oxygen below (%)',delaySeconds:'Duration (seconds)',heartRateMin:'Heart rate below (bpm)',heartRateMax:'Heart rate above (bpm)',soundEnabled:'Enable sound',volume:'Volume (0–6)',repeatSeconds:'Repeat sound (seconds)',repeatHelp:'0 plays the sound once.',alarmBrightness:'Alert brightness (0–255)',saveHelp:'Settings are saved locally on your clock.',save:'Save settings',saving:'Saving …',saved:'Settings saved. The clock is applying your changes.',footer:'Owlanzi for TC002 · standalone device application',disconnect:'Disconnect browser',baseStation:'The Owlet base station is still required to connect the sock.',passwordSaved:'A password is saved. Leave blank to keep it.',passwordEmpty:'No password saved yet.',demo:'Demo · simulated',live:'Device application',neverUpdated:'No current readings yet',updated:'Updated: ',fresh:'Connected to Owlet',stale:'No current readings. Check your account and connection.',simulated:'Simulated data · for testing the display only',pairInvalid:'The setup key is invalid. Please enter it again.',networkError:'The clock is unreachable. Check your connection and try again.',requestError:'The request could not be completed.',invalidConfig:'Please check the highlighted settings.',invalidEmail:'Please enter a valid email address.',invalidRegion:'Please select a valid Owlet region.',invalidPoll:'The polling interval must be 5, 10 or 15 seconds.',invalidBrightness:'Brightness must be between 0 and 255.',invalidPalette:'Please select a valid color for each display element.',invalidAlarm:'Please check alert thresholds and durations.',invalidHeartRange:'The lower heart rate threshold must be smaller than the upper one.',invalidPassword:'Please enter a password no longer than 512 characters.',invalidDevice:'The device serial must not exceed 128 characters.',invalidLanguage:'Please select German or English.',invalidFlags:'Please check the on/off settings.',alarmActive:'An alert is active.',alarmAcknowledged:'Alert acknowledged',awake:'Awake',light_sleep:'Light sleep',deep_sleep:'Deep sleep',unknown_sleep:'Unknown',sleeping:'Sleeping',charging:'Charging',waiting:'Waiting',offline:'Offline',heart:'Heart icon',numbers:'Readings',separator:'Separator',unknown:'Unknown',alarm:'Alerts',info:'Information',matrixLabel:'52 × 16 LED display',sessionUnavailable:'This browser does not allow session storage. The key is held in memory until the page is reloaded.'
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
  if (typeof config.device_serial !== 'string' || config.device_serial.length > 128) errors.push(['device-serial','invalidDevice']);
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

export function requestHeaders(token, method = 'GET') {
  const headers = {Authorization:`Bearer ${token}`};
  if (method !== 'GET') {headers['Content-Type']='application/json';headers['X-Owlanzi-Request']='1';}
  return headers;
}

// This guard also allows the pure validation helpers to be tested without a DOM.
if (typeof document !== 'undefined') startApplication();

function startApplication() {
  const $ = id => document.getElementById(id);
  const sessionKey = 'owlanzi.tc002.setup-token';
  let language = 'de', token = '', config = null, latestStatus = null, polling = false, authenticating = false;
  try { token = sessionStorage.getItem(sessionKey) ?? ''; } catch { /* Memory-only authentication remains available. */ }
  const t = key => translate(language,key);
  const showNotice = message => { $('notice').textContent=message; $('notice').hidden=!message; };
  const saveToken = value => {token=value;try{if(value)sessionStorage.setItem(sessionKey,value);else sessionStorage.removeItem(sessionKey);}catch{if(value)showNotice(t('sessionUnavailable'));}};

  for (const key of PALETTE_KEYS) {
    const label=document.createElement('label');label.className='palette-field';
    const input=document.createElement('input');input.type='color';input.id=`color-${key}`;input.value='#ffffff';
    const caption=document.createElement('span');caption.dataset.i18n=key;
    label.append(input,caption);$('palette-fields').append(label);
  }
  function applyLanguage() {
    document.documentElement.lang=language;$('language').value=language;
    for(const element of document.querySelectorAll('[data-i18n]')) element.textContent=t(element.dataset.i18n);
    $('language').setAttribute('aria-label',t('language'));$('matrix').setAttribute('aria-label',t('matrixLabel'));
    $('password-help').textContent=t(config?.has_password ? 'passwordSaved' : 'passwordEmpty');
    if(latestStatus)renderStatus(latestStatus);
    if(!$('application').hidden){$('connection-badge').textContent=t('connected');}else{$('connection-badge').textContent=t('disconnected');}
  }
  function showPairing(message='') {
    $('pairing').hidden=false;$('application').hidden=true;$('pair-error').textContent=message;
    $('connection-badge').textContent=t('disconnected');$('connection-badge').className='badge';
  }
  async function api(path,method='GET',body) {
    const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),12000);
    let response;
    try {response=await fetch(path,{method,headers:requestHeaders(token,method),body:body===undefined?undefined:JSON.stringify(body),cache:'no-store',signal:controller.signal});}
    catch {throw new Error(t('networkError'));}finally{clearTimeout(timeout);}
    if(response.status===401){saveToken('');showPairing(t('pairInvalid'));const error=new Error(t('pairInvalid'));error.unauthorized=true;throw error;}
    const data=await response.json().catch(()=>({}));
    if(!response.ok)throw new Error(typeof data.error==='string'?data.error:t('requestError'));
    return data;
  }
  function applyConfig(next) {
    config=next;language=['de','en'].includes(next.language)?next.language:language;
    for(const [id,key] of [['email','email'],['region','region'],['device-serial','device_serial'],['poll-interval','poll_interval_seconds'],['brightness','brightness']]) if(next[key]!==undefined)$(id).value=String(next[key]);
    for(const key of PALETTE_KEYS) if(/^#[0-9a-f]{6}$/i.test(next.palette?.[key]??''))$(`color-${key}`).value=next.palette[key];
    const alarms=next.alarms??{};
    $('alarms-enabled').checked=alarms.enabled===true;$('sound-enabled').checked=alarms.sound_enabled===true;
    for(const key of Object.keys(ALARM_LIMITS))if(alarms[key]!==undefined)$(key.replaceAll('_','-')).value=String(alarms[key]);
    $('alarm-fields').disabled=!$('alarms-enabled').checked;
    $('password').value='';$('clear-password').checked=false;
    $('brightness-output').value=`${$('brightness').value} / 255`;applyLanguage();
  }
  function renderMatrix(display) {
    const context=$('matrix').getContext('2d');if(!context)return;
    const pixels=safePixels(display);context.fillStyle='#030406';context.fillRect(0,0,624,192);
    for(let y=0;y<16;y++)for(let x=0;x<52;x++){
      const color=pixels?scaleColor(pixels[y*52+x],$('preview-bright').checked?255:display.brightness):'#000000';context.fillStyle=color.toLowerCase()==='#000000'?'#020305':color;
      context.fillRect(x*12+1,y*12+1,9,9);
    }
  }
  function renderStatus(status) {
    latestStatus=status;const demo=isDemo(status), readings=visibleReadings(status);
    $('mode-badge').textContent=t(demo?'demo':'live');$('mode-badge').className=`badge${demo?' demo':''}`;
    $('demo-notice').hidden=!demo;$('demo-controls').hidden=!demo;
    const displayNumber=value=>typeof value==='number'&&Number.isFinite(value)?String(Math.round(value)):'—';
    $('heart-value').textContent=displayNumber(readings?.heart_rate);$('oxygen-value').textContent=displayNumber(readings?.oxygen);$('battery-value').textContent=displayNumber(readings?.battery);
    const sleep=readings?.sleep_status;$('sleep-value').textContent=typeof sleep==='string'?t(sleep):'—';
    $('device-name').textContent=status.device?.name || status.device?.serial || 'TC002';
    const updated=status.last_updated?new Date(status.last_updated):null;
    $('last-updated').textContent=updated&&!Number.isNaN(updated.getTime())?t('updated')+updated.toLocaleTimeString(language==='de'?'de-DE':'en-GB',{hour:'2-digit',minute:'2-digit',second:'2-digit'}):t('neverUpdated');
    $('cloud-state').textContent=demo?t('simulated'):(status.state_message||(status.cloud_fresh?t('fresh'):(status.last_error||t('stale'))));
    $('cloud-state').className=`state-line${!demo&&status.cloud_fresh?' fresh':''}`;
    const active=status.alarm?.active===true;$('alarm-status').hidden=!active;
    $('alarm-message').textContent=status.alarm?.acknowledged?t('alarmAcknowledged'):(status.alarm?.message||t('alarmActive'));
    $('acknowledge').hidden=status.alarm?.acknowledged===true;
    $('device-options').replaceChildren();
    for(const device of Array.isArray(status.devices)?status.devices:[])if(typeof device.serial==='string'){
      const option=document.createElement('option');option.value=device.serial;option.label=device.name||device.serial;$('device-options').append(option);
    }
    $('clock-brightness').textContent=t('clockBrightness')+(status.display?.brightness??'—')+' / 255';renderMatrix(status.display);
  }
  async function connect() {
    if(authenticating)return;authenticating=true;
    const button=$('pair-form').querySelector('button');button.disabled=true;$('pair-error').textContent='';
    try {
      const next=await api('/api/config');const status=await api('/api/status');
      applyConfig(next);renderStatus(status);$('pairing').hidden=true;$('application').hidden=false;
      $('pairing-token').value='';$('connection-badge').textContent=t('connected');$('connection-badge').className='badge good';showNotice('');
    }catch(error){showPairing(error.message);}finally{authenticating=false;button.disabled=false;}
  }
  async function refresh() {
    if(!token||polling||authenticating||$('application').hidden||document.hidden)return;
    polling=true;
    try{renderStatus(await api('/api/status'));$('connection-badge').textContent=t('connected');$('connection-badge').className='badge good';showNotice('');}
    catch(error){if(!error.unauthorized){$('connection-badge').textContent=t('disconnected');$('connection-badge').className='badge';showNotice(error.message);if(latestStatus)renderStatus({...latestStatus,cloud_fresh:false,readings:null,last_error:t('networkError')});}}
    finally{polling=false;}
  }
  function readForm() {
    const raw={email:$('email').value,password:$('password').value,clear_password:$('clear-password').checked,region:$('region').value,language,device_serial:$('device-serial').value,poll_interval_seconds:$('poll-interval').value,brightness:$('brightness').value,palette:{},alarms:{enabled:$('alarms-enabled').checked,sound_enabled:$('sound-enabled').checked}};
    for(const key of PALETTE_KEYS)raw.palette[key]=$(`color-${key}`).value;
    for(const key of Object.keys(ALARM_LIMITS))raw.alarms[key]=$(key.replaceAll('_','-')).value;
    return prepareConfig(raw);
  }
  $('pair-form').addEventListener('submit',event=>{event.preventDefault();const value=$('pairing-token').value.trim();if(value){saveToken(value);connect();}});
  $('language').addEventListener('change',()=>{language=$('language').value;applyLanguage();});
  $('brightness').addEventListener('input',()=>{$('brightness-output').value=`${$('brightness').value} / 255`;});
  $('alarms-enabled').addEventListener('change',()=>{$('alarm-fields').disabled=!$('alarms-enabled').checked;});
  $('password').addEventListener('input',()=>{if($('password').value)$('clear-password').checked=false;});
  $('clear-password').addEventListener('change',()=>{if($('clear-password').checked)$('password').value='';});
  $('disconnect').addEventListener('click',()=>{saveToken('');latestStatus=null;config=null;$('config-form').reset();showNotice('');showPairing();$('pairing-token').focus();});
  $('config-form').addEventListener('submit',async event=>{
    event.preventDefault();for(const element of $('config-form').querySelectorAll('[aria-invalid]'))element.removeAttribute('aria-invalid');
    const next=readForm(),errors=validateConfig(next);$('save-result').className='';
    if(errors.length){for(const [id] of errors)$(id)?.setAttribute('aria-invalid','true');$('save-result').textContent=t(errors[0][1]);$('save-result').className='error';$(errors[0][0])?.focus();return;}
    $('save').disabled=true;$('save').textContent=t('saving');
    try{
      await api('/api/config','POST',next);applyConfig(await api('/api/config'));$('save-result').textContent=t('saved');$('save-result').className='success';await refresh();
    }catch(error){$('save-result').textContent=error.message;$('save-result').className='error';}
    finally{$('save').disabled=false;$('save').textContent=t('save');}
  });
  $('acknowledge').addEventListener('click',async()=>{
    $('acknowledge').disabled=true;try{await api('/api/acknowledge','POST',{});await refresh();}catch(error){showNotice(error.message);}finally{$('acknowledge').disabled=false;}
  });
  for(const button of document.querySelectorAll('[data-scenario]'))button.addEventListener('click',async()=>{
    button.disabled=true;try{await api('/api/demo','POST',{scenario:button.dataset.scenario});await refresh();}catch(error){showNotice(error.message);}finally{button.disabled=false;}
  });
  $('preview-bright').addEventListener('change',()=>renderMatrix(latestStatus?.display));
  document.addEventListener('visibilitychange',()=>{if(!document.hidden)refresh();});
  applyLanguage();renderMatrix(null);if(token)connect();else showPairing();
  setInterval(refresh,3000);
}
