// SPDX-License-Identifier: GPL-3.0-or-later
async () => {
 const $=id=>document.getElementById(id),get=path=>fetch(path).then(r=>r.json());
 const post=(path,body)=>fetch(path,{method:'POST',headers:{'Content-Type':'application/json','X-Owlanzi-Request':'1'},body:JSON.stringify(body)}).then(r=>r.json());
 const check=(ok,label)=>{if(!ok)throw Error(label);};
 const until=async predicate=>{for(let i=0;i<120;i++){if(await predicate())return;await new Promise(r=>setTimeout(r,100));}throw Error('Clock/setup UI timeout');};
 check((await get('/api/status')).mode==='demo','Use a disposable demo only');
 await until(()=>!$('time-save').disabled);
 $('tab3').click();$('time-zone').value='Europe/Berlin';$('time-automatic').checked=false;$('time-automatic').dispatchEvent(new Event('change',{bubbles:true}));
 check(!$('time-manual-fields').hidden,'Manual date/time visible');
 $('time-local').value='2026-09-09T21:37';$('time-local').dispatchEvent(new Event('input',{bubbles:true}));$('time-save').click();
 await until(async()=>(await get('/api/status')).time.local.startsWith('2026-09-09T21:37'));
 await until(()=>!$('time-save').disabled);
 $('time-zone').value='Asia/Kolkata';$('time-automatic').checked=true;$('time-automatic').dispatchEvent(new Event('change',{bubbles:true}));$('time-save').click();
 await until(async()=>(await get('/api/status')).time.zone==='Asia/Kolkata');
 $('tab1').click();$('color-clock').value='#19ccee';$('color-clock').dispatchEvent(new Event('input',{bubbles:true}));
 await until(()=>{const c=document.querySelector('canvas[data-preview="vitals"]');return c.getContext('2d').getImageData(0,0,c.width,c.height).data.some((v,i,a)=>i%4===0&&v===25&&a[i+1]===204&&a[i+2]===238);});
 $('tab3').click();await post('/api/wifi/hotspot',{});
 await until(()=>!$('setup-finish').hidden);
 check($('s3').querySelector('.savebar').hidden,'Onboarding uses one combined save');
 $('wifi-ssid').value='Setup WLAN';$('wifi-password').value='fictional-wifi-password';$('wifi-connect').click();
 check(document.activeElement===$('email'),'Next step focuses Owlet email before connecting');
 check((await get('/api/wifi')).hotspot,'Hotspot stays open for Owlet credentials');
 $('email').value='setup@example.invalid';$('password').value='fictional-owlet-password';$('region').value='eu';$('setup-finish').click();
 await until(async()=>{const w=await get('/api/wifi');return w.phase==='connected'&&w.ssid==='Setup WLAN';});
 const cfg=await get('/api/config');check(cfg.email==='setup@example.invalid'&&cfg.has_password,'Owlet saved in combined setup');
 check(!$('wifi-password').value&&!$('password').value,'Both submitted passwords cleared');
 check(!JSON.stringify(await get('/api/status')).includes('fictional-'),'No secrets in status');
 check(document.documentElement.scrollWidth<=innerWidth,'No horizontal overflow');
 return {passed:true,checks:['manual time','automatic time and zone','clock color preview','combined onboarding','credentials cleared','mobile layout']};
}
