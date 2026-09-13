#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the real compiled server, with simulation only and no cloud calls."""
import json
import base64
import http.client
from concurrent.futures import ThreadPoolExecutor
import pathlib
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

with tempfile.TemporaryDirectory(prefix='owlanzi-api-') as temp:
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        port = sock.getsockname()[1]
    # Upgrade a real saved palette: retain explicit new colors and inherit
    # newly separated content colors from the old shared settings.
    (pathlib.Path(temp)/'config.json').write_text(json.dumps({'schema':1,'palette':{
        'numbers':'#123456','oxygen':'#778899','battery_charge':'#234567',
        'battery':'#345678','waiting':'#456789'}}))
    process = subprocess.Popen([sys.argv[1], '--demo', '--data-dir', temp, '--port', str(port), '--duration', '40'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    token_file = pathlib.Path(temp) / 'pairing-token'
    base = f'http://127.0.0.1:{port}'
    try:
        for attempt in range(100):
            if process.poll() is not None:
                raise AssertionError('Server exited before startup')
            try:
                urllib.request.urlopen(base, timeout=.25).close()
                break
            except (OSError, urllib.error.URLError):
                time.sleep(.05)
        else:
            raise AssertionError('Server did not start')
        assert not token_file.exists()
        def call(path, payload=None, authorized=False, request_header=True, origin=None):
            headers = {'Authorization': 'Basic ' + base64.b64encode(b'owlanzi:test-web-password').decode()} if authorized else {}
            if origin is not None: headers['Origin']=origin
            if payload is not None:
                headers['Content-Type'] = 'application/json'
                if request_header:
                    headers['X-Owlanzi-Request'] = '1'
            req = urllib.request.Request(base+path, data=None if payload is None else json.dumps(payload).encode(), headers=headers)
            try:
                with urllib.request.urlopen(req, timeout=4) as response:
                    return response.status, json.load(response)
            except urllib.error.HTTPError as error:
                return error.code, json.load(error)
        assert call('/api/config')[0] == 200
        assert call('/api/status')[0] == 200
        assert call('/api/system')[1]=={'persistent_boot':False,'actions_supported':False}
        assert call('/api/system/restore-manufacturer',{'confirm':'RESTORE ULANZI'})[0]==409
        # Three open browser connections must not occupy all device workers
        # while a fourth client tries to load settings.
        idle=[]
        def browser_connection(_):
            client=http.client.HTTPConnection('127.0.0.1',port,timeout=2)
            idle.append(client)
            client.request('GET','/api/defaults')
            response=client.getresponse()
            response.read()
            assert response.status==200
        try:
            with ThreadPoolExecutor(3) as pool:
                list(pool.map(browser_connection,range(3)))
            client=http.client.HTTPConnection('127.0.0.1',port,timeout=2)
            try:
                client.request('GET','/api/config')
                response=client.getresponse()
                assert response.status==200
                response.read()
            finally: client.close()
        finally:
            for client in idle: client.close()
        assert call('/api/config', {}, origin='https://foreign.example')[0] == 403
        # A browser can send headers and body in separate TCP packets. An
        # early rejection must not reset the connection before JSON is read.
        for _ in range(5):
            client=http.client.HTTPConnection('127.0.0.1',port,timeout=2)
            try:
                client.putrequest('POST','/api/config')
                for key,value in {'Content-Type':'application/json','Content-Length':'2','X-Owlanzi-Request':'1','Origin':'https://foreign.example'}.items():
                    client.putheader(key,value)
                client.endheaders()
                time.sleep(.02)
                client.send(b'{}')
                response=client.getresponse()
                assert response.status==403 and json.loads(response.read())['error']=='Same-origin request required'
            finally: client.close()
        assert call('/api/config', {}, origin=base)[0] == 200
        assert call('/api/acknowledge', {}, request_header=False)[0] == 403
        code, config = call('/api/config')
        assert code == 200 and 'password' not in config and not config['alarms']['enabled']
        palette=config['palette']
        assert palette['oxygen']=='#778899' and palette['oxygen_label']==palette['setup_title']=='#123456'
        assert palette['charging_text']=='#234567' and palette['battery_status']=='#345678'
        assert palette['waiting_text']==palette['reconnect_text']=='#456789'
        assert call('/api/config',{'palette':{'numbers':'#112233'}})[1]['palette']['oxygen_label']=='#123456'
        color_modes={'clock':'vitals','heart':'vitals','numbers':'vitals','oxygen':'vitals','oxygen_label':'vitals',
            'awake':'sleep1','light_sleep':'sleep2','deep_sleep':'sleep3','unknown_sleep':'sleep0',
            'battery_frame':'battery','battery_fill':'battery','battery_charge':'charging','battery':'battery',
            'battery_mid':'battery-mid','battery_low':'battery-low','charging_text':'charging','battery_status':'battery',
            'heart_wait':'waiting','waiting':'waiting','waiting_text':'waiting','offline':'offline',
            'reconnect_text':'offline','alarm':'alarm','info':'info','setup_title':'setup'}
        assert set(color_modes)==set(palette)
        areas={'clock':(29,10,47,14),'heart':(1,0,7,6),'numbers':(9,0,22,6),'oxygen':(30,0,47,6),
            'charging_text':(0,1,51,5),'battery_status':(0,1,51,5),
            'waiting_text':(0,10,51,14),'reconnect_text':(0,10,51,14),'setup_title':(0,1,51,5)}
        for key,mode in color_modes.items():
            colors={k:'#000000' for k in palette};colors[key]='#19CCEE'
            code,frame=call('/api/preview/render',{'mode':mode,'palette':colors})
            changed=[(i%52,i//52) for i,color in enumerate(frame['pixels']) if color=='#19CCEE']
            if key=='oxygen_label':
                assert code==200 and not changed, 'legacy label color is preserved but no longer drawn'
                continue
            assert code==200 and changed, f'{key} must affect its TC002 content in {mode}'
            if key in areas:
                x1,y1,x2,y2=areas[key]
                assert all(x1<=x<=x2 and y1<=y<=y2 for x,y in changed), f'{key} affects unrelated content'
        code, config = call('/api/config', {'email':'example@example.invalid','password':'fictional-test-secret','brightness':42})
        assert code == 200 and config['has_password'] and 'password' not in config
        assert call('/api/config', {'password':''})[1]['has_password']
        before = (pathlib.Path(temp)/'config.json').read_bytes()
        assert call('/api/config', {'poll_interval_seconds':1})[0] == 400
        assert (pathlib.Path(temp)/'config.json').read_bytes() == before
        assert call('/api/config', {'clear_password':True})[1]['has_password'] is False
        assert call('/api/defaults')[1]['has_web_password'] is False
        snapshot=(pathlib.Path(temp)/'config.json').read_bytes()
        example={'mode':'charging','palette':{'battery_charge':'#00ff00'},'preview_brightness':12}
        assert call('/api/preview/render',example)[1]['width']==52
        assert call('/api/preview',example)[0]==200
        preview=call('/api/status')[1]
        assert preview['preview_active'] and preview['display']['brightness']==12
        assert (pathlib.Path(temp)/'config.json').read_bytes()==snapshot
        assert call('/api/config')[1]['brightness']==42
        assert call('/api/preview',{'mode':'invalid'})[0]==400
        assert call('/api/preview',{'preview_brightness':999})[0]==400
        assert call('/api/preview/stop',{})[0]==200
        assert not call('/api/status')[1]['preview_active']
        assert call('/api/preview',{'mode':'corners'})[0]==200
        deadline=time.monotonic()+12
        while call('/api/status')[1]['preview_active'] and time.monotonic()<deadline:
            time.sleep(.25)
        assert not call('/api/status')[1]['preview_active']
        assert call('/api/sound',{'volume':7})[0]==400
        assert call('/api/sound',{'volume':0})[0]==200
        assert call('/api/sound/stop',{})[0]==200
        assert call('/api/reset',{})[0]==400
        assert call('/api/config',{'web_password':'test-web-password'})[1]['has_web_password']
        assert call('/api/status')[0]==401
        assert call('/api/config')[0]==401
        assert call('/api/config',{})[0]==401
        assert call('/api/wifi')[0]==401
        assert call('/api/wifi/scan',{})[0]==401
        assert call('/api/setup',{})[0]==401
        assert call('/api/timezones')[0]==401
        for path in ['/', '/app.js', '/style.css']:
            assert call(path)[0]==401
        assert call('/api/status',authorized=True)[0]==200
        assert 'web_password' not in call('/api/config',authorized=True)[1]
        assert call('/api/config',{'web_password':''},authorized=True)[1]['has_web_password']
        assert call('/api/config',{'clear_web_password':True},authorized=True)[1]['has_web_password'] is False
        assert call('/api/status')[0]==200
        code, status = call('/api/status')
        assert code == 200 and status['mode'] == 'demo'
        assert len(status['display']['pixels']) == 832
        assert 'fictional-test-secret' not in json.dumps(status)
        assert call('/api/preview',{'mode':'chase'})[0]==200
        assert call('/api/demo', {'scenario':'alarm'})[0] == 200
        for _ in range(40):
            alarm = call('/api/status')[1]['alarm']
            if alarm['active']:
                break
            time.sleep(.05)
        assert alarm['active'] and alarm['critical']
        assert call('/api/setup',{})[0]==409
        assert call('/api/wifi/hotspot',{})[0]==409
        assert call('/api/wifi/connect',{'ssid':'Guest WLAN'})[0]==409
        assert call('/api/preview',{'mode':'charging'})[0]==409
        assert not call('/api/status')[1]['preview_active']
        assert call('/api/acknowledge', {})[0] == 200
        assert call('/api/status')[1]['alarm']['acknowledged']
        assert call('/api/demo', {'scenario':'offline'})[0] == 200
        status = call('/api/status')[1]
        assert status['readings']['heart_rate'] is None
        assert status['display']['screen'] == 'offline'
        for path in ['/', '/app.js', '/style.css']:
            with urllib.request.urlopen(base+path) as response:
                assert response.headers['Cache-Control'] == 'no-store'
                assert response.headers['X-Content-Type-Options'] == 'nosniff'
                assert len(response.read()) > 100
        # Real HTTP handlers, with simulated radio only. Wi-Fi secrets never
        # enter the Owlet config or the public status response.
        def wifi_until(predicate):
            for _ in range(80):
                state=call('/api/wifi')[1]
                if predicate(state): return state
                time.sleep(.05)
            raise AssertionError('Wi-Fi API state timeout')
        wifi_until(lambda w:w['phase']=='connected')
        assert call('/api/wifi/scan',{},origin='https://foreign.example')[0]==403
        assert call('/api/wifi/scan',{},request_header=False)[0]==403
        assert call('/api/wifi/connect',{'ssid':'','password':'short'})[0]==400
        assert call('/api/wifi/hotspot',{'duration_seconds':4294967306})[0]==400
        assert call('/api/wifi/scan',{})[0]==202
        wifi_until(lambda w:len(w['networks'])==2)
        before_wifi=(pathlib.Path(temp)/'config.json').read_bytes()
        assert call('/api/wifi/connect',{'ssid':'Home','password':'fictional-wifi-secret'})[0]==202
        state=wifi_until(lambda w:w['phase']=='connected' and w['ssid']=='Home')
        assert 'fictional-wifi-secret' not in json.dumps(call('/api/status')[1])
        assert call('/api/wifi/connect',{'ssid':'Missing','password':'wrong-password'})[0]==202
        state=wifi_until(lambda w:w['phase']=='connected' and w['error']=='wifi_join_failed')
        assert state['ssid']=='Home'
        assert (pathlib.Path(temp)/'config.json').read_bytes()==before_wifi
        assert call('/api/wifi/hotspot',{'duration_seconds':10})[0]==202
        wifi_until(lambda w:w['hotspot'])
        client=http.client.HTTPConnection('127.0.0.1',port,timeout=2)
        client.request('GET','/generate_204')
        response=client.getresponse()
        assert response.status==302 and response.headers['Location']=='http://192.168.4.1/'
        response.read();client.close()
        assert call('/api/wifi/cancel',{})[0]==202
        wifi_until(lambda w:w['phase']=='connected' and not w['hotspot'])
        # Combined setup saves Owlet while still connected to the hotspot.
        setup={'wifi':{'ssid':'New Home','password':'fictional-network-pass'},'account':{'email':'setup@example.invalid','password':'fictional-setup-pass','region':'eu'}}
        before_setup=(pathlib.Path(temp)/'config.json').read_bytes()
        assert call('/api/setup',{**setup,'wifi':{'ssid':''}})[0]==400
        assert (pathlib.Path(temp)/'config.json').read_bytes()==before_setup
        assert call('/api/setup',setup,origin='https://foreign.example')[0]==403
        assert call('/api/setup',setup)[0]==202
        assert call('/api/config')[1]['email']=='setup@example.invalid'
        assert call('/api/config')[1]['has_password']
        wifi_until(lambda w:w['phase']=='connected' and w['ssid']=='New Home')
        assert 'fictional-network-pass' not in (pathlib.Path(temp)/'config.json').read_text()
        assert 'fictional-setup-pass' not in json.dumps(call('/api/status')[1])
        # Timezone changes and manual display time cannot alter cloud freshness.
        assert 'Europe/Berlin' in call('/api/timezones')[1]['zones']
        assert call('/api/config',{'time':{'zone':'invalid'}})[0]==400
        assert call('/api/config',{'time':{'manual_utc':1}})[0]==400
        assert call('/api/time/resolve',{'zone':'Europe/Berlin','local':'2026-03-29T02:30'})[0]==400
        manual={'zone':'Europe/Berlin','automatic':False,'local':'2026-09-09T21:37'}
        assert call('/api/config',{'time':manual})[0]==200
        shown=call('/api/status')[1]['time']
        assert shown['local'].startswith('2026-09-09T21:37') and shown['source']=='manual'
        assert call('/api/config',{'time':{'automatic':True,'zone':'Asia/Kolkata'}})[0]==200
        assert call('/api/status')[1]['time']['zone']=='Asia/Kolkata'
        assert call('/api/status')[1]['time']['source']=='demo'
        assert call('/api/reset',{'confirm':'RESET OWLANZI'})[0]==200
        assert not call('/api/config')[1]['has_password']
        print('API integration passed (open setup, optional password, same-origin writes, preview isolation, alerts, config reset).')
    finally:
        process.terminate()
        out, err = process.communicate(timeout=10)
        assert b'fictional-test-secret' not in out + err
