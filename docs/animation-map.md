# Mapa de animação — contrato de design

## Brainstorm (skill brainstorm-experiments-existing / brainstorm-ideas-new) — antes de codar

Para os 3 momentos-chave, comparei 2-3 abordagens (perspectiva PM / Designer / Engenheiro) antes de implementar a primeira ideia, como o prompt exigiu. Decisões e rejeições com motivo:

**Splash (Herman Miller).** A: manter só logo→título→subtítulo (estado atual). B: estender a coreografia para incluir uma prévia das 3 pílulas de função na tela de baixo, aparecendo em stagger durante o boot. C: trocar o bounce do badge por squash&stretch.
→ **Escolhido: B.** O prompt pede explicitamente "adapte isso para a entrada do logo CDS + nome CustomizerDS + **os 3 cards da home**" — o estado atual não anima os 3 cards/pills durante o splash, só o logo. É uma lacuna real, não refinamento cosmético. Implementado em `main.c`/`menu.c`.

**Transição entre telas (Travel Motion).** A: manter parallax 3 camadas + overshoot (atual). B: direção do slide dependente do sentido da navegação (entrar vs voltar, como push/pop do iOS). C: aumentar a diferença de profundidade entre camadas.
→ **B avaliado e adiado de propósito**: é uma melhoria legítima (consistência espacial), mas não está no critério de sucesso do prompt (que pede só "slide+parallax+stagger, não corte seco" — já satisfeito) e exigiria diferenciar `TRANSITION_SLIDE_UP` por sentido em todas as 4 telas. Priorizei o que está no checklist explícito antes de adicionar algo não pedido.

**Pop-up de função (Travel Agency).** A: manter `popupRender` só na confirmação do hex. B: aplicar o mesmo pop de entrada (scale+scrim) na ABERTURA do editor de hex (não só na confirmação). C: aplicar scrim+pop também na troca de modo do LED (RGB/Fixo/Pulso/Off).
→ **Escolhido: B. C rejeitado.** B bate com o exemplo literal do próprio prompt ("abrir o seletor de hex na aba Tema"). C foi rejeitado porque troca de modo do LED é uma ação de **alta frequência** (o usuário troca de aba/modo o tempo todo) — transformar isso num modal com scrim a cada toque seria irritante, não "premium"; o morph do segmented control (já implementado, também baseado na estética Touch Bar) é o padrão certo para esse tipo de controle frequente.


Este arquivo mapeia cada tela/elemento do app contra o tipo de animação, a referência usada (ver `docs/design-references.md`) e a duração/easing **real** já presente no código (`source/*.c`), não um valor planejado-mas-não-implementado. Se a implementação mudar, este arquivo deve ser atualizado junto.

| Tela / Elemento | Tipo de animação | Referência usada | Duração / Easing real |
|---|---|---|---|
| Splash inicial (boot) — badge "CDS" | Scale-in com bounce | Herman Miller (stagger em fases, adaptado) | 0.30s, `EASE_OUT_BACK`, inicia em t=0.08s |
| Splash inicial — título "CustomizerDS" | Fade-in | Herman Miller | 0.25s, `easeOutCubic`, inicia em t=0.28s |
| Splash inicial — subtítulo | Fade-in | Herman Miller | 0.25s, `easeOutCubic`, inicia em t=0.42s |
| Splash inicial — saída (badge escala+some) | Scale-out + fade | Herman Miller | 0.23s, lerp linear de escala 1.0→1.12, inicia em t=1.00s |
| Splash total até liberar input | — | — | 1.30s (`STARTUP_DURATION`), ou imediato se houver input antes |
| Início→Fontes / Início→Tema / Início→LED (e volta) | Slide + parallax 3 camadas + overshoot | Travel Motion (Nick Taylor) + Travel Agency (overshoot) | 0.30s, `EASE_OUT_BACK` (transição), camadas a 70px/40px/18px (`offsetBg/offset/offsetFg`) |
| Card principal de cada tela (entrada) | Overshoot leve (ultrapassa e acomoda) | Travel Agency (Zak Steele-Eklund) | Mesmo 0.30s/`EASE_OUT_BACK` da transição — sem estado novo, reaproveita `transitionEased` |
| Conteúdo interno do card (texto/ícones) | Fade + slide, stagger por item | Travel Motion + Herman Miller (stagger) | `UI_EnterProgress()` = `easeOutCubic(g_enterT)`, atraso por item de 0.08–0.10 (`et*2.x - i*0.08..0.10`) |
| Chips de prévia do Início (Fontes/Tema/LED) | Fade + slide individual, stagger | Travel Motion (camadas) + Herman Miller (stagger) | `easeOutCubic`, atraso 0.10 por chip, slide de 12px |
| Page dots do Início | Scale-in com bounce, stagger | Herman Miller | `easeOutBack`, atraso 0.10 por dot, inicia 0.15s depois dos chips |
| Hex picker (abrir popup de cor) | Card expande de origem + scrim escurece | Travel Agency (Zak Steele-Eklund) | `popupUpdate`: 0.35s, `EASE_OUT_BACK`, escala 0.88→1.0; scrim sobe até alpha 0.6 nos primeiros 0.15s |
| Hex picker (fecha automático após confirmar) | Auto-hide temporizado | Travel Agency | Permanece visível ~0.9s após `animT` atingir 0.35 (ver `s_popupHoldT` em `darkmode.c`) |
| Toggle Claro/Escuro (segmentado) | Morph da pílula selecionada | Touch Bar (uxmisfit) — estética; movimento é nosso | `approach()` a 240px/s até a posição alvo |
| Swatches de accent (seleção) | Scale-in com bounce, anel de foco | Touch Bar (cor de destaque sólida) + Herman Miller (stagger) | `easeOutCubic`, atraso 0.08 por swatch |
| Sliders de LED (R/G/B/Vel, arraste) | Feedback contínuo 1:1 (sem easing — é input direto) | Touch Bar (estética de barra/trilho) | Sem animação de atraso; resposta imediata ao toque/D-pad por design (controle direto) |
| Pills da Touch Bar inferior (Fontes/Tema/LED, RGB/Fixo/Pulso/Off) | Fade+slide na entrada, pulso de brilho quando selecionado | Touch Bar (estética) + 60fps.design (feedback de toque) | Entrada: `easeOutCubic`, atraso 0.10 por pill. Pulso: `sin(uiFrameTime()*3.2)` contínuo enquanto selecionado |
| Botão/ícone ao toque (qualquer tela) | Resposta visual imediata (cor sólida no selecionado) | 60fps.design (princípio geral; site não expõe timing exato — ver design-references.md) | Sem delay — muda no mesmo frame do toque/confirm |

## Notas de honestidade

- `60fps.design` não forneceu nenhuma duração/curva exata (página majoritariamente paga); o que aplicamos dali é o **princípio** de feedback imediato e claro, não um número copiado.
- O acoplamento entre "overshoot do card" e "transição de tela" é intencional: em vez de criar um novo sistema de animação paralelo, a entrada do card reaproveita o mesmo `EASE_OUT_BACK` da transição (`transitionEased`), porque ambos representam o mesmo evento (entrar na tela) — duas animações desincronizadas para o mesmo evento pareceriam erradas.
- Sliders de LED não têm easing de atraso de propósito: são controles de manipulação direta (drag/D-pad), e adicionar lag a esse tipo de controle pioraria a sensação de resposta, não melhoraria.
