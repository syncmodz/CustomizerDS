# Design tokens — contrato (PROMPT_CustomizerDS_v3.md seção 2)

Definidos em `source/theme.c` (`DARK_BASE`/`LIGHT_BASE`) e `source/theme.h` (`ThemePalette`, constantes `RADIUS_*`/`SPACE_*`, `themeAccentGlow()`/`themeAccentSoft()`/`themeCardSelBg()`). Light mode segue a mesma lógica de hierarquia, com valores próprios (a spec só deu números para dark mode). **Esta tabela reflete os valores atuais (v3) — a rodada v2 usava números levemente diferentes (bg_base #0E0E10, card raio 20px etc.), substituídos aqui.**

| Token da spec | Valor pedido | Campo/função | Valor real no código |
|---|---|---|---|
| `bg_base` | `#0E0D12` | `background` / `backgroundTop` | `(14,13,18)` / `(16,15,20)` |
| `bg_card` | `#17151C` | `surface` | `(23,21,28)` alpha 235 (camada translúcida intencional) |
| `bg_card_sel` | mix(bg_card, accent, 8%) | `themeCardSelBg()` | `themeMix(g_theme.surface, g_theme.accent, 0.08f)` — calculado em tempo real, não é uma cor fixa (depende do accent ativo) |
| `stroke` | branco @ 7% | `divider` | `(255,255,255,18)` (18/255 ≈ 7%) |
| `text_primary` | `#F3F1F7` | `textPrimary` | `(243,241,247)` |
| `text_secondary` | branco @ 52% | `textSecondary` | `(255,255,255,133)` (133/255 ≈ 52%) |
| `accent` | `#FF375F` (Rosa) | `g_theme.accent` quando o preset "Rosa" está selecionado | `ACCENTS[3] = {255,55,95}` em `theme.c` — preset persistido no config, por isso já aparece como padrão |
| `accent_glow` (v2) | accent @ 35% | `themeAccentGlow()` | `a=89` (≈35%) — mantido para halo/sombra do elemento ativo |
| `accent_soft` (v3, novo) | accent @ 14% | `themeAccentSoft()` | `a=36` (≈14%) — pra tint sutil Monet em superfícies selecionadas, halos contidos dentro do raio do card |
| `card` raio | 24px | `RADIUS_CARD` (`theme.h`) | `UI_Card(...)` chamado com `RADIUS_CARD` nas 4 telas (era 20px na v2, depois 16px hardcoded antes disso — agora é uma constante nomeada, não mais número solto) |
| `chip/segment` raio | 14px | `RADIUS_CHIP` (`theme.h`) | usado no novo componente `segmentedLozenge` |
| `swatch/pill` raio | full (stadium) | swatches, `UI_PillButton`, segmentado | `r = altura/2`, sempre calculado por widget (nunca um valor fixo — é o que "full" significa) |
| sombra de card | y=8 blur=24 `#000`@40% | `UI_Card`/`UI_Shadow` | `UI_Shadow(x,y,w,h,102,4.0f)` (102/255≈40%, offset 4px — 8px literal ficaria desproporcional num card de 196px numa tela de 240px). Sem blur gaussiano real (motor de desenho não suporta), é a aproximação por 2 retângulos deslocados. |
| espaçamento | múltiplos de 4 | `SPACE_4/8/12/16/24/32` (`theme.h`) | constantes nomeadas agora existem; uso ainda misto com literais antigos nas telas não tocadas nesta rodada — sendo substituído tela por tela conforme a reforma avança |

Não existe nenhum `#000000` puro em nenhuma tela — confirmado por leitura direta de `theme.c` (o mais escuro é `#0E0D12`).
