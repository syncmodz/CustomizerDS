# Referências de design — observação real, não suposição

Este arquivo documenta o que foi **efetivamente visto** nas 3 referências pedidas. Como não tenho navegador gráfico nativo, usei `chromium --headless --screenshot` para capturar as páginas de verdade e li os PNGs resultantes com a ferramenta de leitura de imagem (não apenas o texto via fetch, que para essas 3 páginas é quase todo decorativo/sem specs). Onde a página não fornece um valor técnico exato (hex, ms, px), isso é dito explicitamente — não foi inventado.

Capturas brutas ficaram em `/tmp/refshots/` (fora do repo, são só material de trabalho da sessão).

---

## 1. uxmisfit.com — macOS UI Design / Touch Bar

URL: https://uxmisfit.com/2017/01/16/macos-ui-design-the-best-free-resources-inspiration/

Visitei a página inteira (screenshot de 1400×9000px, rolada do topo ao footer). É um artigo de curadoria de links (UI Kits, templates de ícone, kit de Touch Bar), **não tem nenhum valor técnico em texto** (sem hex, sem px) — a única informação de design real está nas imagens dos kits embutidos. A seção que importa de verdade é a "Touch Bar", com mockups reais do MacBook Pro Touch Bar UI Kit. O que dá pra observar diretamente nessas imagens:

- **Fundo da barra**: cinza-chumbo bem escuro, quase preto, com um leve gradiente/brilho horizontal (não é um preto 100% chapado, tem profundidade sutil). Confirma a decisão já tomada no código (`UI_BottomBackground` usando `#0A0A0B`/`#0A0A0A` em vez do cinza-azulado que tínhamos antes).
- **Botões/chips dentro da barra**: retângulos com cantos discretamente arredondados (raio pequeno, NÃO é pílula completa na maioria dos casos) — cinza médio, claramente mais claro que o fundo da barra, texto/ícone branco pequeno e centralizado.
- **Botão de item único/destaque**: nos exemplos com "um item principal", o botão ativo aparece com um preenchimento **azul sólido** (o azul de sistema da Apple), contrastando com o cinza dos outros — confirma usar accent **sólido** (não tingimento translúcido fraco) no estado selecionado.
- **Botão circular isolado**: o ícone da Siri, à direita em todos os exemplos, é um **círculo perfeito** (não retângulo arredondado) — bom padrão para ícones de ação única (ex.: nosso ícone de tema/LED como botão circular).
- **Agrupamento**: os controles ficam em grupos com espaçamento maior ENTRE grupos do que DENTRO de um grupo — start/end (ESC à esquerda, brilho/volume/Siri à direita) ficam isolados nas pontas.

A galeria "Inspiration" no fim da página é só um embed genérico de um diretório de designs (iOS Up), sem relação direta com Touch Bar — não influencia decisões.

**Decisão aplicada**: já bate com o que fizemos na sessão anterior (barra quase preta, chips cinza claramente mais claros que a barra, accent sólido no selecionado, botão circular para ícone único). Não havia hex exato disponível na página; usamos os tons reais do sistema de cor do macOS (`#0A84FF` azul de sistema) como melhor aproximação, já implementado em `theme.c`.

---

## 2. 60fps.design/shots/filter/button

URL: https://60fps.design/shots/filter/button

Visitei e rolei a página (screenshot 1400×9000px). **Achado importante**: a maior parte do catálogo é **paga (PRO)** — "Access all shot filters with PRO subscription. Unlock with PRO" aparece logo depois dos primeiros exemplos, e o resto da página é espaço vazio (placeholder de scroll infinito que não carrega mais nada sem assinatura). Isso limita o que dá pra observar de verdade a uns 6-9 cards gratuitos visíveis: **Avec** ("Button Morph Scale to Video Interaction"), **CRED** ("Voucher Claim Now Button Tap Interaction"), **Telegram** ("Real Time Translation Feature Site"), e mais 2-3 parcialmente visíveis.

Outro ponto: os exemplos são **vídeos com botão de play**, não GIFs autoplay — o screenshot mostra o primeiro frame **pausado** de cada vídeo (um mockup de telefone parado), não a animação em si. Não dá pra capturar o movimento real (squash/stretch, timing exato) sem reproduzir o vídeo, o que está fora do meu alcance de ferramentas aqui. Sendo honesto: o que vi confirma a ESTÉTICA dos cards (mockup de telefone, fundo claro neutro, badge da marca, contraste alto) mas não me dá curvas de easing/duração exatas — isso já era esperado e está documentado aqui em vez de eu inventar números.

**Decisão aplicada**: mantemos o princípio geral que a página defende textualmente — feedback de toque deve ser imediato e visualmente claro (mudança de cor/escala no toque) — já implementado via o pulso de brilho e o "appearT" com slide em `UI_PillButton`/`UI_TouchBarPill`. Não fabriquei números de easing específicos desse site porque ele não os expõe sem pagar.

