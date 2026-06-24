# Animation spec — contrato (PROMPT_CustomizerDS_v2.md seção 3)

Curvas implementadas em `source/anim.c` (`easeFunc`), nunca linear. Engine de tween genérica em `source/anim.c`/`anim.h` (`Tween`/`tweenStart`/`tweenUpdate`/`tweenValue`). Este documento será atualizado com "implementado em `arquivo:função`" à medida que cada item for codado — é o contrato, não uma lista de intenção.

## 3.1 — Splash (boot)
- Fundo preto → `bg_base`: 250ms, `easeOutCubic`.
- Logo "CDS": escala 0.85→1.0 + opacidade 0→1, `easeOutBack`, 420ms.
- Título+tagline: 100ms depois do logo, sobe 12px + fade, `easeOutCubic`, 360ms.
- 3 cards da home: stagger t=600/680/760ms, sobe 16px + fade + escala 0.96→1.0, `easeOutCubic`, 340ms.
- Total: ~1.1s.
- **Implementado em**: `source/ui.c::UI_StartupLogo` (fundo 250ms easeOutCubic, badge "CDS" escala 0.85→1.0 easeOutBack + fade 420ms, título/tagline a partir de 100ms, sobe 12px + fade 360ms easeOutCubic) + `source/menu.c::menuRenderStartupPills` (3 pills com `START_T={0.60,0.68,0.76}`, `DUR=0.34f`, slide 16px + fade + escala 0.96→1.0, `easeFunc(...,EASE_OUT_CUBIC)`) + `source/main.c` (`STARTUP_DURATION = 1.15f`, alinhado ao total ~1.1s da spec). Verificado: build limpo (zero erros/warnings) e deploy no sdmc do Azahar com sha256sum confirmado.

## 3.2 — Transição entre telas
- Tela que sai: 24px sentido oposto + fade→0 + escala 1.0→0.98, `easeInOutCubic`, 280ms.
- Tela que entra: 24px no sentido da navegação + escala 0.98→1.0 + fade, `easeOutCubic`, 320ms, começando 60ms após o início da saída.
- Stagger de 40ms entre elementos internos, 10px + fade, `easeOutCubic`, 260ms.
- Direção: abrir função = direita; voltar (B) = esquerda.
- **Implementado em**: `source/main.c` (`transClock`/`transActive`/`navDir`, decide direção: `SCREEN_MAIN_MENU` = voltar = esquerda, qualquer outra tela = abrir função = direita; calcula `enterE = easeFunc(enterRaw, EASE_OUT_CUBIC)` com o delay de 60ms embutido no `clampf((transClock-0.06f)/0.32f,...)`, e `dirSlideX/dirFadeA/dirScaleM` repassados a cada `*RenderTop`/`*RenderBottom`). Cada uma das 8 funções (`menu.c`, `fonts.c`, `darkmode.c`, `led.c`) aplica `slideX` ao card/conteúdo principal (centrado, com `scaleM` 0.98→1.0) e termina com um veu (`UI_Fill` na cor de fundo com alpha `255*(1-fadeA)`) cobrindo a área do card — fade real mesmo sobre widgets sem alpha próprio (`UI_StatChip`, `UI_Badge`), sem precisar repassar alpha por dentro de cada um. Stagger interno exato (40ms/260ms/`easeOutCubic`) via `UI_StaggerT`/`UI_EnterSeconds` (`source/ui.c`), aplicado às listas reais: chips do Início, linhas de Fontes, swatches de Tema, chips do LED.
- **Simplificação assumida e não escondida**: a tela antiga NÃO é desenhada/animada saindo — como a troca de tela é instantânea no modelo de input atual (sem estado da tela anterior preservado por múltiplos frames), só a tela que entra recebe o tratamento completo de slide+fade+scale; a "saída" de 280ms do enunciado não tem uma renderização própria. O `EASE_OUT_BACK` que dava overshoot ao card foi removido e substituído por `EASE_OUT_CUBIC` (overshoot não é mais usado em transição de tela, só onde a spec pede explicitamente, ex. splash/popup).

## 3.3 — Pop-up/modal de função
- Scrim `#000` 0%→55%, `easeOutCubic`, 220ms.
- Modal nasce na posição/escala do elemento que o abriu, cresce escala 0.6→1.0 + opacidade 0.4→1.0, `easeOutBack` (overshoot ~1.5), 360ms.
- Fechar: caminho inverso, `easeInCubic`, 240ms.
- Aplicado a: abrir o seletor hex (tela Tema).
- **Implementado em**: `source/darkmode.c`. Origem real do toque guardada em `s_hexOriginX/s_hexOriginY` nos dois pontos de entrada (toque direto no swatch HEX e confirmação via botão A, ambos calculam a mesma posição de grade usada no render). Crescimento: `scaleG = easeOutBackAmp(growT, 1.5f)` (`source/anim.c`, nova função com overshoot literal 1.5 — `EASE_OUT_BACK` padrão do `easeFunc` usa 1.70158 e não serviria para o valor exato da spec) sobre 360ms; posição usa `easeFunc(growT, EASE_OUT_CUBIC)`. Scrim: `easeFunc(t, EASE_OUT_CUBIC)*0.55` sobre 220ms. Fechar: `s_hexClosing`/`s_hexCloseT`, 240ms, nova curva `EASE_IN_CUBIC` (`t*t*t`, adicionada a `EaseType` — a spec pede "easeInCubic" e não havia essa variante, só `EASE_IN_OUT_CUBIC`); só depois dos 240ms `s_hexEditing` vira false de fato (antes cortava instantâneo).
- **Simplificação assumida e não escondida**: a tela de cima (`darkmodeRenderTop`) não tem um "elemento que abriu" equivalente ao swatch tocado (é um painel físico separado da tela de baixo) — lá o crescimento é ancorado no centro do próprio frame de preview. Na tela de baixo (onde o toque realmente acontece), `colorPickerRender` só recebe `x,y` (sem parâmetro de escala/tamanho), então a "escala 0.6→1.0" do teclado é entregue via posição real (`lerpf` da origem até o destino) + opacidade (veu), não via redimensionamento geométrico de cada tecla.

