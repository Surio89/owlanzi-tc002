# SPDX-License-Identifier: GPL-3.0-or-later
"""Offline desktop tests: local discovery bounds, selected-clock credentials, image compatibility."""
from pathlib import Path
import hashlib
import json
import socket
import struct
import sys
import threading
from types import SimpleNamespace
import unittest
import urllib.error
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
import installer_discovery as discovery
from installer_account import ClockAccount,AccountError
from installer_container import wrap_res,unwrap_res
from installer_update import ClockUpdater

class DiscoveryTests(unittest.TestCase):
    def test_only_active_local_networks_with_a_hard_bound(self):
        def addr(ip,mask):return SimpleNamespace(family=socket.AF_INET,address=ip,netmask=mask)
        interfaces={'Ethernet':[addr('192.168.100.20','255.255.0.0')], 'Wi-Fi':[addr('10.1.2.5','255.255.255.0')],
                    'docker0':[addr('172.17.0.1','255.255.0.0')], 'public':[addr('8.8.8.8','255.255.255.0')],
                    'offline':[addr('192.168.4.1','255.255.255.0')]}
        stats={k:SimpleNamespace(isup=k!='offline') for k in interfaces}
        result=discovery.candidates(interfaces,stats)
        self.assertEqual(len(result),506);self.assertIn('192.168.100.235',result)
        self.assertNotIn('192.168.99.2',result);self.assertNotIn('192.168.100.20',result)
        self.assertTrue(all(ip.startswith(('192.168.100.','10.1.2.')) for ip in result))
    def test_probe_is_read_only_and_does_not_expose_stock_network_details(self):
        base={'devSn':'private-serial','ssid':'private-wifi','ip':'192.168.1.2','mac':'private-mac','mcuVer':'1','appVer':'1.0.1'}
        with patch.object(discovery,'local_request',side_effect=[ValueError(),ValueError(),json.dumps(base).encode(),b'<title>Ulanzi Clock</title>']) as request:
            result=discovery.identify('192.168.1.2')
        self.assertEqual(result['kind'],'manufacturer');self.assertNotIn('private-',json.dumps(result))
        self.assertTrue(all(len(call.args)==3 for call in request.call_args_list))
    def test_unrelated_and_demo_servers_are_not_clocks(self):
        for payload in ({'target':'tc001','mode':'live'},{'target':'tc002','mode':'demo'},{}):
            with patch.object(discovery,'local_request',return_value=json.dumps(payload).encode()):self.assertIsNone(discovery.identify('192.168.1.2'))
    def test_cancellation_and_multiple_clocks(self):
        cancelled=threading.Event();cancelled.set()
        with patch.object(discovery,'identify') as probe:
            self.assertEqual(discovery.discover(cancelled,addresses=['192.168.1.2'],probe=probe),[]);probe.assert_not_called()
        result=discovery.discover(addresses=['192.168.1.3','192.168.1.2','192.168.1.3'],probe=lambda ip,timeout:{'ip':ip})
        self.assertEqual([r['ip'] for r in result],['192.168.1.2','192.168.1.3'])
    def test_nonlocal_and_redirect_targets_are_rejected(self):
        for address in ('127.0.0.1','8.8.8.8','localhost','192.168.1.2;reboot','::1'):
            with self.assertRaises(ValueError):discovery.private_ip(address)
        self.assertIsNone(discovery.NoRedirect().redirect_request(None,None,None,None,None,None))

    def test_stock_103_canonical_route_and_unknown_route_redirect(self):
        calls=[]
        def endpoint(ip,port,path,**kwargs):
            calls.append((port,path))
            if port==8080:raise OSError('closed')
            if path=='/getBase':return json.dumps({'devSn':'private','ssid':'private','ip':ip,'mac':'private','mcuVer':'V1.0.16','appVer':'1.0.3'}).encode()
            if path=='/settings/general':return b'<!doctype html><title>Ulanzi Clock - Settings</title><script src="/settings/assets/common.js"></script>'
            raise urllib.error.HTTPError('http://'+ip+path,301,'Moved',{'Location':'/settings/general'},None)
        with patch.object(discovery,'local_request',side_effect=endpoint):
            result=discovery.identify('192.168.1.2')
            self.assertEqual(discovery.discover(addresses=['192.168.1.2']),[result])
        self.assertEqual((result['kind'],result['version']),('manufacturer','1.0.3'))
        self.assertIn((80,'/settings/general'),calls)
        self.assertNotIn('private',json.dumps(result))

    def test_old_static_route_still_supported_but_generic_server_rejected(self):
        base=json.dumps({'devSn':'','ssid':'','ip':'','mac':'','mcuVer':'1','appVer':'1.0.1'}).encode()
        with patch.object(discovery,'local_request',side_effect=[OSError(),OSError(),base,OSError(),b'<title>Ulanzi Clock - Info</title>']):
            self.assertEqual(discovery.identify('192.168.1.2')['kind'],'manufacturer')
        with patch.object(discovery,'local_request',side_effect=[OSError(),OSError(),base,b'<title>Router</title>',b'<title>Router</title>',b'<title>Router</title>']):
            self.assertIsNone(discovery.identify('192.168.1.2'))

    def test_existing_owlanzi_on_either_port_and_protected_clock(self):
        payload=b'{"target":"tc002","mode":"live","version":"0.3.0"}'
        for replies,port in (([payload],8080),([OSError(),payload],80)):
            with patch.object(discovery,'local_request',side_effect=replies):
                result=discovery.identify('192.168.1.2');self.assertEqual(result['kind'],'owlanzi');self.assertEqual(result['port'],port)
        error=urllib.error.HTTPError('',401,'Unauthorized',{'WWW-Authenticate':'Basic realm="Owlanzi TC002"'},None)
        with patch.object(discovery,'local_request',side_effect=error):self.assertTrue(discovery.identify('192.168.1.2')['protected'])

    def test_local_area_connection_and_pasted_webui_address(self):
        addr=SimpleNamespace(family=socket.AF_INET,address='192.168.1.2',netmask='255.255.255.0')
        self.assertEqual(len(discovery.candidates({'Local Area Connection':[addr]})),253)
        for value in ('192.168.1.2','http://192.168.1.2/','http://192.168.1.2:8080/settings/general'):
            self.assertEqual(discovery.clock_address(value),'192.168.1.2')
        for value in ('https://192.168.1.2','http://user:pass@192.168.1.2','http://example.com','http://192.168.1.2:22'):
            with self.assertRaises(ValueError):discovery.clock_address(value)

