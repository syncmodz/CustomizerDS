#!/usr/bin/env python3
# mk_sysfont_merge.py -- gera uma fonte de sistema 3DS SEGURA fazendo MERGE:
# parte da fonte STOCK (cbf_std.bcfnt, ~7500 glifos incl. CJK + os icones de
# botao da faixa PUA E000+ que o Home Menu indexa) e substitui SO o desenho dos
# glifos Latinos pelo TTF/OTF custom, na mesma metrica de celula. Mantem cmap,
# larguras dos nao-tocados, glifo-fallback e estrutura -> nao crasha o NS, so
# "veste" o texto com a fonte nova.
#
# Por que: uma fonte feita so com mkbcfnt tem ~95 glifos (ASCII) -> o sistema
# acessa glifo inexistente no boot e CRASHA (brick recuperavel). Ver
# docs/SYSTEM_FONT.md.
#
# Uso: mk_sysfont_merge.py <stock.bcfnt> <custom.ttf> <out.bcfnt> [--size N] [--dump]
#   stock.bcfnt = STOCK ja DESCOMPRIMIDA (lzex -uvf). Saida = .bcfnt cru.
#
# Requer freetype-py.  Formato A4 tiled do 3DS validado por round-trip.

import sys, struct, argparse
import freetype

# ---- tiling A4 (8x8 tiles, ordem morton; 2 px/byte, par=nibble baixo) ----
def _morton(x, y):
    return (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)
