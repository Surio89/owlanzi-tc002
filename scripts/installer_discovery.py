# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded, read-only discovery on active, directly attached private networks."""
from concurrent.futures import ThreadPoolExecutor, as_completed
import ipaddress
import json
import socket
import threading
import urllib.error
import urllib.request

PRIVATE=tuple(map(ipaddress.ip_network,('10.0.0.0/8','172.16.0.0/12','192.168.0.0/16')))
MAX_HOSTS=1024

def private_ip(value):
    ip=ipaddress.IPv4Address(value)
    if not any(ip in network for network in PRIVATE):raise ValueError('Use a private IPv4 address')
    return str(ip)

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs):return None

def local_request(ip,port,path,body=None,headers=None,timeout=1.0):
    url=f'http://{private_ip(ip)}:{port}{path}'
    request=urllib.request.Request(url,data=body,headers=headers or {})
    opener=urllib.request.build_opener(urllib.request.ProxyHandler({}),NoRedirect())
    with opener.open(request,timeout=timeout) as response:
        data=response.read(65537)
    if len(data)>65536:raise ValueError('Unexpected clock response')
    return data

def candidates(interfaces=None,stats=None):
    if interfaces is None:
        import psutil
        interfaces=psutil.net_if_addrs();stats=psutil.net_if_stats()
    found=set()
    for name,addresses in interfaces.items():
        if stats and (name not in stats or not stats[name].isup):continue
        # Virtual/container/VPN interfaces are not the user's home LAN.
        if name.lower().startswith(('lo','docker','veth','br-','vmnet','utun','tun','tap','wg','tailscale','vEthernet'.lower())):continue
        for address in addresses:
            if address.family!=socket.AF_INET or not address.netmask:continue
            try:
                ip=ipaddress.IPv4Address(private_ip(address.address))
                network=ipaddress.IPv4Network(f'{ip}/{address.netmask}',strict=False)
            except ValueError:continue
            # Avoid silently sweeping a corporate /16. A manual address remains
            # available when the bounded nearby range does not find the clock.
            if network.prefixlen<24:network=ipaddress.IPv4Network(f'{ip}/24',strict=False)
            for host in network.hosts():
                if host!=ip and len(found)<MAX_HOSTS:found.add(str(host))
    return sorted(found,key=ipaddress.IPv4Address)

def identify(ip,timeout=1.0):
    ip=private_ip(ip)
    for port in (8080,80):
        try:
            status=json.loads(local_request(ip,port,'/api/status',timeout=timeout))
            if isinstance(status,dict) and status.get('target')=='tc002' and status.get('mode')=='live':
                return {'ip':ip,'port':port,'kind':'owlanzi','name':'Owlanzi TC002','version':str(status.get('version',''))[:32],'protected':False}
        except urllib.error.HTTPError as error:
            if error.code==401 and 'Owlanzi TC002' in error.headers.get('WWW-Authenticate',''):
                return {'ip':ip,'port':port,'kind':'owlanzi','name':'Owlanzi TC002','version':'','protected':True}
        except (OSError,ValueError):pass
    try:
        base=json.loads(local_request(ip,80,'/getBase',timeout=timeout))
        if not isinstance(base,dict) or not {'devSn','ssid','ip','mac','mcuVer','appVer'}<=base.keys():return None
        page=local_request(ip,80,'/uclockInfo.html',timeout=timeout)
        if b'<title>Ulanzi Clock' not in page:return None
        # The full board/peripheral signature is checked before preparation.
        return {'ip':ip,'port':80,'kind':'manufacturer','name':'Ulanzi TC002','version':str(base.get('appVer',''))[:32],'protected':False}
    except (OSError,ValueError):return None

def discover(cancel=None,report=lambda *args:None,addresses=None,probe=identify):
    cancel=cancel or threading.Event()
    addresses=candidates() if addresses is None else [private_ip(ip) for ip in addresses]
    addresses=list(dict.fromkeys(addresses))[:MAX_HOSTS]
    results=[];completed=0
    def check(ip):return None if cancel.is_set() else probe(ip,timeout=.65)
    with ThreadPoolExecutor(max_workers=32) as pool:
        futures=[pool.submit(check,ip) for ip in addresses]
        for future in as_completed(futures):
            completed+=1
            try:result=future.result()
            except Exception:result=None
            if result:results.append(result)
            report(completed,len(addresses),list(results))
            if cancel.is_set():
                for pending in futures:pending.cancel()
                break
    return sorted(results,key=lambda item:ipaddress.IPv4Address(item['ip']))
