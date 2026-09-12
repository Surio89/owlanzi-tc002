# SPDX-License-Identifier: GPL-3.0-or-later
"""Drive the clock's own OTA service; never flash or resend uncertain installs."""
import re
from installer_account import ClockAccount,AccountError

BUSY={'queued','checking','downloading','switching'}

def version(value):
    if not isinstance(value,str) or not re.fullmatch(r'\d{1,6}\.\d{1,6}\.\d{1,6}',value):
        raise AccountError('invalid_update_version')
    return tuple(map(int,value.split('.')))

class ClockUpdater(ClockAccount):
    def status(self):
        state=self.request('/api/status')
        if not isinstance(state,dict) or state.get('target')!='tc002' or state.get('mode')!='live':
            raise AccountError('not_owlanzi_tc002')
        update=state.get('update') or {};wifi=state.get('wifi') or {};alarm=state.get('alarm') or {}
        if not all(isinstance(item,dict) for item in (update,wifi,alarm)):raise AccountError('invalid_clock_response')
        phase=str(update.get('phase','unavailable'))
        return {'current':str(state.get('version',''))[:32],'latest':str(update.get('latest',''))[:32],
                'phase':phase if phase in BUSY|{'idle','available','current','error','unavailable'} else 'unavailable',
                'available':update.get('available') is True,'install_supported':update.get('install_supported') is True,
                'blocked':alarm.get('critical') is True or wifi.get('connected') is not True or bool(wifi.get('hotspot')) or bool(wifi.get('busy')),
                'rolled_back':update.get('last_result')=='rolled_back'}
    def check(self):
        state=self.status()
        if state['blocked'] or state['phase'] in BUSY|{'unavailable'}:raise AccountError('update_unavailable')
        self.request('/api/update/check',{})
        return self.status()
    def install(self,expected):
        state=self.status()
        if (state['blocked'] or state['phase'] in BUSY|{'unavailable','error'} or not state['available'] or
            not state['install_supported'] or state['latest']!=expected or version(expected)<=version(state['current'])):
            raise AccountError('update_state_changed')
        self.request('/api/update/install',{'confirm':'UPDATE OWLANZI'})
        return self.status()
