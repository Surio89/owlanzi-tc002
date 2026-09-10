#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Explicit local TC002 development operations. Never flashes an image.

No network or device operations happen on import or when building the app.
Unknown model responses fail closed until reviewed on actual hardware.
"""
import argparse
import hashlib
import ipaddress
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
import urllib.request
import urllib.error

ROOT = Path(__file__).resolve().parents[1]
REMOTE = '/tmp/owlanzi-tc002-app'
ACTIVE = '/tmp/EasyUI.cfg'
PAYLOAD = {'EasyUI.cfg', 'lib/libzkgui.so', 'ui/main.ftu', 'ui/cacert.pem'}
MODEL_KEYS = {'model', 'device_model', 'devicemodel', 'productmodel', 'product_model'}


def private_ip(value):
    ip = ipaddress.ip_address(value)
    allowed = ('10.0.0.0/8', '172.16.0.0/12', '192.168.0.0/16')
    if ip.version != 4 or not any(ip in ipaddress.ip_network(net) for net in allowed):
        raise ValueError('Use an explicit IPv4 address in your local private network')
    return str(ip)


def verified_bundle(directory):
    directory = Path(directory).resolve()
    manifest = json.loads((directory / 'manifest.json').read_text(encoding='utf-8'))
    if (manifest.get('schema') != 1 or manifest.get('target') != 'tc002' or
            manifest.get('kind') != 'flythings-debug-app' or manifest.get('hardware_verified') is not False):
        raise ValueError('This helper requires a local TC002 development bundle')
    files = manifest.get('files', {})
    if set(files) != PAYLOAD:
        raise ValueError('Unexpected or missing payload files')
    for relative, expected in files.items():
        original = directory / relative
        path = original.resolve()
        if not path.is_relative_to(directory) or original.is_symlink() or not path.is_file():
            raise ValueError('Payload escapes its bundle')
        data = path.read_bytes()
        if len(data) > 32 * 1024 * 1024 or expected.get('size') != len(data) or expected.get('sha256') != hashlib.sha256(data).hexdigest():
            raise ValueError('Payload checksum or size mismatch: ' + relative)
    cfg = (directory / 'EasyUI.cfg').read_text(encoding='utf-8')
    if not own_config(cfg):
        raise ValueError('Debug configuration does not point at the isolated Owlanzi directory')
    image = (directory / 'lib/libzkgui.so').read_bytes()
    if image[:5] != b'\x7fELF\x01' or image[5] != 1 or int.from_bytes(image[18:20], 'little') != 40:
        raise ValueError('Application is not an ELF32 little-endian ARM image')
    return directory, manifest


def elf_symbols(tool, path, undefined):
    output = subprocess.check_output([str(tool), '--dyn-syms', '--wide', str(path)], text=True, timeout=20)
    symbols = set()
    for line in output.splitlines():
        parts = line.split()
        if len(parts) < 8 or not parts[0].endswith(':') or not parts[0][:-1].isdigit():
            continue
        if undefined:
            if parts[6] != 'UND' or parts[4] == 'WEAK':
                continue
        elif parts[6] == 'UND' or parts[4] not in {'GLOBAL', 'WEAK'}:
            continue
        name = parts[7].replace('@@', '@')
        symbols.add(name)
        if not undefined:
            symbols.add(name.split('@')[0])
    return symbols


def own_config(text):
    # EasyUI configuration in this SDK is JSON. Compare semantic paths rather
    # than accepting a substring in a comment or an unrelated setting.
    try:
        cfg = json.loads(text)
        return cfg.get('startupLibPath') == REMOTE + '/lib/libzkgui.so' and cfg.get('resPath') == REMOTE + '/ui/'
    except (ValueError, AttributeError):
        return False


def model_labels(value):
    labels = []
    if isinstance(value, dict):
        for key, item in value.items():
            if key.lower() in MODEL_KEYS and isinstance(item, str):
                labels.append(item[:120])
            elif isinstance(item, (dict, list)):
                labels.extend(model_labels(item))
    elif isinstance(value, list):
        for item in value:
            labels.extend(model_labels(item))
    return labels


class Device:
    def __init__(self, ip, adb):
        self.ip = private_ip(ip)
        self.adb = adb
        self.serial = self.ip + ':5555'

    def command(self, *args, check=True):
        result = subprocess.run([self.adb, '-s', self.serial, *args], capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=45)
        if check and result.returncode:
            raise RuntimeError('ADB operation failed; verify device connectivity')
        return result

    def shell(self, command, check=True):
        # Stock adbd uses shell v1: the host exit code does not report remote
        # failures. mksh has echo/test, but no uname or sha256sum applets.
        result = self.command('shell', command + '; owlanzi_rc=$?; echo; echo OWLANZI_EXIT=$owlanzi_rc', check=False)
        match = re.search(r'(?:^|\n)OWLANZI_EXIT=(\d+)\s*$', result.stdout)
        if not match:
            raise RuntimeError('ADB shell did not return a remote exit status')
        result.returncode = result.returncode or int(match.group(1))
        result.stdout = result.stdout[:match.start()].rstrip('\r\n')
        if check and result.returncode:
            raise RuntimeError('Remote device command failed: ' + command)
        return result

    def identify(self, require_model):
        with urllib.request.urlopen('http://' + self.ip + '/getBase', timeout=5) as response:
            payload = response.read(65537)
        if len(payload) > 65536:
            raise RuntimeError('Unexpected device response size')
        base = json.loads(payload)
        labels = model_labels(base)
        result = subprocess.run([self.adb, 'connect', self.serial], capture_output=True, text=True, timeout=15)
        if result.returncode:
            raise RuntimeError('WLAN ADB is not reachable')
        cpu = self.shell('cat /proc/cpuinfo').stdout
        architecture = 'armv7l' if re.search(r'model name\s*:\s*ARMv7 Processor', cpu) else 'unknown'
        props = {}
        for name in ['ro.product.model', 'ro.product.board', 'ro.hardware', 'ro.app.name', 'ro.app.version']:
            props[name] = self.shell('getprop ' + name, check=False).stdout.strip()[:120]
        # Never log the raw getBase response; it may contain network settings.
        print(json.dumps({'http_model_labels': labels, 'architecture': architecture, 'system': props}, indent=2))
        identified = any(re.search(r'\bTC002\b', label, re.I) for label in labels)
        # TC002 stock app 1.0.1 omits a model field. Match its combined HTTP,
        # board, static manufacturer UI and peripheral signature instead.
        if not identified and isinstance(base, dict) and {'devSn', 'ssid', 'ip', 'mac', 'mcuVer', 'appVer'} <= base.keys():
            board = (props['ro.product.model'] == 'Zkswe_SSD21X_SPINOR' and
                     props['ro.product.board'] == 'swaio' and
                     props['ro.hardware'] == 'sstarsoc(flatteneddevicetree)')
            peripherals = self.shell('test -e /dev/spidev0.0 && test -e /dev/ttyS1 && test -e /dev/input/event67 && test -e /dev/input/event68', check=False).returncode == 0
            ui = self.shell('cat /res/ui/web/uclockInfo.html', check=False).stdout
            identified = board and peripherals and '<title>Ulanzi Clock' in ui
            if identified:
                print('Recognized TC002 stock firmware signature (Z21 / Ulanzi Clock / MCU / SPI / input).')
        if require_model and (not identified or architecture not in {'armv7l', 'armv7', 'armv7a'}):
            raise RuntimeError('Unrecognized hardware identity; inspect the real model fields before enabling this device')

    def active_config(self):
        result = self.shell('cat ' + ACTIVE, check=False)
        if result.returncode:
            if self.shell('test -e ' + ACTIVE, check=False).returncode == 0:
                raise RuntimeError('Existing debug configuration cannot be read')
            return None
        return result.stdout

    def preflight(self, bundle, manifest, readelf):
        if not Path(readelf).is_file():
            raise RuntimeError('Build the TC002 toolchain first, or provide --readelf for device ABI checks')
        required = manifest.get('needed_sonames')
        if not isinstance(required, list) or not 1 <= len(required) <= 64 or any(not isinstance(name, str) or not re.fullmatch(r'lib[A-Za-z0-9_.+-]+|ld-linux-armhf\.so\.3', name) for name in required):
            raise ValueError('Missing or invalid required library list')
        output = ROOT / '.local' / ('device-' + self.ip) / 'abi'
        output.mkdir(parents=True, exist_ok=True)
        exports = set()
        for name in required:
            location = None
            for directory in ['/res/lib', '/lib', '/usr/lib']:
                candidate = directory + '/' + name
                if self.shell('test -f ' + candidate, check=False).returncode == 0:
                    location = candidate
                    break
            if not location:
                raise RuntimeError('Device is missing required library: ' + name)
            target = output / name
            self.command('pull', location, str(target))
            exports.update(elf_symbols(readelf, target, undefined=False))
        missing = elf_symbols(readelf, bundle / 'lib/libzkgui.so', undefined=True) - exports
        if missing:
            # These are public ELF symbol names, never settings or credentials.
            raise RuntimeError('Device ABI is incompatible with the app: ' + ', '.join(sorted(missing)[:12]))
        print('Device libraries satisfy the application\'s direct strong ELF imports. Hardware operation remains unverified.')

    def run(self, bundle, manifest):
        if self.active_config() is not None:
            raise RuntimeError('A debug application is already active. Restore it before starting another test')
        self.shell('mkdir -p ' + REMOTE + '/lib ' + REMOTE + '/ui')
        for relative, expected in manifest['files'].items():
            remote_path = REMOTE + '/' + relative
            self.command('push', str(bundle / relative), remote_path)
            # Read back the bytes: stock firmware has no checksum utility.
            with tempfile.TemporaryDirectory(prefix='owlanzi-verify-') as temp:
                copy = Path(temp) / 'payload'
                self.command('pull', remote_path, str(copy))
                if hashlib.sha256(copy.read_bytes()).hexdigest() != expected['sha256']:
                    raise RuntimeError('Device checksum mismatch; application was not activated')
        self.command('push', str(bundle / 'EasyUI.cfg'), ACTIVE)
        try:
            self.shell('setprop ctl.restart zkswe')
            self.wait_for_startup()
        except (OSError, ValueError, RuntimeError, subprocess.SubprocessError):
            # A failed first run must not leave the device in a restart loop.
            self.restore()
            raise

    def wait_for_startup(self):
        for _ in range(20):
            try:
                with urllib.request.urlopen('http://' + self.ip + ':8080/api/status', timeout=2) as response:
                    status = json.load(response)
                if status.get('target') == 'tc002' and status.get('mode') == 'live':
                    print('Owlanzi responds at http://' + self.ip + ':8080 (no setup key required)')
                    return
            except urllib.error.HTTPError as error:
                if error.code == 401 and 'Owlanzi TC002' in error.headers.get('WWW-Authenticate', ''):
                    print('Owlanzi responds; sign in with your optional web password at http://' + self.ip + ':8080')
                    return
            except OSError:
                pass
            time.sleep(1)
        raise RuntimeError('Application did not pass its HTTP startup check')

    def restore(self):
        current = self.active_config()
        if current is None:
            print('No temporary debug application is active.')
            return
        if not own_config(current):
            raise RuntimeError('Refusing to remove a different application\'s debug configuration')
        self.shell('rm -f ' + ACTIVE)
        self.shell('setprop ctl.restart zkswe')
        # Remove only our fixed allowlisted files; never recursively erase /tmp.
        for relative in sorted(PAYLOAD):
            self.shell('rm -f ' + REMOTE + '/' + relative)
        for relative in ['/lib', '/ui', '']:
            self.shell('rmdir ' + REMOTE + relative, check=False)
        print('Original application restart requested. Check the clock visually; account settings under /data/owlanzi were preserved.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='action', required=True)
    for name in ['inspect', 'run', 'restore']:
        command = commands.add_parser(name)
        command.add_argument('--ip', required=True, type=private_ip)
        command.add_argument('--adb', default=shutil.which('adb') or 'adb')
        if name == 'run':
            command.add_argument('--bundle', type=Path, default=ROOT / 'build/tc002/device')
            command.add_argument('--readelf', type=Path, default=ROOT / '.cache/tooling/z21/bin/arm-pc-linux-gnueabihf-readelf.exe')
    args = parser.parse_args()
    bundle = manifest = None
    if args.action == 'run':
        bundle, manifest = verified_bundle(args.bundle)  # fail before any network activity
    device = Device(args.ip, args.adb)
    # Stock HTTP disappears while our application runs. Restoration uses the
    # exact own-config marker over the same explicit ADB target instead.
    if args.action != 'restore':
        device.identify(require_model=args.action == 'run')
    if args.action == 'run':
        device.preflight(bundle, manifest, args.readelf)
        device.run(bundle, manifest)
    elif args.action == 'restore':
        subprocess.run([args.adb, 'connect', device.serial], capture_output=True, timeout=15, check=True)
        device.restore()


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        raise SystemExit('Local test stopped: ' + str(error))
