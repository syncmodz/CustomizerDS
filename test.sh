#!/bin/bash
# CustomizerDS - build + run helper
# Uso: ./test.sh {build|run|check|clean|fonts|wipe-azahar}

SELF="$0"
SRC="/home/chicharito/CustomizerDS"
FILE="$SRC/build/CustomizerDS.3dsx"
SDMC_DIR="/home/chicharito/.var/app/org.azahar_emu.Azahar/data/azahar-emu/sdmc"
SDMC="$SDMC_DIR/CustomizerDS.3dsx"
AZAHAR_DIR="/home/chicharito/.var/app/org.azahar_emu.Azahar"

FONTS_SRC="/home/chicharito/Downloads/adicional"
MKBCFNT="/opt/devkitpro/tools/bin/mkbcfnt"

case "${1:-run}" in
  build)
    cd "$SRC"
    make clean 2>&1 >/dev/null
    make 2>&1
    echo "--- BUILD OK ---"
    chown chicharito:chicharito "$FILE" 2>/dev/null || true
    sha256sum "$FILE"
    ;;
  fonts)
    "$MKBCFNT" -o "$SRC/romfs/fonts/comfortaa_regular.bcfnt" -s 10 "$FONTS_SRC/Comfortaa-Regular.ttf"
    "$MKBCFNT" -o "$SRC/romfs/fonts/comfortaa_bold.bcfnt"    -s 10 "$FONTS_SRC/Comfortaa-Bold.ttf"
    "$MKBCFNT" -o "$SRC/romfs/fonts/made_evolve_regular.bcfnt" -s 10 "$FONTS_SRC/MADE Evolve Sans Regular EVO (PERSONAL USE).otf"
    "$MKBCFNT" -o "$SRC/romfs/fonts/made_evolve_bold.bcfnt"    -s 10 "$FONTS_SRC/MADE Evolve Sans Bold EVO (PERSONAL USE).otf"
    chown chicharito:chicharito "$SRC/romfs/fonts/"*.bcfnt
    echo "--- FONTS OK ---"
    ;;
  run)
    mkdir -p "$SDMC_DIR"
    cp "$FILE" "$SDMC"
    flatpak run --filesystem=host org.azahar_emu.Azahar "$FILE"
    ;;
  check)
    echo "=== SHA256 ==="
    sha256sum "$FILE"
    echo "=== Timestamp ==="
    ls -la --full-time "$FILE"
    echo "=== Tamanho ==="
    ls -lh "$FILE"
    echo "=== Strings novas ==="
    for s in "Touchbar" "Velocidade" "simulacao" "customizada" "Preview" "RGB"; do
      n=$(strings "$FILE" 2>/dev/null | grep -c "$s" || echo "0")
      echo "  $s: $n ocorrencias"
    done
    ;;
  clean)
    rm -rf "$SRC/build"
    rm -f "$SDMC"
    rm -f "$SDMC_DIR/3ds/CustomizerDS/config.bin"
    echo "=== Limpo ==="
    ;;
  wipe-azahar)
    rm -rf "$AZAHAR_DIR/cache" "$AZAHAR_DIR/config" "$AZAHAR_DIR/data" "$AZAHAR_DIR/.ld.so" "$AZAHAR_DIR/.local"
    mkdir -p "$AZAHAR_DIR/config/azahar-emu"
    cat > "$AZAHAR_DIR/config/azahar-emu/qt-config.ini" << INI
[Renderer]
resolution_factor=4

[Data Storage]
sdmc_directory=$AZAHAR_DIR/data/azahar-emu/sdmc/
nand_directory=$AZAHAR_DIR/data/azahar-emu/nand/
use_virtual_sd=true

[System]
is_new_3ds=true
INI
    echo "=== Azahar zerado (4x resolucao nativa) ==="
    ;;
  *)
    echo "Uso: $SELF {build|fonts|run|check|clean|wipe-azahar}"
    exit 1
    ;;
esac
