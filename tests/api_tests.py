#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the real compiled server, with simulation only and no cloud calls."""
import json
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
    process = subprocess.Popen([sys.argv[1], '--demo', '--data-dir', temp, '--port', str(port), '--duration', '30'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
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
        token = token_file.read_text()
        assert len(token) == 64
        def call(path, payload=None, authorized=True, request_header=True):
            headers = {'Authorization': 'Bearer ' + token} if authorized else {}
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
        assert call('/api/config', authorized=False)[0] == 401
        assert call('/api/status', authorized=False)[0] == 401
        assert call('/api/acknowledge', {}, request_header=False)[0] == 403
        code, config = call('/api/config')
        assert code == 200 and 'password' not in config and not config['alarms']['enabled']
        code, config = call('/api/config', {'email':'example@example.invalid','password':'fictional-test-secret','brightness':42})
        assert code == 200 and config['has_password'] and 'password' not in config
        assert call('/api/config', {'password':''})[1]['has_password']
        before = (pathlib.Path(temp)/'config.json').read_bytes()
        assert call('/api/config', {'poll_interval_seconds':1})[0] == 400
        assert (pathlib.Path(temp)/'config.json').read_bytes() == before
        assert call('/api/config', {'clear_password':True})[1]['has_password'] is False
        code, status = call('/api/status')
        assert code == 200 and status['mode'] == 'demo'
        assert len(status['display']['pixels']) == 832
        assert 'fictional-test-secret' not in json.dumps(status)
        assert call('/api/demo', {'scenario':'alarm'})[0] == 200
        for _ in range(40):
            alarm = call('/api/status')[1]['alarm']
            if alarm['active']:
                break
            time.sleep(.05)
        assert alarm['active'] and alarm['critical']
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
        print('API integration passed (pairing, CSRF headers, secrets, atomic config, simulation, acknowledgement, offline display).')
    finally:
        process.terminate()
        out, err = process.communicate(timeout=10)
        assert b'fictional-test-secret' not in out + err
