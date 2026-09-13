# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise explicit system-action authorization on an offline runtime fixture."""
import base64
import json
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

with tempfile.TemporaryDirectory(prefix='owlanzi-system-api-') as directory:
    with socket.socket() as port_socket:
        port_socket.bind(('127.0.0.1', 0))
        port=port_socket.getsockname()[1]
    # Manual display time must not disable the device's network/system UTC sync.
    (Path(directory)/'config.json').write_text(json.dumps({'schema':1,'time':{'zone':'UTC','automatic':False,
        'manual_utc':1704067200,'manual_saved_utc':int(time.time())}}))
    process=subprocess.Popen([sys.argv[1],directory,str(port)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    base=f'http://127.0.0.1:{port}'
    def call(path,body=None,origin=None,header=True,authorized=False):
        headers={}
        if origin:headers['Origin']=origin
        if authorized:headers['Authorization']='Basic '+base64.b64encode(b'owlanzi:fixture-password').decode()
        if body is not None:
            headers['Content-Type']='application/json'
            if header:headers['X-Owlanzi-Request']='1'
        request=urllib.request.Request(base+path,headers=headers,data=None if body is None else json.dumps(body).encode())
        try:
            with urllib.request.urlopen(request,timeout=3) as response:return response.status,json.load(response)
        except urllib.error.HTTPError as error:return error.code,json.load(error)
    receipt=Path(directory)/'actions.txt'
    try:
        for attempt in range(100):
            try:
                code,info=call('/api/system')
                break
            except OSError:time.sleep(.05)
        else:raise AssertionError('Fixture did not start')
        assert code==200 and info=={'persistent_boot':True,'actions_supported':True}
        for attempt in range(100):
            clock=call('/api/status')[1]['time']
            if clock['synchronized']:break
            time.sleep(.05)
        else:raise AssertionError('Manual display mode disabled the platform time source')
        assert clock['automatic'] is False and clock['source']=='manual'
        assert clock['local'].startswith('2024-01-01T00:00:')
        assert (Path(directory)/'network-time.txt').read_text()=='called'
        for action,confirm in [('restore-manufacturer','RESTORE ULANZI')]:
            path='/api/system/'+action
            for body in [{},{'confirm':'yes'},{'confirm':False}]:assert call(path,body)[0]==400
            assert call(path,{'confirm':confirm},origin='https://foreign.example')[0]==403
            assert call(path,{'confirm':confirm},header=False)[0]==403
        assert not receipt.exists()
        # Settings reset must never invoke a boot action.
        assert call('/api/reset',{'confirm':'RESET OWLANZI'})[0]==200
        assert not receipt.exists()
        assert call('/api/config',{'web_password':'fixture-password'})[0]==200
        assert call('/api/system/restore-manufacturer',{'confirm':'RESTORE ULANZI'})[0]==401
        assert call('/api/system/restore-manufacturer',{'confirm':'RESTORE ULANZI'},authorized=True)[0]==202
        assert receipt.read_text().splitlines()==['restore-manufacturer']
        assert call('/api/demo',{'scenario':'alarm'},authorized=True)[0]==200
        for attempt in range(100):
            if call('/api/status',authorized=True)[1]['alarm']['critical']:break
            time.sleep(.05)
        else:raise AssertionError('Critical alarm did not activate')
        for action,confirm in [('restore-manufacturer','RESTORE ULANZI')]:
            assert call('/api/system/'+action,{'confirm':confirm},authorized=True)[0]==409
        assert receipt.read_text().splitlines()==['restore-manufacturer']
        assert call('/api/demo',{'scenario':'offline'},authorized=True)[0]==200
        for attempt in range(100):
            if not call('/api/status',authorized=True)[1]['alarm']['critical']:break
            time.sleep(.05)
        assert call('/api/system/restore-manufacturer',{'confirm':'RESTORE ULANZI'},authorized=True)[0]==202
        assert receipt.read_text().splitlines()==['restore-manufacturer','restore-manufacturer']
        print('System API passed: explicit confirmation, origin, password, alarm guard and reset isolation.')
    finally:
        process.terminate();process.communicate(timeout=10)
