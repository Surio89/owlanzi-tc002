# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the flash safety boundaries with a fake clock; no network access."""
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import installer_device as installer


class PermanentInstallTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.image = self.root / 'image'
        self.image.mkdir()
        self.original = b'original resource partition'
        self.payload = b'hsqs' + bytes(4092)
        for name, data in [('update.img', b'update'), ('restore.img', b'restore'), ('res.squashfs', self.payload)]:
            (self.image / name).write_bytes(data)
        self.receipt = {'stock_partition_sha256': installer.sha(self.original),
                        'image_sha256': installer.sha(b'update'),
                        'restore_sha256': installer.sha(b'restore'),
                        'new_squashfs_sha256': installer.sha(self.payload)}
        (self.image / 'preparation.json').write_text(json.dumps(self.receipt))
        self.readback = self.payload + bytes(0x800000 - len(self.payload))
        self.run_id = 'a' * 32
        self.guard_ready = True
        self.trigger_disconnect = False
        self.result = {'schema': 1, 'run_id': self.run_id, 'phase': 'write_verified', 'verified_size': len(self.payload),
                       'write_verified': True, 'resources_remounted': True, 'resume_restored': True, 'app_restart_requested': True}
        self.results = None
        self.events = []
        self.clock = Mock()
        self.clock.command.side_effect = self.command
        self.clock.shell.side_effect = self.shell
        self.patches = [
            patch.object(installer, 'connect', return_value=self.clock),
            patch.object(installer, 'active_safe'),
            patch.object(installer, 'check_upgrader'),
            patch.object(installer.device, 'verified_bundle', return_value=(self.root, {'version': '0.3.0'})),
            patch.object(installer, 'stage_app', return_value='b'),
            patch.object(installer, 'transfer', side_effect=self.transfer),
            patch.object(installer.time, 'sleep'),
            patch.object(installer.secrets, 'token_hex', return_value=self.run_id),
            patch.object(installer, 'install_connection', return_value=True),
        ]
        self.mocks = [p.start() for p in self.patches]
        self.addCleanup(self.temp.cleanup)
        for p in self.patches:
            self.addCleanup(p.stop)

    def command(self, operation, remote, local):
        self.assertEqual((operation, remote), ('pull', '/dev/block/mtdblock3'))
        self.events.append(Path(local).name)
        Path(local).write_bytes(self.original if Path(local).name == 'res-before.bin' else self.readback)

    def shell(self, text, check=True):
        self.events.append(text)
        if text == 'setprop ctl.restart zkswe' and self.trigger_disconnect:
            raise RuntimeError('ADB shell did not return a remote exit status')
        state = ''
        if text == 'cat /tmp/owlanzi-permanent-install/guard-ready':
            state = self.run_id if self.guard_ready else ''
        if text == 'cat /data/owlanzi-app/install-result.json':
            state = json.dumps(next(self.results) if self.results is not None else self.result)
        return SimpleNamespace(stdout=state, returncode=0)

    def transfer(self, clock, local, remote, work):
        self.events.append(remote)
        if remote.endswith('/zkrebootdelay'):
            self.assertEqual(Path(local).read_text().strip(), '-1')

    def install(self):
        return installer.install('192.168.1.2', 'fake-adb', self.root, self.root,
                                 self.image, self.root / 'work', None, report=lambda _: None)

    def test_success_requires_completed_write_and_full_readback_before_power_cycle(self):
        result = self.install()
        self.assertTrue(result['write_verified'])
        self.assertTrue(result['power_cycle_required'])
        self.assertEqual(result['target_slot'], 'b')
        reboot_flag = '/tmp/owlanzi-permanent-install/zkrebootdelay'
        trigger = 'setprop ctl.restart zkswe'
        self.assertLess(self.events.index(reboot_flag), self.events.index(trigger))
        self.assertLess(self.events.index('setprop sys.zkupgrade.state -1'), self.events.index(trigger))
        self.assertLess(self.events.index('/tmp/owlanzi-permanent-install/install-guard --watch'), self.events.index(trigger))
        self.assertEqual(self.events[-1], 'res-written.bin')
        self.assertTrue((self.root / 'work/write-verified.json').exists())
        self.assertFalse(any(event in ('reboot', 'setprop ctl.reboot') for event in self.events))

    def test_changed_clock_stops_before_app_or_flash(self):
        self.original = b'changed'
        with self.assertRaisesRegex(ValueError, 'Clock changed'):
            self.install()
        self.mocks[4].assert_not_called()
        self.assertNotIn('setprop ctl.restart zkswe', self.events)

    def test_damaged_image_stops_before_app_or_flash(self):
        for name in ('update.img', 'restore.img', 'res.squashfs'):
            with self.subTest(name=name):
                path = self.image / name
                original = path.read_bytes()
                path.write_bytes(original + b'changed')
                with self.assertRaisesRegex(ValueError, 'image changed'):
                    self.install()
                path.write_bytes(original)
        self.mocks[4].assert_not_called()
        self.assertNotIn('setprop ctl.restart zkswe', self.events)

    def test_clock_side_failure_never_reports_success(self):
        self.result['phase'] = 'upgrade_failed'
        with self.assertRaisesRegex(RuntimeError, 'verification failed'):
            self.install()
        self.assertFalse((self.root / 'work/write-verified.json').exists())
        self.assertNotIn('res-written.bin', self.events)

    def test_timeout_never_reports_success_or_retriggers_flash(self):
        with patch.object(installer.time, 'monotonic', side_effect=[0, 301]):
            with self.assertRaisesRegex(RuntimeError, 'not confirmed'):
                self.install()
        self.assertEqual(self.events.count('setprop ctl.restart zkswe'), 1)
        self.assertFalse((self.root / 'work/write-verified.json').exists())

    def test_corrupt_or_truncated_readback_never_allows_power_cycle(self):
        for data in (b'wrong' + self.readback[5:], self.readback[:-1]):
            with self.subTest(size=len(data)):
                self.readback = data
                with self.assertRaisesRegex(RuntimeError, 'verification failed'):
                    self.install()
                self.assertFalse((self.root / 'work/write-verified.json').exists())

    def test_verifier_must_be_ready_before_upgrade_is_triggered(self):
        self.guard_ready = False
        with self.assertRaisesRegex(RuntimeError, 'verifier did not start'):
            self.install()
        self.assertNotIn('setprop ctl.restart zkswe', self.events)

    def test_expected_wlan_loss_and_stale_receipt_do_not_retrigger_upgrade(self):
        self.mocks[-1].side_effect = [False, False, True, True]
        self.results = iter([{**self.result, 'run_id': 'b' * 32}, self.result])
        self.assertTrue(self.install()['write_verified'])
        self.assertEqual(self.events.count('setprop ctl.restart zkswe'), 1)
        self.assertEqual(self.events.count('cat /data/owlanzi-app/install-result.json'), 2)

    def test_stale_receipt_cannot_confirm_this_installation(self):
        self.result['run_id'] = 'b' * 32
        with patch.object(installer.time, 'monotonic', side_effect=[0, 1, 301]):
            with self.assertRaisesRegex(RuntimeError, 'not confirmed'):
                self.install()
        self.assertNotIn('res-written.bin', self.events)

    def test_connection_lost_during_trigger_waits_for_matching_proof_without_retry(self):
        self.trigger_disconnect = True
        self.assertTrue(self.install()['write_verified'])
        self.assertEqual(self.events.count('setprop ctl.restart zkswe'), 1)

    def test_wrong_verified_size_or_mismatch_is_rejected(self):
        for change in ({'verified_size': 1}, {'phase': 'readback_mismatch'}):
            with self.subTest(change=change):
                original = dict(self.result)
                self.result.update(change)
                with self.assertRaisesRegex(RuntimeError, 'verification failed'):
                    self.install()
                self.result = original
                self.assertNotIn('res-written.bin', self.events)

    def test_write_alone_cannot_confirm_restored_application(self):
        for key in ('write_verified', 'resources_remounted', 'resume_restored', 'app_restart_requested'):
            for value in (None, False, 1, 'true'):
                with self.subTest(key=key, value=value):
                    original = dict(self.result)
                    if value is None:
                        self.result.pop(key)
                    else:
                        self.result[key] = value
                    with self.assertRaisesRegex(RuntimeError, 'verification failed'):
                        self.install()
                    self.result = original
                    self.assertNotIn('res-written.bin', self.events)
                    self.assertFalse((self.root / 'work/write-verified.json').exists())


if __name__ == '__main__':
    unittest.main()
