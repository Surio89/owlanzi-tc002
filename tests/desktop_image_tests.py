# SPDX-License-Identifier: GPL-3.0-or-later
"""Root permissions must be independent of the host staging folder's ACL."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
from installer_image import layout,restore_root_mode

class RootModeTests(unittest.TestCase):
    fixture=ROOT/'tests/fixtures/root-mode.squashfs'
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.image=Path(self.temp.name)/'image.squashfs';self.original=self.fixture.read_bytes();self.image.write_bytes(self.original)
    def test_host_750_and_other_modes_restore_exact_original_permissions(self):
        original_layout=layout(self.image)
        for host_mode in (0o750,0o770,0o755,0o700,0o777):
            restore_root_mode(self.image,host_mode)
            self.assertEqual(layout(self.image)['']['mode'],host_mode)
            restore_root_mode(self.image,0o711)
            self.assertEqual(layout(self.image),original_layout)
            self.assertEqual(self.image.read_bytes(),self.original)
    def test_only_two_byte_root_mode_field_can_change(self):
        restore_root_mode(self.image,0o770);changed=self.image.read_bytes()
        reference=struct.unpack_from('<Q',self.original,32)[0];start=struct.unpack_from('<Q',self.original,64)[0]
        field=start+(reference>>16)+2+(reference&0xffff)+2
        self.assertEqual(changed[:field],self.original[:field]);self.assertEqual(changed[field+2:],self.original[field+2:])
        self.assertEqual(len(changed),len(self.original))
    def test_bad_format_compressed_truncated_and_non_directory_are_unchanged(self):
        reference=struct.unpack_from('<Q',self.original,32)[0];start=struct.unpack_from('<Q',self.original,64)[0]
        block=start+(reference>>16);location=block+2+(reference&0xffff)
        cases=[(0,b'bad!'),(24,bytes(2)),(28,struct.pack('<H',5)),(32,struct.pack('<Q',0xffffffffffffffff)),
               (40,struct.pack('<Q',99999999)),(block,bytes(2)),(location,struct.pack('<H',2))]
        for offset,value in cases:
            with self.subTest(offset=offset):
                malformed=bytearray(self.original);malformed[offset:offset+len(value)]=value;self.image.write_bytes(malformed)
                with self.assertRaises(ValueError):restore_root_mode(self.image,0o770)
                self.assertEqual(self.image.read_bytes(),malformed)
        for length in (0,95,block+1):
            self.image.write_bytes(self.original[:length])
            with self.assertRaises(ValueError):restore_root_mode(self.image,0o770)
    def test_invalid_mode_does_not_change_image(self):
        for mode in (-1,0o10000,'0770'):
            with self.assertRaises(ValueError):restore_root_mode(self.image,mode)
            self.assertEqual(self.image.read_bytes(),self.original)

if __name__=='__main__':unittest.main()
