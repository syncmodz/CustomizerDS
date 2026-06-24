# CustomizerDS

![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Platform: Nintendo 3DS](https://img.shields.io/badge/Platform-Nintendo%203DS-red)
![Language: C](https://img.shields.io/badge/Language-C-00599C)
![Version: 1.0.0](https://img.shields.io/badge/Version-1.0.0-success)

Um homebrew elegante e fluido para Nintendo 3DS para personalizar seu console —
interface estilo Glass (inspirada em Reva UI / cakeOS), 60fps reais e animações
suaves tipo Apple. **Totalmente navegável só com o D-pad.**

## ✨ Features

- 🔤 **8 fontes** embarcadas (Comfortaa, MADE Evolve, Love House, Comic Sans MS3, Minecraftia, Patterns & Dots) com preview na própria fonte.
- 🌍 **3 idiomas — Português, Inglês, Espanhol** — seletor na aba Tema, padrão pelo idioma do console e persistência.
- 🌗 **Temas** Claro/Escuro com animação **sol/lua** girando + wipe radial de ambiente.
- 🎨 **Accent** com presets + **picker HSV/HEX** interativo e preview em tempo real.
- 💡 **LED RGB** (modos RGB, Fixo, Pulso, Off) com sliders animados.
- 🎬 **Compositor de transições** (render-to-texture): troca de aba com fade-out real e movimento em bloco, 10 curvas no pool.
- 🕹️ **D-pad + Touch**: toda a interface funciona apenas com o D-pad (stylus/mouse opcionais).

## 🎮 Controles

| Botão | Ação |
|---|---|
| **D-Pad** | Navegar / ajustar valores |
| **A** | Selecionar / aplicar |
| **B** | Voltar |
| **L+R+SELECT** | Laboratório de transições (build de dev) |
| **START** | Sair |

## 🔧 Build

Requer [devkitPro](https://devkitpro.org/) (libctru + citro2d/citro3d), `tex3ds`,
`mkbcfnt` e `bannertool`/`makerom`.

```bash
cd CustomizerDS
make clean && make
# Gera build/CustomizerDS.3dsx (HBL/emulador) e build/CustomizerDS.cia (instalável)
```

Testado no **Azahar** (emulador). `build/` é descartável — o `make` regenera tudo.

## 📦 Instalação

- **Emulador:** abra `CustomizerDS.3dsx` no Azahar/Lime3DS, ou instale o `.cia`.
- **Console (CFW):** instale `CustomizerDS.cia` pelo FBI, ou rode o `.3dsx` pelo Homebrew Launcher.

## ⚠️ Fonte do sistema & segurança

A troca de fonte é **100% in-app e segura** — o app **nunca escreve na NAND**.
Trocar a fonte do **sistema inteiro** é outra coisa (não implementado), com risco
real de brick. Leia a doutrina completa em **[docs/ANTIBRICK.md](docs/ANTIBRICK.md)**.

## 🙏 Créditos

- Ícones inspirados no **Reva UI / cakeOS**.
- Fontes de seus respectivos autores.
- Construído com **devkitPro / libctru / citro2d**.

## 📄 Licença

GNU General Public License v3.0 — veja [LICENSE](LICENSE).

Desenvolvido com ❤️ para o 3DS.
