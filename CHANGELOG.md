# Changelog

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
