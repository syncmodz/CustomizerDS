#!/usr/bin/env python3
"""
Conversor PNG para BCLIM (Binary Container Layout Image) para 3DS.
Gera arquivos .bclim para uso com Luma3DS LayeredFS.
"""

import sys
import os
import struct
from PIL import Image

def create_bclim(png_path, bclim_path, width, height):
    """Converte PNG para formato BCLIM."""
    try:
        img = Image.open(png_path).convert('RGBA')
        img = img.resize((width, height), Image.LANCZOS)
    except Exception as e:
        print(f"Erro ao abrir {png_path}: {e}")
        return False

    pixels = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = img.getpixel((x, y))
            # Converte RGBA para RGB565
            rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            pixels.append(struct.pack('<H', rgb565))

    pixel_data = b''.join(pixels)

    # Header BCLIM (40 bytes, little-endian)
    # magic(4) + bom(2) + headerSize(2) + version(4) + fileSize(4)
    # blockCount(4) + padding(8) + imag_magic(4) + blockSize(4)
    # width(2) + height(2) = 40 bytes total
    header = b''
    header += b'CLIM'                    # magic (4 bytes)
    header += struct.pack('<H', 0xFEFF)  # bom (2 bytes)
    header += struct.pack('<H', 0x0028)  # headerSize (2 bytes)
    header += struct.pack('<I', 0x02020000)  # version (4 bytes)
    fileSize = 40 + 4 + len(pixel_data)  # header(40) + imag_block(4) + pixels
    header += struct.pack('<I', fileSize)  # fileSize (4 bytes)
    header += struct.pack('<I', 0x00000001)  # blockCount (4 bytes)
    header += b'\x00' * 8                 # padding (8 bytes)
    header += b'imag'                     # imag_magic (4 bytes)
    header += struct.pack('<I', 0x00000010)  # blockSize (4 bytes)
    header += struct.pack('<H', width)    # width (2 bytes)
    header += struct.pack('<H', height)   # height (2 bytes)
    # Ensure header is exactly 40 bytes
    if len(header) != 40:
        print(f"Erro: header tem {len(header)} bytes, esperado 40")
        return False
    
    # Imag block: format(2) + mapMethod(2) = 4 bytes
    imag_block = struct.pack('<HH', 0x0002, 0x0000)  # RGB565, no map

    with open(bclim_path, 'wb') as f:
        f.write(header)
        f.write(imag_block)
        f.write(pixel_data)

    print(f"Gerado: {bclim_path} ({width}x{height}, {len(pixel_data)} bytes)")
    return True

def main():
    base_input = "/home/chicharito/Downloads/adicional/reva ui/Status Bar Icons (Reva UI).theme"
    output_dir = "/home/chicharito/CustomizerDS/revaui_output"
    
    os.makedirs(output_dir, exist_ok=True)

    # Mapeamento: (arquivo_png, nome_bclim, largura, altura)
    icons = [
        (f"{base_input}/UIImages/wifi@3x.png", "wifi_0.bclim", 24, 24),
        (f"{base_input}/Bundles/com.apple.CoreGlyphsPrivate/bluetooth@3x.png", "bt_0.bclim", 24, 24),
        (f"{base_input}/UIImages/airplane@3x.png", "airplane_0.bclim", 24, 24),
        (f"{base_input}/UIImages/alarm@3x.png", "alarm_0.bclim", 24, 24),
        (f"{base_input}/UIImages/moon.fill@3x.png", "mute_0.bclim", 24, 24),
        (f"{base_input}/UIImages/lock.rotation@3x.png", "orientation_0.bclim", 24, 24),
    ]

    print("Convertendo ícones RevaUI para BCLIM...")
    for png_path, bclim_name, w, h in icons:
        bclim_path = os.path.join(output_dir, bclim_name)
        if not create_bclim(png_path, bclim_path, w, h):
            print(f"Falha ao converter: {png_path}")

    print(f"\nConcluído! Arquivos em: {output_dir}")

if __name__ == "__main__":
    main()