## 3.4 — Microinterações de toque
- Press: escala→0.96, 90ms, `easeOutCubic` + escurece.
- Release: escala→1.0, `easeOutBack` (overshoot 1.7), 220ms.
- Seleção D-pad: halo pulsante (opacidade 0.25↔0.4, 1.4s, loop, `easeInOut`) + escala 1.0→1.03; anel desliza 180ms `easeOutCubic`.
- Swatches: bounce 1.0→1.15→1.0, `easeOutBack`, 300ms.
- **Implementado em**: `source/ui.c::UI_PillButtonPress` (nova; `UI_PillButton` agora é um wrapper fino que chama com `pressScale=1.0`) — press 0.96/90ms/`EASE_OUT_CUBIC` + escurece (`themeMix` proporcional ao quão apertado), release 1.0/220ms/`EASE_OUT_BACK` (c1 padrão 1.70158 já bate com o "overshoot 1.7" da spec, sem precisar de variante nova); halo do estado `selected` reescrito para os valores literais da spec (opacidade 0.25↔0.4 a cada 1.4s, `EASE_IN_OUT_CUBIC` sobre uma onda triangular, + escala 1.0→1.03), visível continuamente nos 3 botões de navegação do Início (cada um usa `UI_PillButton`, que herda esse halo automaticamente). Bounce de swatches (`source/darkmode.c`): `s_swatchBounceT`/`s_swatchBounceIdx`, disparado nos 3 caminhos que mudam o accent (toque, D-pad esq/dir, confirm) — 1.0→1.15 (`EASE_OUT_CUBIC`, primeira metade) →1.0 (`EASE_OUT_BACK`, segunda metade), 300ms total. O anel do accent "ativo" em `darkmode.c` também ganhou o mesmo halo pulsante 0.25↔0.4/1.4s (é um círculo perfeito, `w==h`, então fica no caminho seguro de `UI_RoundRect` sem reintroduzir o artefato de blobs em alpha parcial).
- **Não aplicado, e por quê**: o `pressScale` real de `UI_PillButtonPress` não foi conectado aos 3 pills de navegação do Início porque lá o toque já navega instantaneamente no mesmo frame (`menuUpdate` troca de tela no `touchDown`) — o squash de 90ms nunca chegaria a ser visível antes da tela mudar, e adicionar um delay artificial de navegação só para mostrar o press não foi pedido e mudaria a sensação de resposta imediata já validada pelo usuário. O anel de seleção dos chips do Início (`menu.c`) foi deliberadamente mantido opaco/sem pulso — ele é um retângulo (`chipW != chipH`), e dar alpha pulsante reintroduziria o artefato de "blobs" nos cantos já corrigido nesta mesma sessão.

## 3.5 — Sliders do LED (Touch Bar)
- Trilho: cápsula, altura ~10px, fundo branco@10%.
- Preenchimento: gradiente na cor do canal.
- Thumb: círculo branco, escala 1.0→1.18 `easeOutBack` ao arrastar, trilho com glow na cor do canal.
- Valor numérico conta animado (tween 180ms `easeOutCubic` sobre o inteiro exibido).
- Preview em tempo real a cada frame.
- Modo Pulso: senoide `0.5 + 0.5*sin(time*vel)`.
- **Implementado em**: `source/ui.c::UI_TouchBarSliderDrag` (nova; `UI_TouchBarSlider` é wrapper fino com `thumbScale=1.0`/`displayValue=value`). Trilho: cápsula `barH=10` (era 5), `{255,255,255,26}` (branco@10%). Preenchimento: `UI_GradientH` (nova, mesmo princípio de `UI_GradientV` já existente) do canal escurecido 35% até a cor cheia. Thumb: círculo branco (`UI_RoundRect` com `w==h`, caminho seguro), raio `7*thumbScale`; glow no trilho (`{canal, a:30 ou 70}`) e glow extra atrás do thumb quando `thumbScale>1.001`. `source/led.c`: `s_thumbTween[5]`/`s_valTween[5]` (índices 1-4 = R/G/B/Vel), `updateSliderTweens(dt)` chamada incondicional no topo de `ledUpdate` — detecta início/fim de arrasto via `s_draggingSlider` (`tweenStart` para 1.18 `EASE_OUT_BACK` ao iniciar, para 1.0 `EASE_OUT_CUBIC` ao soltar) e qualquer mudança no valor real (`tweenStart` 180ms `EASE_OUT_CUBIC` para a contagem). Pulso: `previewColor()` corrigida para a fórmula literal `0.5+0.5*sin(time*vel)` (antes usava `0.25+0.75*sin(time*(1.2+vel))`, uma aproximação de uma versão anterior). Preview em tempo real: já era por frame (sem mudança necessária).
