# SPDX-License-Identifier: GPL-3.0-or-later
"""Configure only a selected Owlanzi TC002. No host-side credential persistence."""
import base64
import json
import urllib.error
from installer_discovery import private_ip,local_request

class AccountError(ValueError):pass

class ClockAccount:
    def __init__(self,ip,port=8080,web_password=''):
        self.ip=private_ip(ip)
        if port not in (80,8080):raise AccountError('Invalid clock port')
        self.port=port;self.web_password=web_password
    def close(self):self.web_password=''
    def request(self,path,body=None):
        headers={'Content-Type':'application/json','Origin':f'http://{self.ip}:{self.port}'}
        if self.web_password:headers['Authorization']='Basic '+base64.b64encode(('owlanzi:'+self.web_password).encode()).decode()
        data=None if body is None else json.dumps(body,ensure_ascii=False).encode()
        try:return json.loads(local_request(self.ip,self.port,path,data,headers,timeout=8))
        except urllib.error.HTTPError as error:
            if error.code==401:raise AccountError('web_password_required') from None
            raise AccountError('clock_rejected_settings') from None
        except Exception:raise AccountError('clock_unreachable') from None
    def status(self):
        status=self.request('/api/status')
        if not isinstance(status,dict) or status.get('target')!='tc002' or status.get('mode')!='live':raise AccountError('not_owlanzi_tc002')
        # Never return tokens, full server errors or unrelated configuration.
        devices=status.get('devices',[])
        if not isinstance(devices,list):raise AccountError('invalid_clock_response')
        return {'version':str(status.get('version',''))[:32],'cloud_fresh':status.get('cloud_fresh') is True,
                'has_error':bool(status.get('last_error')),
                'devices':[{'serial':str(item.get('serial',''))[:128],'name':str(item.get('name','Owlet'))[:128]} for item in devices[:32] if isinstance(item,dict)]}
    def configure(self,email,password,region,language='de',serial=''):
        if not isinstance(email,str) or not 3<=len(email)<=254 or '@' not in email:raise AccountError('invalid_email')
        if not isinstance(password,str) or not 1<=len(password)<=512:raise AccountError('invalid_password')
        if region not in ('eu','world') or language not in ('de','en'):raise AccountError('invalid_region')
        if not isinstance(serial,str) or len(serial)>128:raise AccountError('invalid_device')
        self.status()  # Reidentify immediately before transmitting account data.
        self.request('/api/config',{'email':email.strip(),'password':password,'region':region,'language':language,'device_serial':serial})
    def choose_device(self,serial):
        status=self.status();devices=status.get('devices') or []
        if not serial or not any(item.get('serial')==serial for item in devices if isinstance(item,dict)):raise AccountError('invalid_device')
        self.request('/api/config',{'device_serial':serial})
