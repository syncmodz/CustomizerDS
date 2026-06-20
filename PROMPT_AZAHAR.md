# Testar CustomizerDS no Azahar

## 1. Abrir no Azahar
```bash
azahar /home/chicharito/CustomizerDS/build/CustomizerDS.3dsx
```

## 2. Testes Obrigatarios

### Stutter
- Navegar entre os 3 cards do menu com setas cima/baixo
- Entrar em "LED RGB", mudar modo com setas esquerda/direita
- Ajustar slider R/G/B com setas
- Verificar: SEM travamentos, mudancas suaves

### Tema Claro/Escuro
- Entrar em "Tema do app"
- Alternar entre Claro e Escuro (toque ou botao A)
- Verificar: AMBAS as telas (cima e baixo) mudam de cor

### Preview de Fontes
- Entrar em "Fontes"
- Navegar entre as opcoes com setas
- Verificar: top screen mostra preview da fonte
- Pressionar A para aplicar

### Arredondamento
- Verificar: cantos sao circulos perfeitos em cards, botoes, paineis

### LED
- Testar todos os 4 modos: RGB, Fixo, Pulso, Off
- Ajustar sliders R/G/B com touch (arrastar)
- Verificar: slider nao trava, preview atualiza suave

### Touch Bar macOS
- Bottom screen deve ter faixa Touch Bar no topo (44px)
- Botoes em capsula com prefixo icone
- Key help bar no fundo da tela

## 3. Screenshots
```bash
grim /home/chicharito/CustomizerDS/test_screenshot.png
```

## 4. Se crashar ao abrir Fontes
Proavar: problema de carregamento de fonte .bcfnt.
Verificar:
```bash
ls -la /home/chicharito/CustomizerDS/romfs/fonts/
```
Cada .bcfnt deve ter tamanho > 0.