---

## 3. Behance — "20 Excellent UI/UX Design Animation Examples"

URL: https://www.behance.net/gallery/67817393/20-Excellent-UIUX-Design-Animation-Examples

Visitei a página inteira (duas capturas, até 20000px de altura) e localizei e ampliei especificamente os 3 exemplos pedidos:

### "Travel Motion Exploration" — Nick Taylor
Cover real observado: site de viagem em tela cheia, foto de fundo de uma cabana de montanha (tons frios, dessaturados, azul/cinza/verde escuro). Título grande em branco "STUNNING CABIN" no canto superior esquerdo, botão pílula vermelho/coral "EXPLORE NOW" abaixo do título. No canto superior direito, um ícone de rota pontilhada (mapa) com texto pequeno ao lado. No canto inferior esquerdo, um índice grande "03" (numeração do slide atual). No canto inferior direito, dois botões circulares de navegação (anterior/próximo).
→ **Leitura de UI**: é um slider/carrossel em tela cheia onde cada slide troca a foto de fundo + headline + CTA + número de índice. A composição em camadas (foto de fundo / ícone de rota e texto / headline e CTA) é exatamente o tipo de estrutura que pede parallax em profundidade — bate com o que já implementamos como as 3 camadas `offsetBg/offset/offsetFg` nas 4 telas do app.

### "Travel Agency Landing Page" — Zak Steele-Eklund
Cover real observado: foto de fundo em tela cheia (paisagem montanhosa escura, deserto/vulcânico), com um **cartão branco sobreposto à esquerda** contendo: título em negrito escuro "Gobustan: A Natural Wonder Hidden in the Heart of the Caucasus", subtítulo pequeno, parágrafo de corpo de texto, e um botão pílula azul "Read More" no final do cartão. Ícones pequenos verticais no canto superior direito da imagem (compartilhar/favoritar). Barra de progresso fina no canto inferior direito da foto.
→ **Leitura de UI**: o padrão central é "cartão de conteúdo flutuando sobre uma imagem de fundo cheia" — exatamente a estrutura de popup que já existe em `popupRender` (card com fundo escurecido por trás). A "expansão de um ponto de origem" pedida no prompt é o scale-in com easeOutBack que já está implementado ali, e que agora também aplicamos à entrada do card principal de cada tela via overshoot em `EASE_OUT_BACK` (main.c).

### "Herman Miller Furniture Picker" — Andrew Baygulov
Cover real observado: foto de ambiente (sala de estar/jantar, paredes em tons terrosos/taupe, piso de madeira) com mobília real (poltrona de couro caramelo, luminárias pendentes esféricas brancas em alturas diferentes, mesa de madeira). Pequenos marcadores circulares (hotspots) sobrepostos sobre os móveis na foto — é um "seletor de produto" clicável dentro de uma foto de estilo de vida. Abaixo da foto, um painel de detalhes em duas colunas: lista de itens com preço à esquerda, nome da coleção em negrito ("Darcy Collection") + descrição à direita.
→ **Leitura de UI**: a "coreografia de entrada" aqui é a revelação **sequenciada** dos hotspots sobre a foto e das linhas de preço no painel — elementos não aparecem todos de uma vez, aparecem em sequência (stagger). Não temos fotografia de produto no homebrew, então a adaptação (já presente em `UI_StartupLogo`) é abstrata: o badge "CDS" entra primeiro, depois o título, depois o subtítulo, em fases — o mesmo princípio de revelação em estágios, adaptado ao que o app realmente tem (logo + título + 3 cards), como o próprio prompt pede explicitamente.

---

## Resumo — o que muda de fato no código por causa disso

| Referência | O que já bate com o código atual | O que foi ajustado nesta rodada |
|---|---|---|
| Touch Bar (uxmisfit) | Barra quase preta, chips cinza claro, accent sólido no selecionado, botão circular | Confirmado visualmente, sem mudança adicional necessária |
| 60fps.design | Feedback de toque imediato e visualmente claro | Sem dados de easing exatos disponíveis (página paga); mantido o pulso/slide já implementado |
| Travel Motion (Nick Taylor) | Parallax em 3 camadas já implementado nas 4 telas | Confirma a estrutura; ver `docs/animation-map.md` |
| Travel Agency (Zak Steele-Eklund) | Popup com scale-in `EASE_OUT_BACK` já implementado | Estendido para o overshoot de entrada do card principal (main.c) |
| Herman Miller (Andrew Baygulov) | Splash em fases já implementado em `UI_StartupLogo` | Confirma a estrutura de stagger; sem mudança estrutural necessária |
