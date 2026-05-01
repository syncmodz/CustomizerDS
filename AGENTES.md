# AGENTES.md - CustomizerDS: Correção e Validação Completa

**Data:** 01/05/2026  
**Usuário:** chicharito  
**Objetivo:** Corrigir sintaxe do `main.c` e validar o projeto

---

## ⚠️ PROBLEMA CRÍTICO ATUAL

O arquivo `/home/chicharito/CustomizerDS/source/main.c` tem erros de sintaxe que persistem:
1. **Faltam vírgulas** em chamadas de função
2. **Nomes incorretos** de constantes e funções
3. **Função inexistente** sendo chamada

---

## 📋 INSTRUÇÕES PARA CORREÇÃO MANUAL

### PASSO 1: Backup do main.c atual
```bash
cp /home/chicharito/CustomizerDS/source/main.c /home/chicharito/CustomizerDS/source/main.c.backup
```

### PASSO 2: Remover main.c com erros
```bash
rm /home/chicharito/CustomizerDS/source/main.c
```

### PASSO 3: Criar main.c correto (COPY & PASTE)

**⚠️ IMPORTANTE:** Copie TODO o código abaixo e cole no terminal:

```bash
cat > /home/chicharito/CustomizerDS/source/main.c << 'MAINEOF'
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "menu.h"
#include "assets.h"
#include "fonts.h"
#include "darkmode.h"
#include "led.h"
#include "config.h"

C3D_RenderTarget *topTarget, *botTarget;

int main() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    
    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    
    int currentScreen = SCREEN_MAIN_MENU;
    menuInit();
    
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        
        if (kDown & KEY_START) break;
        
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(20, 20, 30, 255));
        C2D_TargetClear(botTarget, C2D_Color32(20, 20, 30, 255));
        
        C2D_SceneBegin(topTarget);
        switch (currentScreen) {
            case SCREEN_MAIN_MENU:
                menuRender(kDown, kHeld, &currentScreen);
                break;
            case SCREEN_ASSETS:
                assetsRender(kDown, kHeld, &currentScreen);
                break;
            case SCREEN_FONTS:
                fontsRender(kDown, kHeld, &currentScreen);
                break;
            case SCREEN_DARKMODE:
                darkmodeRender(kDown, kHeld, &currentScreen);
                break;
            case SCREEN_LED:
                ledRender(kDown, kHeld, &currentScreen);
                break;
        }
        
        C2D_SceneBegin(botTarget);
        C2D_TextBuf buf = C2D_TextBufNew(1024);
        C2D_Text info;
        C2D_TextParse(&info, buf, "Pressione START para sair");
        C2D_TextOptimize(&info);
        C2D_DrawText(&info, 0.3f, 10, 200, 0.3f, 0.3f, C2D_Color32(200, 200, 200, 255));
        C2D_TextBufDelete(buf);
        
        C3D_FrameEnd(0);
    }
    
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
MAINEOF
```

### PASSO 4: Verificar se o main.c foi criado corretamente
```bash
head -30 /home/chicharito/CustomizerDS/source/main.c
```

**Verifique se:**
- Linha 5: `#include "common.h"` (com aspas retas)
- Linha 17: `C3D_DEFAULT_CMDBUF_SIZE` (não `C3D_DEFAULT_CMDBUF_SIZE`)
- Linha 18: `C2D_DEFAULT_MAX_OBJECTS` (com S no final)
- Linha 22: `GFX_BOTTOM` (não `GFX_BOTTOM`)
- Linha 24: `SCREEN_MAIN_MENU` (não `SCREEN_MAIN_MENU`)
- Linha 34: `C3D_FRAME_SYNCDRAW` (não `C3D_FRAME_SYNC_DRAW`)
- Linha 35: `C2D_Color32(20, 20, 30, 255)` (com vírgulas)
- Linha 38: `C2D_SceneBegin` (não `C2D_SceneBegin`)
- Linha 41: `menuRender(kDown, kHeld, &currentScreen)` (com vírgulas)
- Linha 60: `C2D_TextParse(&info, buf, "Pressione START para sair")` (com vírgulas)
- Linha 62: `C2D_DrawText(&info, 0.3f, 10, 200, 0.3f, 0.3f, C2D_Color32(200, 200, 200, 255))` (com vírgulas)

