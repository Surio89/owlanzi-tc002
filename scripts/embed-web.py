#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Embed the actual offline UI in both the desktop and TC002 application."""
import pathlib
import sys
root = pathlib.Path(sys.argv[1])
output = pathlib.Path(sys.argv[2])
output.parent.mkdir(parents=True, exist_ok=True)
parts = ['// Generated from web/. Do not edit.\n#pragma once\nnamespace owlanzi {\n']
for filename, symbol in [('index.html', 'WebIndex'), ('app.js', 'WebScript'), ('style.css', 'WebStyle')]:
    text = (root / filename).read_text(encoding='utf-8')
    if ')OWLANZI_ASSET"' in text:
        raise ValueError('Raw string delimiter collision')
    parts.append(f'inline const char {symbol}[] =\n')
    for start in range(0, len(text), 4000):
        parts.append(f'R"OWLANZI_ASSET({text[start:start+4000]})OWLANZI_ASSET"\n')
    parts.append(';\n')
parts.append('}\n')
output.write_text(''.join(parts), encoding='utf-8')