class AccountTests(unittest.TestCase):
    def test_reidentifies_and_sends_only_to_selected_clock(self):
        client=ClockAccount('192.168.1.2',8080,'local-only')
        status={'target':'tc002','mode':'live','cloud_fresh':False,'devices':[]}
        with patch('installer_account.local_request',side_effect=[json.dumps(status).encode(),b'{}']) as call:
            client.configure('example@example.test','secret-fixture','eu')
        self.assertEqual(call.call_args_list[0].args[:3],('192.168.1.2',8080,'/api/status'))
        ip,port,path,body,headers=call.call_args.args
        self.assertEqual((ip,port,path),('192.168.1.2',8080,'/api/config'))
        self.assertEqual(headers['Origin'],'http://192.168.1.2:8080')
        self.assertEqual(headers['X-Owlanzi-Request'],'1')
        self.assertEqual(json.loads(body)['password'],'secret-fixture');self.assertNotIn('secret-fixture',repr(client.__dict__))
        client.close();self.assertEqual(client.web_password,'')
    def test_foreign_endpoint_cannot_receive_credentials(self):
        client=ClockAccount('192.168.1.2')
        with patch('installer_account.local_request',return_value=b'{"target":"tc001","mode":"live"}') as call:
            with self.assertRaises(AccountError):client.configure('a@b.test','secret-fixture','eu')
        self.assertEqual(call.call_count,1)
    def test_status_and_errors_do_not_expose_unexpected_server_secrets(self):
        payload={'target':'tc002','mode':'live','devices':[],'password':'secret-fixture','last_error':'secret-fixture'}
        with patch('installer_account.local_request',return_value=json.dumps(payload).encode()):
            result=ClockAccount('192.168.1.2').status()
        self.assertNotIn('secret-fixture',json.dumps(result));self.assertTrue(result['has_error'])
        with patch('installer_account.local_request',side_effect=RuntimeError('secret-fixture')):
            with self.assertRaisesRegex(AccountError,'clock_unreachable'):ClockAccount('192.168.1.2').status()
    def test_unknown_sock_and_invalid_account_are_not_written(self):
        client=ClockAccount('192.168.1.2')
        with patch.object(client,'status',return_value={'devices':[{'serial':'known'}]}),patch.object(client,'request') as request:
            with self.assertRaises(AccountError):client.choose_device('unknown')
            with self.assertRaises(AccountError):client.configure('a@b.test','p','invalid')
            request.assert_not_called()

