# CustomizerDS - macOS Touch Bar Edition

## Resumo das Correcoes e Melhorias

### Bugs Corrigidos

1. **Stutter em setas (causa raiz)**
   - `input.c`: left/right agora usam debounce/repeat igual up/down
   - `led.c`: saves em disco nao sao mais chamados a cada frame de seta — usam `SAVE_DEBOUNCE_FRAMES` (18 frames ~300ms)

2. **Tema funcionando nas duas telas**
   - `ui.c`: `UI_TouchBarBackground()` usa `g_theme.background`
   - `UI_KeyHelp`: barra de fundo adapta ao tema (claro/escuro)
   - Todas as cores hardcoded foram substituidas por `themeIsDark()` ternarios

3. **Preview de fontes**
   - `fonts.c`: `FONT_REAL_LOAD` = 1 (reativado)
   - Preview agora mostra mensagem elegante se fonte nao carregar, ao inves de crashar
   - Fontes em `romfs/fonts/` carregadas via `C2D_FontLoad`

4. **Arredondamento**
   - `UI_RoundRect`: usa `sqrtf(ri*ri - dy*dy)` para circulos perfeitos

5. **UI_MorphSelector**
   - Variavel static `s_morphX` agora tem `s_morphInit` booleano para inicializacao correta
   - Mesmo fix em `darkmode.c` e `led.c`

6. **UI_Shadow**
   - Agora aceita parametro `alpha` para controle de intensidade

### Redesign macOS Touch Bar

#### Top Screen (400x240)
- `UI_MacPanel`: glass panel com sombra suave e borda sutil
- Fundo com gradiente sutil (highlight no topo, sombra embaixo)
- Tipografia hierarquica: titulo (.58f), subtitulo (.34f), caption (.25f)

#### Bottom Screen / Touch Bar (320x240)
- `UI_TouchBarBackground`: faixa escura no topo (44px) simulando Touch Bar do MacBook Pro
- Abaixo: MacPanel clean estilo macOS para conteudo
- Key help bar adaptativa ao tema na base (26px)
- Seletor modo LED e segmentos escuro/claro com morph animation

#### Componentes Novos
- `UI_TouchBarButton`: botao estilo Touch Bar com icone em badge quadrado + label
- `UI_MacPanel`: glass panel padrao macOS (sombra + borda + highlight)

#### Animacoes
- Entrada de tela com `easeOutBack` (spring com overshoot)
- Itens aparecem escalonados com delay
- Startup logo com overshoot no badge CDS e saida suave

### Reva UI Icons

Os icones do Reva UI (extraidos de `/home/chicharito/Downloads/reva ui.tar`) servem como
referencia de estilo. O app usa icon badges em quadrados arredondados com fundo na
cor de destaque, que replicam o estilo Reva UI de icones minimalistas em formato
rounded square com simbolo central.

Para integracao futura de icones Reva UI como texturas:
```bash
# Extrair icones
tar xf /home/chicharito/Downloads/reva\ ui.tar -C /tmp/reva
# Criar sprite sheet (requer tex3ds)
# Mais detalhes: https://github.com/devkitPro/tex3ds
```

### Arquivos Modificados

| Arquivo | Linhas | O que mudou |
|---------|--------|-------------|
| `source/ui.c` | 434 | Rewrite completo: macOS Touch Bar, shadows, morph fix |
| `source/ui.h` | 63 | Add UI_MacPanel, UI_TouchBarButton, UI_Shadow alpha |
| `source/menu.c` | 140 | Pills com cores do tema, TouchBarButton |
| `source/menu.h` | 16 | Add menuRenderTouchBar |
| `source/fonts.c` | 244 | Preview seguro com NULL font, cores do tema |
| `source/darkmode.c` | 269 | Morph init fix, cores adaptativas |
| `source/led.c` | 452 | Save debounce dpad, morph init fix |
| `source/main.c` | 189 | Cleanup, sem mudancas estruturais |

### Build

```bash
cd /home/chicharito/CustomizerDS
make clean && make
# Resultados em build/:
#   CustomizerDS.3dsx  (Homebrew Launcher, 2.3MB)
#   CustomizerDS.cia   (Instalacao via FBI, 2.3MB)
#   CustomizerDS.elf   (Debug, 1.5MB)
```

### Teste no Azahar

```bash
azahar /home/chicharito/CustomizerDS/build/CustomizerDS.3dsx
```

### Uso no Azahar

```bash
flatpak run org.azahar_emu.Azahar /home/chicharito/CustomizerDS/build/CustomizerDS.3dsx
```

### Checklist de Testes

- [ ] Stutter: setas nao travam em nenhuma tela
- [ ] Tema: claro/escuro funciona nas duas telas
- [ ] Fontes: preview mostra fonte carregada, nao crasha
- [ ] LED: modo RGB, sliders R/G/B, velocity
- [ ] HEX picker: editar cor customizada por hex
- [ ] Arredondamento: cantos sao circulos perfeitos
- [ ] Animacoes: entrada com spring, suave
- [ ] Touch Bar: faixa escura no topo da tela inferior

### Ultimas Alteracoes

- `led.c`: corrigida funcao de render corrompida no for loop dos swatches de cor (linhas 424-428 estavam mescladas com argumentos extras)
- `led.c`: removido sentinela redundante `s_morphX < -9000` na inicializacao do morph
- `main.c`: removido `menuInit()` duplicado (ja era chamado por `onScreenEnter(SCREEN_MAIN_MENU)`)
- Build 100% limpo: zero erros, zero warnings
