// SPDX-License-Identifier: GPL-3.0-or-later
// Run this function through browser DevTools on a disposable --demo instance.
// Exercises real DOM handlers and HTTP persistence. Never use real credentials.
async () => {
 const $=id=>document.getElementById(id),results=[];
 const wait=ms=>new Promise(resolve=>setTimeout(resolve,ms));
 const check=(value,message)=>{if(!value)throw new Error(message);results.push(message);};
 const get=async path=>(await fetch(path)).json();
 const until=async predicate=>{for(let n=0;n<50;n++){if(await predicate())return;await wait(100);}throw new Error('UI operation timed out');};
 const input=(id,value)=>{const e=$(id);e.value=value;e.dispatchEvent(new Event('input',{bubbles:true}));};
 if((await get('/api/status')).mode!=='demo')throw new Error('Requires disposable demo server');
 await until(()=>!$('email').disabled);
 $('tab1').click();
 const before=$('color-heart').value;
 input('color-heart','#19ccee');
 await until(()=>document.querySelector('canvas[data-preview="vitals"]').getContext('2d').getImageData(0,0,624,192).data.some((v,i,a)=>i%4===0&&v===25&&a[i+1]===204&&a[i+2]===238));
 check(true,'Selected heart color appears in canvas preview');
 await until(async()=>(await get('/api/status')).preview_active);
 check(true,'Color input activates temporary device preview');
 document.querySelector('[data-save="1"]').click();
 await until(async()=>(await get('/api/config')).palette.heart.toLowerCase()==='#19ccee');
 check(!(await get('/api/status')).preview_active,'Display save persists color and stops temporary preview');
 input('color-heart',before);document.querySelector('[data-save="1"]').click();
 await until(async()=>(await get('/api/config')).palette.heart.toLowerCase()===before);
 $('tab2').click();$('alarms-enabled').click();
 check(!$('alarm-fields').disabled,'Alarm switch enables threshold inputs');
 input('spo2-min','91');document.querySelector('[data-save="2"]').click();
 await until(async()=>{const a=(await get('/api/config')).alarms;return a.enabled&&a.spo2_min===91;});
 check(true,'Alarm switch and threshold are saved');
 $('alarms-enabled').click();document.querySelector('[data-save="2"]').click();
 await until(async()=>!(await get('/api/config')).alarms.enabled);
 $('tab3').click();input('email','ui-test@example.invalid');input('password','fictional-ui-test');
 document.querySelector('[data-eye="password"]').click();
 check($('password').type==='text','Password visibility toggle works');
 document.querySelector('[data-eye="password"]').click();
 document.querySelector('[data-save="3"]').click();
 await until(async()=>{const c=await get('/api/config');return c.email==='ui-test@example.invalid'&&c.has_password;});
 check($('password').value==='','Owlet account form saves and clears the password field');
 check($('notice').hidden,'No remaining UI error');
 return results;
};