class UpdateTests(unittest.TestCase):
    def state(self,**changes):
        return {'target':'tc002','mode':'live','version':'0.2.4','wifi':{'connected':True},'alarm':{},
                'update':{'phase':'available','latest':'0.3.0','available':True,'install_supported':True,**changes}}
    def test_check_uses_clock_api_and_install_requires_matching_newer_version(self):
        client=ClockUpdater('192.168.1.2')
        with patch.object(client,'request',side_effect=[self.state(),{'accepted':True},self.state()]) as request:
            client.check();self.assertEqual(request.call_args_list[1].args,('/api/update/check',{}))
        with patch.object(client,'request',side_effect=[self.state(),{'accepted':True},self.state(phase='queued')]) as request:
            client.install('0.3.0');self.assertEqual(request.call_args_list[1].args,('/api/update/install',{'confirm':'UPDATE OWLANZI'}))
    def test_no_downgrade_stale_release_busy_or_unsupported_installs(self):
        client=ClockUpdater('192.168.1.2')
        for changes,expected in [({},'0.3.1'),({'latest':'0.2.3'},'0.2.3'),({'available':False},'0.3.0'),({'install_supported':False},'0.3.0'),({'phase':'downloading'},'0.3.0')]:
            with patch.object(client,'request',return_value=self.state(**changes)) as request:
                with self.assertRaises(AccountError):client.install(expected)
                self.assertEqual(request.call_count,1)
    def test_alarm_foreign_target_and_missing_network_cannot_update(self):
        client=ClockUpdater('192.168.1.2')
        for change in ({'target':'tc001'},{'mode':'demo'},{'wifi':{}},{'alarm':{'critical':True}}):
            with patch.object(client,'request',return_value={**self.state(),**change}) as request:
                with self.assertRaises(AccountError):client.install('0.3.0')
                self.assertEqual(request.call_count,1)
    def test_uncertain_install_reply_is_never_retried(self):
        client=ClockUpdater('192.168.1.2')
        with patch.object(client,'request',side_effect=[self.state(),AccountError('clock_unreachable')]) as request:
            with self.assertRaises(AccountError):client.install('0.3.0')
            self.assertEqual(request.call_count,2)

class ContainerTests(unittest.TestCase):
    def test_legacy_vendor_fixture_is_identical_with_its_padding(self):
        fixture=(ROOT/'tests/fixtures/res-container.bin').read_bytes();payload=bytes(range(64))
        self.assertEqual(unwrap_res(fixture),payload)
        self.assertEqual(wrap_res(payload,padding=fixture[59:568]),fixture)
    def test_payload_header_and_partition_tampering_are_detected(self):
        image=wrap_res(bytes(range(256))*32)
        for offset in (0,20,28,32,52,53,57,567,568,572,600,len(image)-1):
            changed=bytearray(image);changed[offset]^=1
            with self.subTest(offset=offset),self.assertRaises(ValueError):unwrap_res(changed)
        with self.assertRaises(ValueError):unwrap_res(image+b'extra')
        with self.assertRaises(ValueError):wrap_res(b'short')
    def test_full_partition_round_trip(self):
        payload=bytes(range(256))*(0x800000//256)
        self.assertEqual(unwrap_res(wrap_res(payload)),payload)

if __name__=='__main__':unittest.main()
