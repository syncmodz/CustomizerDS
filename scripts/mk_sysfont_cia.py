#!/usr/bin/env python3
# mk_sysfont_cia.py -- gera UM .cia que substitui o titulo da FONTE DE SISTEMA
# do 3DS (0004009B00014002, USA/EUR/JPN) a partir de um .bcfnt em A4.
#
# Pipeline = FontTool (astronautlevel2 & ihaveamac, MIT -- ver
# scripts/sysfont/FontTool-LICENSE.txt) portado pra py3 e usando makerom no
# lugar do make_cia:
#   1) 3dstool -zvf font.bcfnt --compress-type lzex --compress-out cbf_std.bcfnt.lz
#   2) 3dstool -cvtf romfs romfs.bin --romfs-dir romfs/   (romfs/ = so o .lz)
#   3) 3dstool -cvtf cfa font.cfa --header ncchheader.bin --romfs romfs.bin
#   4) patch do tamanho da romfs no header NCCH (@0x1B4, media units 0x200)
#   5) makerom -f cia -o out.cia -content font.cfa:0:0
#
# O ncchheader.bin (vendorizado em scripts/sysfont/) ja traz o TID certo e a
# flag NoCrypto (0x04) -> o .cia sai "decrypted" e instala/roda sob Luma SEM
# re-encriptar (o aviso do makerom "Failed to load ncch aes key" e NORMAL).
#
# Valida ANTES de empacotar: A4 (sheetFormat 0x0B no TGLP do bcfnt) e
# comprimido <= 1.5 MiB -- fonte fora disso pode travar o Home Menu.
#
# Uso: mk_sysfont_cia.py <font.bcfnt> <out.cia> --header <ncchheader.bin>
#                        [--3dstool PATH] [--makerom PATH]

import os, sys, argparse, subprocess, tempfile, shutil

MAX_LZ = 1536 * 1024  # 1.5 MiB

def run(cmd):
    r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if r.returncode != 0:
        sys.stderr.write("FALHOU: " + " ".join(cmd) + "\n")
        sys.stderr.write(r.stdout.decode("utf-8", "replace") + "\n")
        sys.exit(1)

def check_a4(bcfnt_path):
    """Confere o byte de formato do sheet no bloco TGLP: 0x0B = A4 (bom).
    08 (ou outros) = formato que corrompe/trava a fonte de sistema."""
    data = open(bcfnt_path, "rb").read()
    if data[:4] not in (b"CFNT", b"CFNU"):
        sys.exit("  ! nao parece um .bcfnt (magic CFNT ausente): " + bcfnt_path)
    i = data.find(b"TGLP")
    if i < 0:
        sys.exit("  ! bloco TGLP nao encontrado em " + bcfnt_path)
    # TGLP: sig(4) size(4) cellW(1) cellH(1) baseline(1) maxCharW(1)
    #       sheetSize(4) numSheets(2) sheetFormat(2 LE)
    fmt = int.from_bytes(data[i + 18:i + 20], "little")
    if fmt != 0x0B:
        sys.exit("  ! sheetFormat=0x%02X (esperado 0x0B / A4) em %s -- abortado "
                 "(fonte fora de A4 pode travar o Home Menu)" % (fmt, bcfnt_path))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bcfnt", help=".bcfnt (A4); ou o cbf_std.bcfnt.lz se --lz")
    ap.add_argument("out_cia")
    ap.add_argument("--header", required=True)
    ap.add_argument("--3dstool", dest="t3", default="3dstool")
    ap.add_argument("--makerom", default="makerom")
    ap.add_argument("--lz", action="store_true",
                    help="entrada ja e um cbf_std.bcfnt.lz (lzex) -- nao recomprime "
                         "(usado p/ a fonte STOCK extraida da NAND)")
    a = ap.parse_args()

    for p in (a.bcfnt, a.header):
        if not os.path.exists(p):
            sys.exit("arquivo nao existe: " + p)

    work = tempfile.mkdtemp(prefix="sysfontcia_")
    try:
        romfs = os.path.join(work, "romfs"); os.makedirs(romfs)
        lz = os.path.join(romfs, "cbf_std.bcfnt.lz")  # nome interno OBRIGATORIO
        if a.lz:
            # entrada ja comprimida: descomprime so p/ validar A4 (0B), e usa o
            # .lz original tal e qual (fiel a fonte do console).
            chk = os.path.join(work, "check.bcfnt")
            run([a.t3, "-uvf", a.bcfnt, "--compress-type", "lzex", "--compress-out", chk])
            check_a4(chk)
            shutil.copyfile(a.bcfnt, lz)
        else:
            check_a4(a.bcfnt)
            run([a.t3, "-zvf", a.bcfnt, "--compress-type", "lzex", "--compress-out", lz])

        lzsize = os.path.getsize(lz)
        if lzsize > MAX_LZ:
            sys.exit("  ! comprimido %d bytes > 1.5 MiB em %s -- abortado"
                     % (lzsize, a.bcfnt))

        romfsbin = os.path.join(work, "romfs.bin")
        run([a.t3, "-cvtf", "romfs", romfsbin, "--romfs-dir", romfs])

        cfa = os.path.join(work, "font.cfa")
        run([a.t3, "-cvtf", "cfa", cfa, "--header", a.header, "--romfs", romfsbin])

        # makerom usa um header NCCH estatico; corrige o tamanho da romfs nele.
        sz = os.path.getsize(romfsbin) // 0x200
        with open(cfa, "r+b") as f:
            f.seek(0x1B4); f.write(sz.to_bytes(4, "little"))

        outdir = os.path.dirname(os.path.abspath(a.out_cia))
        if outdir:
            os.makedirs(outdir, exist_ok=True)
        run([a.makerom, "-f", "cia", "-o", a.out_cia, "-content", cfa + ":0:0"])
        print("  OK %s  (lz=%d bytes)" % (a.out_cia, lzsize))
    finally:
        shutil.rmtree(work, ignore_errors=True)

if __name__ == "__main__":
    main()
