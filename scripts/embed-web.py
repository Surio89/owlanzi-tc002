#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Embed the actual offline UI in both the desktop and TC002 application."""
import pathlib
import sys
root = pathlib.Path(sys.argv[1])
output = pathlib.Path(sys.argv[2])
output.parent.mkdir(parents=True, exist_ok=True)
parts = ['// Generated from web/. Do not edit.\n#pragma once\nnamespace owlanzi {\n']
licenses=[root.parent/'LICENSE',root.parent/'THIRD_PARTY_NOTICES.md',*sorted((root.parent/'vendor').glob('LICENSE-*')), *sorted((root.parent/'vendor/licenses').glob('*.txt'))]
assets=[('index.html','WebIndex'),('app.js','WebScript'),('style.css','WebStyle'),(None,'WebLicenses')]
for filename, symbol in assets:
    text = (root / filename).read_text(encoding='utf-8') if filename else '\n\n'.join(p.name+'\n'+p.read_text(encoding='utf-8') for p in licenses)
    if ')OWLANZI_ASSET"' in text:
        raise ValueError('Raw string delimiter collision')
    parts.append(f'inline const char {symbol}[] =\n')
    for start in range(0, len(text), 4000):
        parts.append(f'R"OWLANZI_ASSET({text[start:start+4000]})OWLANZI_ASSET"\n')
    parts.append(';\n')
parts.append('}\n')
output.write_text(''.join(parts), encoding='utf-8')
