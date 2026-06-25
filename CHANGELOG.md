# Changelog

## [1.1.0] — 2026-06-24

### 🐛 Fixed (crítico)
- **Ícones e fontes sumiam no `.cia`** (romfs não era lido) — o wrapper IVFC artesanal gerava uma RomFS que o 3DS não conseguia ler. Agora o **makerom constrói a RomFS** a partir da pasta (`RomFs: RootPath`), método padrão/correto. O `.3dsx` sempre funcionou; o bug era só no `.cia`.
- **LED sumia ao fechar o app** — o app não apaga mais o LED ao sair; o padrão continua até reboot/notificação.

### ✨ Added / Changed
- **9ª fonte: Super Mario 64**; **"Padrão do Sistema"** de volta como 1ª opção (10 itens).
- LED **reaplica a cor salva ao abrir o app**.
- Home: pílulas (Fontes/Tema/LED) descidas pro centro da tela de baixo; ícones de cima afastados do ícone do app.
- Tema: rótulo **"HEX"** legível no swatch; fundo da tela de baixo **sem bicolor**; lista de fontes **rolável** com indicador "em uso".
- Fluidez: 1º frame da transição otimizado (sem o soluço de abertura).
- Docs: `SYSTEM_FONT.md` e `LED_PERSIST.md` (métodos provados + settings da Luma); `ANTIBRICK.md` encolhido.
- README resumido + **QR code** pra instalar o `.cia` pelo FBI.

## [1.0.0] — 2026-06-24

### ✨ Added
- **3 idiomas: Português, Inglês, Espanhol** — seletor na aba Tema, padrão pelo idioma do sistema (CFGU), persistido no save.
- **8 fontes** (4 novas: Love House, Comic Sans MS3, Minecraftia, Patterns & Dots), cada uma com preview na própria fonte + selo "em uso".
- **Compositor de texturas offscreen**: transições de aba com **fade-out real** (tela velha sai) e movimento em bloco; 10 transições no pool + Laboratório de transições (build de dev).
- **Home** de cima vira slide de 1 função por vez; ícone real do app no lugar do placeholder "CDS".
- Sprites cakeOS (`extra.t3x`): swatches e knob do slider via aro tintado, sem empilhamento de camadas.
- `docs/ANTIBRICK.md` — doutrina de segurança de fonte de sistema.

### 🐛 Fixed
- Lua ficava preta no escuro → agora **prata** (moon_fill original tintado); sol amarelo.
- "Aa"/"raio" da home agora em **accent** (claro e escuro).
- HEX no modo claro sem sombreado feio; rótulo "HEX" do swatch visível.
- Lista de 8 fontes não estoura mais a tela; fundo da aba Fontes uniforme (sem bicolor).
- **D-pad**: sliders de cor do LED agora ajustáveis por D-pad (eram só toque) — app 100% navegável por D-pad.
- LED restaurado ao sair; `fontIndex`/`lang` com clamp; save round-trip com idioma.
- Home: vitrine de cima e card de baixo centralizados.

### 🔧 Technical
- `build/` deixa de ser versionado (regenerado pelo `make`).
- Módulos novos: `compositor`, `transitions`, `lang`. Sheet `extra.t3x` gerado via `tex3ds --atlas`.

## [0.1-alpha] — 2026-06-23

### ✨ Added
- Interface completa com 5 abas (Home, Fontes, Tema, LED)
- 10+ transições de abertura de aba variadas (Slide Parallax, Pop-up, Wipe Radial, etc.)
- Animação yin-yang (claro↔escuro) com wipe radial de ambiente
- Glass UI estilo Reva/cakeOS (ícones com contorno bold, translúcidos)
- Seletor HSV de cores com preview em tempo real
- Sliders RGB + Velocidade com spring-bounce suave
- Touch & D-pad (stylus, mouse no emulador, controles de console)
- Fontes embarcadas (Comfortaa, MADE Evolve)
- Easing Apple-grade (easeOut/InOut/Back + spring), 60fps reais

### 🐛 Fixed
- Formas arredondadas desenhadas como peça única (eliminou costuras de camadas)
- Acúmulo de alpha em cards e sliders
- Centralização de conteúdo nas telas
- Input D-pad horizontal na home (não vertical)

### 🔧 Technical
- Engine de tween por dt
- Registro de transições plugável
- Design tokens centralizados (theme.h)
- Build limpo, zero warnings

## Planned
- v0.2: Mais temas, persistência de settings
- v0.3: Temas customizados, mais fontes
