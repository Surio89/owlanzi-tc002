// SPDX-License-Identifier: GPL-3.0-or-later
// Evaluate this function on a disposable --demo server. No settings are saved.
async () => {
 const get=async path=>(await fetch(path)).json(),$=id=>document.getElementById(id);
 if((await get('/api/status')).mode!=='demo')throw new Error('Requires disposable demo server');
 const before=await get('/api/config'),results=[];
 const groups={vitals:['clock','heart','numbers','oxygen','oxygen_label'],sleep2:['awake','light_sleep','deep_sleep','unknown_sleep'],charging:['battery_frame','battery_fill','battery_charge','battery_mid','battery_low','battery','charging_text','battery_status'],waiting:['heart_wait','waiting','waiting_text','offline','reconnect_text'],alarm:['alarm','info','setup_title']};
 const until=async predicate=>{for(let n=0;n<60;n++){if(await predicate())return;await new Promise(r=>setTimeout(r,70));}throw new Error('Color preview timed out: '+results.at(-1));};
 const edit=(key,value)=>{const input=$('color-'+key);input.value=value;input.dispatchEvent(new Event('input',{bubbles:true}));};
 $('tab1').click();
 try {
  let blue=100;
  for(const [group,keys]of Object.entries(groups))for(const key of keys){
   const b=++blue,color='#19cc'+b.toString(16);edit(key,color);
   await until(()=>{const c=document.querySelector('canvas[data-preview="'+group+'"]'),a=c.getContext('2d').getImageData(0,0,c.width,c.height).data;return a.some((v,i)=>i%4===0&&v===25&&a[i+1]===204&&a[i+2]===b);});
   results.push(key);
  }
  const selected=()=>[...document.querySelectorAll('[data-preview-mode][aria-pressed="true"]')].map(b=>b.dataset.previewMode);
  if(!selected().includes('setup')||!selected().includes('offline')||!selected().includes('sleep0'))throw new Error('Unrelated color changes reset selected preview states');
  if(JSON.stringify(await get('/api/config'))!==JSON.stringify(before))throw new Error('Preview saved configuration');
  return {visibleColorControls:results,selectedStatesPreserved:true,configurationUnchanged:true};
 }finally{
  for(const [key,value]of Object.entries(before.palette))$('color-'+key).value=value;
  $('preview-stop').click();
  await until(async()=>!(await get('/api/status')).preview_active);
 }
};