---

## 🔧 COMPILAÇÃO E TESTE

### PASSO 5: Compilar o projeto
```bash
cd /home/chicharito/CustomizerDS && make clean && make 2>&1 | tee build/log_compilacao.txt
```

**Se houver erros:**
- Verifique o arquivo `build/log_compilacao.txt`
- Corrija os erros manualmente com `nano` ou `vim`

### PASSO 6: Gerar o CIA
```bash
cd /home/chicharito/CustomizerDS/build && \
/home/chicharito/bin/makerom -f cia -o CustomizerDS_FINAL.cia \
-rsf app.rsf -elf CustomizerDS.elf -icon CustomizerDS.smdh
```

### PASSO 7: Copiar para Downloads
```bash
cp /home/chicharito/CustomizerDS/build/CustomizerDS_FINAL.cia /home/chicharito/Downloads/
```

---

## 🎮 TESTE NO 3DS

### PASSO 8: Montar SD Card
```bash
# Verificar se o SD card está conectado
lsblk | grep sd

# Montar (substitua /dev/sda pelo dispositivo correto)
mount /dev/sda /mnt/sd

# Verificar se montou
ls /mnt/sd/
```

### PASSO 9: Copiar CIA para SD Card
```bash
mkdir -p /mnt/sd/cias
cp /home/chicharito/Downloads/CustomizerDS_FINAL.cia /mnt/sd/cias/CustomizerDS.cia
sync
umount /mnt/sd
```

### PASSO 10: Testar no 3DS
1. Insira o SD card no 3DS
2. Ligue o 3DS
3. Abra o FBI
4. Vá em `SD` -> `cias` -> `CustomizerDS.cia`
5. Instale o CIA
6. Saia do FBI e abra o CustomizerDS pelo menu

---

## 📝 ATUALIZAÇÃO DO RESUMO

### PASSO 11: Atualizar o arquivo de resumo
```bash
nano /home/chicharito/Downloads/CustomizerDS_Resumo.txt
```

Adicione:
```
[✅] 5. MAIN.C CORRIGIDO (01/05/2026)
- main.c reescrito com sintaxe correta
- Vírgulas adicionadas em todas as chamadas de função
- Nomes de constantes corrigidos (GFX_BOTTOM, SCREEN_MAIN_MENU, etc.)
- Removido C2D_Prepare() inexistente
- Compilação limpa funcionando
- .CIA gerado e testado no 3DS
```

---

## 🛠️ COMANDOS ÚTEIS PARA DEPURAÇÃO

### Verificar erros de compilação:
```bash
cd /home/chicharito/CustomizerDS && make 2>&1 | grep -i "error\|warning"
```

### Limpar e recompilar do zero:
```bash
cd /home/chicharito/CustomizerDS && rm -rf build && make
```

### Verificar se o CIA é válido:
```bash
hexdump -C /home/chicharito/Downloads/CustomizerDS_FINAL.cia | head -3
# Deve mostrar: 20 20 00 00 (magic bytes do CIA)
```

### Verificar crash dumps no 3DS:
```bash
# Após testar no 3DS, monte o SD e verifique:
ls /mnt/sd/luma/dumps/arm11/
```

---

## ✅ CHECKLIST FINAL

- [ ] main.c reescrito com sintaxe correta
- [ ] Compilação limpa (sem erros)
- [ ] .3dsx gerado em build/
- [ ] .CIA gerado em build/
- [ ] CIA copiado para Downloads
- [ ] SD card montado
- [ ] CIA copiado para /mnt/sd/cias/
- [ ] Testado no 3DS real
- [ ] Sem crashes no 3DS
- [ ] Resumo atualizado

---

## 📞 CONTATO E SUPORTE

Se encontrar problemas persistentes:
1. Verifique o arquivo `build/log_compilacao.txt`
2. Leia o erro com atenção
3. Corrija manualmente o arquivo afetado
4. Recompile e teste novamente

**Lembre-se:** O 3DS OLD tem apenas 64MB RAM e CPU 268MHz - mantenha o código simples!

---

**FIM DO ARQUIVO AGENTES.md**
