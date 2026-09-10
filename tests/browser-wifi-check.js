async () => {
 const $=id=>document.getElementById(id), pause=ms=>new Promise(r=>setTimeout(r,ms));
 const read=path=>fetch(path).then(r=>r.json());
 const check=(ok,message)=>{if(!ok)throw Error(message);};
 const until=async predicate=>{for(let i=0;i<80;i++){if(await predicate())return;await pause(100);}throw Error('Wi-Fi browser timeout');};
 check((await read('/api/status')).mode==='demo','This test requires simulated radio');
 await until(()=>!$('wifi-connect').disabled);
 const before=JSON.stringify(await read('/api/config'));
 document.querySelector('[data-tab="3"]').click();
 $('wifi-scan').click();
 await until(()=>$('wifi-network').options.length===3);
 await until(()=>/abgeschlossen|complete/.test($('wifi-result').textContent));
 $('wifi-network').value='Guest WLAN';$('wifi-network').dispatchEvent(new Event('change',{bubbles:true}));
 check($('wifi-ssid').value==='Guest WLAN','Selection fills SSID');
 $('wifi-password').value='fictional-wifi-secret';$('wifi-show').click();
 check($('wifi-password').type==='text','Password reveal works');$('wifi-show').click();
 $('wifi-connect').click();
 await until(async()=>{const w=await read('/api/wifi');return w.phase==='connected'&&w.ssid==='Guest WLAN';});
 check($('wifi-password').value==='','Submitted password cleared');
 await until(()=>!$('wifi-connect').disabled);
 $('wifi-ssid').value='Missing';$('wifi-password').value='wrong-password';$('wifi-connect').click();
 await until(async()=>{const w=await read('/api/wifi');return w.phase==='connected'&&w.error==='wifi_join_failed';});
 await until(()=>/fehlgeschlagen|failed/.test($('wifi-result').textContent));
 check((await read('/api/wifi')).ssid==='Guest WLAN','Failed join restores previous Wi-Fi');
 check(JSON.stringify(await read('/api/config'))===before,'Wi-Fi leaves Owlet/display configuration intact');
 check(!JSON.stringify(await read('/api/status')).includes('fictional-wifi-secret'),'Wi-Fi password absent from status');
 check(document.documentElement.scrollWidth<=innerWidth,'No horizontal overflow');
 return {passed:true,checks:['scan feedback','SSID selection','password reveal/clear','connection','failed password rollback','config isolation','responsive layout']};
}
