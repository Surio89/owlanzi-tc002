// SPDX-License-Identifier: GPL-3.0-or-later
// Run with: node --test tests/web-tests.mjs (no packages or browser required).
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';

const source=await readFile(new URL('../web/app.js',import.meta.url),'utf8');
const ui=await import(`data:text/javascript;base64,${Buffer.from(source).toString('base64')}`);
const fixture=()=>({email:'parent@example.org',password:'',clear_password:false,region:'eu',language:'de',device_serial:'',poll_interval_seconds:10,brightness:96,palette:Object.fromEntries(ui.PALETTE_KEYS.map(key=>[key,'#aabbcc'])),alarms:{enabled:false,sound_enabled:false,spo2_min:90,heart_rate_min:60,heart_rate_max:220,spo2_seconds:30,heart_rate_low_seconds:30,heart_rate_high_seconds:30,volume:2,alarm_repeat_seconds:30,alarm_brightness:128}});

test('updates require home Wi-Fi, no critical alarm, an installed loader and an available release',()=>{
 const ready={phase:'available',install_supported:true,available:true,rollback_available:true},status={wifi:{connected:true},alarm:{critical:false}};
 assert.equal(ui.updateControls(ready,status).install,true);
 for(const phase of ['queued','checking','downloading','switching'])assert.equal(ui.updateControls({...ready,phase},status).install,false);
 for(const wifi of [{connected:false},{connected:true,hotspot:true},{connected:true,busy:true}])assert.equal(ui.updateControls(ready,{wifi}).check,false);
 assert.equal(ui.updateControls(ready,{...status,alarm:{critical:true}}).rollback,false);
 assert.equal(ui.updateControls({...ready,install_supported:false},status).install,false);
 assert.equal(ui.updateControls({...ready,available:false},status).install,false);
 for(const language of ['de','en'])for(const key of ['updateTitle','updateDaily','updateError','updateRestored','updateKeep'])assert.notEqual(ui.translate(language,key),key);
});

test('a blank password keeps credentials and private/server fields are never echoed',()=>{
  const data=ui.prepareConfig({...fixture(),email:' parent@example.org ',has_password:true,setup_token:'sensitive',unknown:'discard'});
  assert.equal(data.email,'parent@example.org');assert.ok(!('password' in data));assert.ok(!('has_password' in data));assert.ok(!('setup_token' in data));assert.ok(!('unknown' in data));assert.deepEqual(ui.validateConfig(data),[]);
});
test('password clearing is explicit and a new password is not trimmed',()=>{
  assert.equal(ui.prepareConfig({...fixture(),clear_password:true}).clear_password,true);
  const data=ui.prepareConfig({...fixture(),clear_password:true,password:' new password '});assert.equal(data.password,' new password ');assert.equal(data.clear_password,false);
});
test('all allowed polling values, language and brightness endpoints are accepted',()=>{
  for(const seconds of [5,10,15])for(const brightness of [0,255])for(const language of ['de','en'])assert.deepEqual(ui.validateConfig(ui.prepareConfig({...fixture(),poll_interval_seconds:seconds,brightness,language})),[]);
});
test('validation rejects unsupported intervals, invalid colors, reversed thresholds and decimals',()=>{
  const data=ui.prepareConfig(fixture());data.poll_interval_seconds=1;data.palette.heart='red';data.alarms.heart_rate_min=250;data.alarms.spo2_seconds=0.5;
  const errors=ui.validateConfig(data).map(([,error])=>error);for(const expected of ['invalidPoll','invalidPalette','invalidHeartRange','invalidAlarm'])assert.ok(errors.includes(expected));
});
test('validation blocks malformed and out-of-range values, even when alarms are disabled',()=>{
  const data=ui.prepareConfig(fixture());data.brightness=256;data.email='<script>';data.region='custom';data.alarms.volume=7;data.alarms.heart_rate_high_seconds=301;data.alarms.alarm_brightness=-1;
  assert.equal(ui.validateConfig(data).length,6);
});
test('stale readings are suppressed but explicit demo values remain visible',()=>{
  const readings={heart_rate:110,oxygen:98};assert.equal(ui.visibleReadings({mode:'device',cloud_fresh:false,readings}),null);assert.equal(ui.visibleReadings({mode:'device',readings}),null);assert.equal(ui.visibleReadings({mode:'device',cloud_fresh:true,readings}),readings);assert.equal(ui.visibleReadings({mode:'demo',cloud_fresh:false,readings}),readings);assert.equal(ui.visibleReadings({mode:'simulation',readings}),readings);
});
test('matrix geometry is strict and untrusted colors are replaced with black',()=>{
  const display={width:52,height:16,pixels:Array(832).fill('#FF8800')};assert.equal(ui.safePixels(display).length,832);display.pixels[0]='url(https://external.invalid)';assert.equal(ui.safePixels(display)[0],'#000000');assert.equal(ui.safePixels({...display,width:32}),null);assert.equal(ui.safePixels({...display,pixels:[]}),null);
});
test('matrix colors follow actual brightness, including a dark display at zero',()=>{
  assert.equal(ui.scaleColor('#ff8040',255),'#ff8040');assert.equal(ui.scaleColor('#ff8040',0),'#000000');assert.equal(ui.scaleColor('#ff8040',128),'#804020');assert.equal(ui.scaleColor('#ffffff',NaN),'#000000');
});
test('initial access needs no key; writes retain same-origin JSON headers',()=>{
  assert.deepEqual(ui.requestHeaders(),{});assert.deepEqual(ui.requestHeaders('POST'),{'Content-Type':'application/json','X-Owlanzi-Request':'1'});
});
test('optional web password and preview brightness stay separate from Owlet credentials',()=>{
 const data=ui.prepareConfig({...fixture(),web_password:' new web password ',preview_brightness:0});
 assert.equal(data.web_password,' new web password ');assert.equal(data.preview_brightness,0);assert.deepEqual(ui.validateConfig(data),[]);
 data.web_password='bad\npassword';assert.ok(ui.validateConfig(data).some(([id])=>id==='web-password'));
});
test('every static translation exists in German and English; all assets stay local',async()=>{
  const html=await readFile(new URL('../web/index.html',import.meta.url),'utf8');
  for(const [,key] of html.matchAll(/data-i18n="([^"]+)"/g))for(const language of ['de','en'])assert.notEqual(ui.translate(language,key),key,`${language}: ${key}`);
  for(const [,target] of html.matchAll(/(?:src|href)="([^"]+)"/g))assert.ok(target.startsWith('/')||target.startsWith('data:')||['https://discord.gg/Bhpr3zRfVv','https://ko-fi.com/owlanzi'].includes(target),target);
  assert.ok(!source.includes('innerHTML'));assert.ok(!source.includes('localStorage'));assert.ok(!source.includes('https://'));
});
