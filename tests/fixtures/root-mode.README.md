# Synthetic SquashFS root-mode fixture

`root-mode.squashfs` contains only generated test text, a minimal startup config,
and directories `bin`, `etc` and `lib`. It contains no manufacturer resources,
executable app, private configuration or credentials. The root mode is 0711;
directories use 0755 and text files 0644 except `bin/fixture` (0755).
All owners are uid/gid 1000. Inode metadata is uncompressed (`-noI`).

The fixture exercises exact two-byte root permission restoration and the complete
image build inside each frozen Windows, macOS and Linux package. Its unusual root
mode intentionally differs from host staging folder permissions. Never install it
on hardware. Generated for Owlanzi; GPL-3.0-or-later like the tests.
