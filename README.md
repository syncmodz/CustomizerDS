# CustomizerDS

![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Platform: Nintendo 3DS](https://img.shields.io/badge/Platform-Nintendo%203DS-red)
![Version: 1.1.0](https://img.shields.io/badge/Version-1.1.0-success)

Personalize seu Nintendo 3DS — interface fluida estilo Glass (Reva UI / cakeOS),
**totalmente navegável só com o D-pad**.

## 📲 Instalar (rápido)

Escaneie o QR no **FBI → Remote Install → Scan QR Code** (instala o `.cia` direto):

<img src="docs/img/qr_install_cia.png" width="180" alt="QR para instalar o .cia"/>

Ou baixe manualmente em **[Releases](https://github.com/syncmodz/CustomizerDS/releases/latest)**:
- `CustomizerDS.cia` — instalar com FBI / GodMode9 (*Install game image*)
- `CustomizerDS.3dsx` — Homebrew Launcher / emulador (Azahar, Lime3DS)

## ✨ Features

- 🔤 **9 fontes** (inclui Super Mario 64) + opção **Padrão do Sistema** — preview na própria fonte.
- 🌍 **3 idiomas** (PT / EN / ES) — padrão pelo idioma do console, persistido.
- 🌗 Temas Claro/Escuro com animação sol/lua + **accent** (presets + picker HSV/HEX).
- 💡 **LED RGB** — modos RGB/Fixo/Pulso/Off; a cor escolhida **reaplica ao abrir o app** e continua depois de fechar.
- 🎬 Transições com compositor (fade-out real, movimento em bloco).
- 🕹️ **D-pad total** (toque opcional).

## 🎮 Controles

| Botão | Ação |
|---|---|
| **D-Pad** | Navegar / ajustar |
| **A** | Selecionar / aplicar |
| **B** | Voltar |
| **START** | Sair |

## 💡 LED persistente & 🔤 Fonte de sistema (avançado)

- **LED através de reboot:** precisa de um patch da Luma — ligue **"Enable game patching"** + **"Enable loading external FIRMs and modules"** na config da Luma. Passo a passo em **[docs/LED_PERSIST.md](docs/LED_PERSIST.md)**.
- **Trocar a fonte do SISTEMA:** método manual provado (A4, NAND backup) em **[docs/SYSTEM_FONT.md](docs/SYSTEM_FONT.md)** — o app **não** gera a fonte de sistema (evita texto corrompido); a fonte **in-app** é 100% segura.

## 🔧 Build

devkitPro (libctru + citro2d), `tex3ds`, `mkbcfnt`, `bannertool`/`makerom`:

```bash
make clean && make   # gera build/CustomizerDS.3dsx e .cia
```

## 📄 Licença

GNU General Public License v3.0 — veja [LICENSE](LICENSE). Feito com ❤️ para o 3DS.
