# SPDX-License-Identifier: GPL-3.0-or-later
"""Collect redistributable notices and source locations before signing a bundle."""
import importlib.metadata
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import urllib.request

def collect(root, output, mksquashfs=None):
    root, output = Path(root), Path(output)
    output.mkdir()
    shutil.copytree(root/'desktop/licenses', output/'texts')
    for name in ('LICENSE', 'THIRD_PARTY_NOTICES.md'):
        shutil.copy2(root/name, output/name)
    shutil.copy2(root/'desktop/READ-ME.txt', output/'READ-ME.txt')
    components = []
    for dist in sorted(importlib.metadata.distributions(), key=lambda d:d.metadata['Name'].lower()):
        name, version = dist.metadata['Name'], dist.version
        components.append({'name':name, 'version':version,
                           'source':f'https://pypi.org/project/{name}/{version}/#files'})
        for file in dist.files or []:
            # Distribution metadata carries the upstream license notices.
            if '.dist-info/' not in file.as_posix() or not re.search(r'(license|copying|notice|authors)', file.name, re.I):
                continue
            source = Path(dist.locate_file(file))
            if source.is_file() and source.stat().st_size < 2*1024*1024:
                target = output/'python'/name/Path(*file.parts[1:])
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
    (output/'components.json').write_text(json.dumps({'python':sys.version.split()[0], 'distributions':components},indent=2)+'\n')
    if mksquashfs:
        result=subprocess.run([str(mksquashfs),'-version'],capture_output=True,text=True)
        match=re.search(r'mksquashfs version ([0-9.]+)',result.stdout)
        if not match:raise ValueError('Cannot identify bundled filesystem tool')
        version=match.group(1)
        url=f'https://codeload.github.com/plougher/squashfs-tools/tar.gz/refs/tags/{version}'
        with urllib.request.urlopen(url, timeout=60) as response:data=response.read(10*1024*1024)
        (output/f'squashfs-tools-{version}-source.tar.gz').write_bytes(data)
        (output/'squashfs-build.txt').write_text(result.stdout+'\nCorresponding source: '+url+'\nNative package: '+('Homebrew squashfs' if sys.platform=='darwin' else 'Ubuntu squashfs-tools')+'\n')
        if sys.platform=='darwin':
            cellar=Path(subprocess.check_output(['brew','--cellar'],text=True).strip())
            for package in ('squashfs','xz','lz4','zstd','lzo'):
                if not (cellar/package).is_dir():continue
                for source in (cellar/package).rglob('*'):
                    if source.is_file() and source.stat().st_size<2*1024*1024 and re.match(r'(LICENSE|COPYING|COPYRIGHT|AUTHORS)',source.name,re.I):
                        target=output/'native'/source.relative_to(cellar)
                        target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
        else:
            for package in ('squashfs-tools','liblzma5','liblz4-1','libzstd1','liblzo2-2','zlib1g','libgcc-s1','libstdc++6','libglib2.0-0','libpcre2-8-0','libffi8'):
                source=Path('/usr/share/doc')/package/'copyright'
                if source.is_file():
                    target=output/'native'/package;target.mkdir(parents=True,exist_ok=True)
                    shutil.copy2(source,target/'copyright')
            if Path('/usr/share/common-licenses').is_dir():shutil.copytree('/usr/share/common-licenses',output/'native/common-licenses')
    # Exact application/build source accompanies every binary, including macOS.
    source=output/'owlanzi-installer-source.zip'
    subprocess.run(['git','-c','safe.directory='+str(root),'archive','--format=zip','--output='+str(source),'HEAD'],cwd=root,check=True)
    (output/'SOURCE-AND-LICENSES.txt').write_text('''Owlanzi Installer is GPL-3.0-or-later, without warranty. See LICENSE.
The exact installer and build source is in owlanzi-installer-source.zip.
The TC002 app 0.3.2 has separate corresponding source and notices:
https://owlanzi.com/firmware/owlanzi-tc002-0.3.2-source.zip
The unchanged, accepted boot components come from app release 0.3.0:
https://owlanzi.com/firmware/owlanzi-tc002-0.3.0-source.zip

This app uses Qt/PySide6 and Shiboken 6.10.2 (The Qt Company Ltd. and contributors),
under LGPL-3.0, and dynamically links Qt. Full LGPL/GPL terms are included.
Modification, library replacement and reverse engineering for debugging such
modifications are permitted. Rebuild with the included requirements and build
script or replace compatible libraries in _internal/PySide6 (macOS: within
Contents/Frameworks). A modified macOS app may need local ad-hoc signing.
Complete Qt and PySide sources, including third-party attributions:
https://download.qt.io/archive/qt/6.10/6.10.2/submodules/qtbase-everywhere-src-6.10.2.tar.xz
https://download.qt.io/official_releases/QtForPython/pyside6/PySide6-6.10.2-src/
https://code.qt.io/cgit/pyside/pyside-setup.git/tag/?h=v6.10.2

Python runtime: Python Software Foundation license, see texts/Python-LICENSE.txt.
Other Python components: version/source inventory in components.json; upstream
copyright and license notices in python/. PyInstaller's GPL exception permits
distribution of bundled applications; its notices are included there.
Native packer source and native dependency notices are included when bundled.
Windows downloads its separately licensed manufacturer tools on first install;
those tools are not included in this distribution.
''',encoding='utf-8')
    return output
