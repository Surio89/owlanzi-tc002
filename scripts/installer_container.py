# SPDX-License-Identifier: GPL-3.0-or-later
"""Portable Z21 RES-only container, interoperable with the manufacturer fsimg.

The v1 format stores the first 16 payload bytes in the descriptor, its MD5 at
the payload offset, and the remaining bytes thereafter. CRC32 covers the header.
MD5 is required by this legacy format; installation additionally verifies SHA256
and complete device readback. The padding is deterministic, non-secret filler.
"""
import hashlib
import struct
import zlib

MAGIC=b'ZKSWEV1.0-180127'
HEADER_SIZE=572

def filler():
    result=bytearray();seed=1
    for _ in range(128):
        seed=(seed*214013+2531011)&0xffffffff
        result.extend(struct.pack('<I',(seed>>16)&0x7fff))
    return bytes(result[:509])

def wrap_res(payload,padding=None):
    if not 16<=len(payload)<=0x800000:raise ValueError('Invalid RES payload size')
    padding=filler() if padding is None else padding
    if len(padding)!=509:raise ValueError('Invalid header padding')
    header=bytearray(MAGIC+b'\x30\x01\x30\x23\x03\x10\x60\x6c')
    header.extend(struct.pack('<II',HEADER_SIZE,len(payload)))
    header.extend(payload[:16])
    header.extend(struct.pack('<I',524)+b'\x02\x06\x06\x55\xaa\x00\x00'+padding)
    assert len(header)==568
    header.extend(struct.pack('<I',zlib.crc32(header)))
    return bytes(header)+hashlib.md5(payload,usedforsecurity=False).digest()+payload[16:]

def unwrap_res(image):
    if len(image)<HEADER_SIZE+16 or image[:24]!=MAGIC+b'\x30\x01\x30\x23\x03\x10\x60\x6c':raise ValueError('Invalid Z21 RES header')
    offset,size=struct.unpack_from('<II',image,24)
    if offset!=HEADER_SIZE or not 16<=size<=0x800000 or len(image)!=offset+size:raise ValueError('Invalid RES bounds')
    if image[48:59]!=struct.pack('<I',524)+b'\x02\x06\x06\x55\xaa\x00\x00':raise ValueError('Unexpected partition or platform')
    if zlib.crc32(image[:568])!=struct.unpack_from('<I',image,568)[0]:raise ValueError('Invalid header checksum')
    data=image[32:48]+image[offset+16:]
    if hashlib.md5(data,usedforsecurity=False).digest()!=image[offset:offset+16]:raise ValueError('Invalid payload checksum')
    return data
