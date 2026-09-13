# SPDX-License-Identifier: GPL-3.0-or-later
import subprocess
import sys
import tempfile
with tempfile.TemporaryDirectory(prefix='owlanzi-knob-') as directory:
    subprocess.run([sys.argv[1],directory],check=True,timeout=15)
print('Rotary input, preview takeover, brightness limits and persistence passed.')
