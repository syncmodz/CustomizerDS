#!/usr/bin/env python3
# Wraps a Level 3 RomFS (from mkromfs3ds) in an IVFC container
# compatible with makerom -romfs.
import sys, struct, hashlib

def align_up(v, b):
    return ((v + b - 1) // b) * b

def make_ivfc(l3_raw):
    L3, L2, L1 = 0x1000, 0x800, 0x800   # block sizes
    L3LOG, L2LOG, L1LOG = 12, 11, 11
    HASH = 32  # SHA-256

    # Pad Level 3 data to block boundary
    l3_size = len(l3_raw)
    l3_ps = align_up(l3_size, L3)
    l3 = l3_raw + bytes(l3_ps - l3_size)

    # Level 2: one SHA-256 hash per L3 block
    l2 = b''.join(hashlib.sha256(l3[i:i+L3]).digest() for i in range(0, l3_ps, L3))
    l2_size = len(l2)
    l2_ps = align_up(l2_size, L2)
    l2 = l2 + bytes(l2_ps - l2_size)

    # Level 1: one SHA-256 hash per L2 block
    l1 = b''.join(hashlib.sha256(l2[i:i+L2]).digest() for i in range(0, l2_ps, L2))
    l1_size = len(l1)
    l1_ps = align_up(l1_size, L1)
    l1 = l1 + bytes(l1_ps - l1_size)

    # Master hash: SHA-256 of all Level 1 data
    master = hashlib.sha256(l1).digest()

    # Logical offsets (relative to start of L1 data, i.e. after header + master hash)
    l1_off = 0
    l2_off = l1_ps
    l3_off = l1_ps + l2_ps

    hdr = bytearray(0x60)
    hdr[0:4] = b'IVFC'
    struct.pack_into('<I', hdr, 0x04, 0x10000)   # IVFC magic for RomFS
    struct.pack_into('<I', hdr, 0x08, HASH)       # master hash size
    struct.pack_into('<Q', hdr, 0x0C, l1_off)
    struct.pack_into('<Q', hdr, 0x14, l1_size)
    struct.pack_into('<I', hdr, 0x1C, L1LOG)
    struct.pack_into('<Q', hdr, 0x24, l2_off)
    struct.pack_into('<Q', hdr, 0x2C, l2_size)
    struct.pack_into('<I', hdr, 0x34, L2LOG)
    struct.pack_into('<Q', hdr, 0x3C, l3_off)
    struct.pack_into('<Q', hdr, 0x44, l3_size)
    struct.pack_into('<I', hdr, 0x4C, L3LOG)
    struct.pack_into('<I', hdr, 0x54, 0x78)       # optional info size

    return bytes(hdr) + master + l1 + l2 + l3

if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.stderr.write('Usage: mk_ivfc_romfs.py <input_l3.romfs> <output_ivfc.romfs>\n')
        sys.exit(1)
    with open(sys.argv[1], 'rb') as f:
        data = f.read()
    with open(sys.argv[2], 'wb') as f:
        f.write(make_ivfc(data))