def _pidx(x, y, W):
    return ((y // 8) * (W // 8) + (x // 8)) * 64 + _morton(x % 8, y % 8)
def decode_a4(buf, W, H):
    g = [[0] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            pi = _pidx(x, y, W); b = buf[pi // 2]
            g[y][x] = (b & 0xF) if (pi & 1) == 0 else (b >> 4)
    return g
def encode_a4(g, W, H):
    o = bytearray(W * H // 2)
    for y in range(H):
        for x in range(W):
            pi = _pidx(x, y, W); v = g[y][x] & 0xF
            if (pi & 1) == 0: o[pi // 2] = (o[pi // 2] & 0xF0) | v
            else:             o[pi // 2] = (o[pi // 2] & 0x0F) | (v << 4)
    return bytes(o)

# ---- parse minimo do BCFNT ----
class Bcfnt:
    def __init__(self, path):
        self.d = bytearray(open(path, 'rb').read())
        d = self.d
        fi = d.find(b'FINF'); self.fi = fi
        self.height, self.width, self.ascent = d[fi+28], d[fi+29], d[fi+30]
        self.tglpOff, self.cwdhOff, self.cmapOff = struct.unpack_from('<III', d, fi+16)
        tb = self.tglpOff - 8
        self.cellW, self.cellH, self.baseline, self.maxCharW = d[tb+8], d[tb+9], d[tb+10], d[tb+11]
        self.sheetSize = struct.unpack_from('<I', d, tb+12)[0]
        self.numSheets, self.fmt, self.cols, self.rows, self.sheetW, self.sheetH = \
            struct.unpack_from('<HHHHHH', d, tb+16)
        self.dataOff = struct.unpack_from('<I', d, tb+28)[0]
        assert self.fmt == 0x0B, "stock nao e A4 (0x0B)"
        self.cmap = self._read_cmap()
    def _read_cmap(self):
        d = self.d; m = {}; off = self.cmapOff
        while off:
            base = off - 8
            if d[base:base+4] != b'CMAP': break
            cb, ce, meth, _ = struct.unpack_from('<HHHH', d, base+8)
            nxt = struct.unpack_from('<I', d, base+16)[0]; p = base+20
            if meth == 0:
                co = struct.unpack_from('<H', d, p)[0]
                for c in range(cb, ce+1):
                    idx = co + (c - cb)
                    if idx != 0xFFFF: m[c] = idx
            elif meth == 1:
                for i in range(ce - cb + 1):
                    idx = struct.unpack_from('<H', d, p + i*2)[0]
                    if idx != 0xFFFF: m[cb+i] = idx
            elif meth == 2:
                cnt = struct.unpack_from('<H', d, p)[0]; p += 2
                for i in range(cnt):
                    c, idx = struct.unpack_from('<HH', d, p + i*4)
                    if idx != 0xFFFF: m[c] = idx
            off = nxt
        return m
    # célula (de-tiled) de um glifo -> grid 2D editavel + writeback
    def sheet_grid(self, sheet):
        o = self.dataOff + sheet * self.sheetSize
        return decode_a4(self.d[o:o+self.sheetSize], self.sheetW, self.sheetH)
    def write_sheet(self, sheet, grid):
        o = self.dataOff + sheet * self.sheetSize
        self.d[o:o+self.sheetSize] = encode_a4(grid, self.sheetW, self.sheetH)
    def set_width(self, idx, left, gw, cw):
        p = self.cwdhOff + 8 + idx * 3   # data = base+16 = cwdhOff+8
        self.d[p] = left & 0xFF
        self.d[p+1] = max(0, min(255, gw))
        self.d[p+2] = max(0, min(255, cw))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("stock"); ap.add_argument("ttf"); ap.add_argument("out")
    ap.add_argument("--size", type=int, default=0, help="px (0 = auto-fit pela metrica)")
    ap.add_argument("--dump", action="store_true", help="ascii-art de alguns glifos p/ conferir")
    a = ap.parse_args()

    b = Bcfnt(a.stock)
    face = freetype.Face(a.ttf)
    cellW, cellH, baseline = b.cellW, b.cellH, b.baseline
    colStride = cellW + 1

    # auto-fit: escolhe px pra ascender caber ~ baseline e altura total <= cellH
    if a.size > 0:
        px = a.size
    else:
        face.set_pixel_sizes(0, 48)
        asc = face.size.ascender / 64.0; desc = -face.size.descender / 64.0
        scale = min((baseline - 1) / max(asc, 1), (cellH - 1) / max(asc + desc, 1))
        px = max(8, int(round(48 * scale)))
    face.set_pixel_sizes(0, px)

    targets = [c for c in range(0x20, 0x250) if c in b.cmap]
    edited_sheets = {}
    n = 0
    for c in targets:
        try:
            face.load_char(chr(c), freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_NORMAL)
        except Exception:
            continue
        g = face.glyph; bm = g.bitmap
        idx = b.cmap[c]; sheet = idx // (b.cols * b.rows); col = idx % b.cols
        cx0 = col * colStride
        if sheet not in edited_sheets:
            edited_sheets[sheet] = b.sheet_grid(sheet)
        grid = edited_sheets[sheet]
        # limpa a celula
        for yy in range(cellH):
            for xx in range(cellW):
                grid[yy][cx0 + xx] = 0
        gx = max(0, g.bitmap_left)
        gy = baseline - g.bitmap_top
        for ry in range(bm.rows):
            ty = gy + ry
            if ty < 0 or ty >= cellH: continue
            for rx in range(bm.width):
                tx = gx + rx
                if tx < 0 or tx >= cellW: continue
                v = bm.buffer[ry * bm.pitch + rx]
                grid[ty][cx0 + tx] = (v * 15 + 127) // 255   # 8bit->4bit
        adv = (g.advance.x >> 6)
        b.set_width(idx, max(-8, min(127, g.bitmap_left)), min(cellW, bm.width), max(1, adv))
        n += 1
        if a.dump and c in (ord('A'), ord('a'), ord('g'), ord('@')):
            print(f"--- '{chr(c)}' (px={px}, adv={adv}) ---")
            for yy in range(cellH):
                print("".join(" .:-=+*#%@"[min(9, grid[yy][cx0+xx]*9//15)] for xx in range(cellW)))

    for sheet, grid in edited_sheets.items():
        b.write_sheet(sheet, grid)
    open(a.out, 'wb').write(bytes(b.d))
    print(f"OK {a.out}: {n} glifos Latinos remapeados (px={px}); {b.numSheets} folhas, estrutura/CJK/PUA intactos.")

if __name__ == "__main__":
    main()
