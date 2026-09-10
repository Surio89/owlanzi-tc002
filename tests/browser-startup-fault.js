// SPDX-License-Identifier: GPL-3.0-or-later
// Install as a DevTools navigation initScript, then evaluate
// window.checkSettingsRecovery(). This intercepts only this test tab's reads.
{
 const realFetch=window.fetch.bind(window);
 let allowSettings=false,attempts=0;
 window.fetch=(url,options)=>{
  if(url==='/api/config'){
   attempts++;
   if(!allowSettings)return Promise.resolve(new Response(JSON.stringify({error:'Test: configuration unavailable'}),{status:503,headers:{'Content-Type':'application/json'}}));
  }
  return realFetch(url,options);
 };
 window.checkSettingsRecovery=async()=>{
  const $=id=>document.getElementById(id),results=[];
  const until=async predicate=>{for(let n=0;n<100;n++){if(predicate())return;await new Promise(r=>setTimeout(r,100));}throw new Error('Settings recovery timed out');};
  const check=(condition,message)=>{if(!condition)throw new Error(message);results.push(message);};
  try{
   await until(()=>attempts>=2&&$('reason')?.textContent.length>0);
   check($('email').disabled,'Controls stay disabled while settings are unavailable');
   check(!$('settings-loading').hidden&&$('settings-loading-message').textContent.includes('Test:'),'Status polling preserves the settings error');
   check(attempts>=2,'Initialization retries automatically');
  }finally{allowSettings=true;}
  await until(()=>!$('email').disabled);
  check($('settings-loading').hidden,'Recovery removes loading error without reloading');
  check(!$('alarms-enabled').disabled,'Recovery enables alarm switch');
  check([...document.querySelectorAll('[data-color]')].every(e=>!e.disabled),'Recovery enables every color input');
  return results;
 };
}
