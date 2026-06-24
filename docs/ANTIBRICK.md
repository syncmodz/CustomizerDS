# Anti-Brick & Fonte de Sistema — Doutrina (CustomizerDS)

> **TL;DR:** O CustomizerDS **nunca escreve na NAND**. A troca de fonte é
> **100% in-app** (segura). Trocar a fonte do **sistema inteiro** é o *único*
> caminho que pode brickar — e **não está implementado** (só documentado aqui
> para o futuro, como export opcional no SD que o usuário instala por conta e
> risco). LED é MCU, não NAND: risco nulo de brick.

Fontes de referência:
- aromakitsune — *"Custom System Fonts"*
- GBAtemp — *"Customising a System Font"*

---

## 1. Por que fonte de sistema NÃO sai por LayeredFS/overlay

- O **LayeredFS do Luma3DS** (e temas tipo **Anemone**) cobre **wallpaper,
  badges e som do HOME Menu** — tudo lido do **SD**, seguro e reversível.
- A **shared system font** (`cbf_std.bcfnt`) **não** é coberta por esse
  overlay: ela vive num **título de sistema** (`0004003000......`) na **NAND**,
  carregado cedo demais / por um caminho que o LayeredFS não intercepta.
- Logo: **não existe** um jeito "só no SD" de trocar a fonte do sistema inteiro
  como se faz com wallpaper. Quem promete isso está confundindo tema de HOME
  Menu com fonte de sistema — **são coisas diferentes**.

## 2. O único caminho real (e por que é perigoso)

Trocar a fonte system-wide exige **substituir o arquivo de sistema** na NAND,
via uma **`.cia` patcheada** do título da fonte. Isso é:

- **Brick real, porém recuperável** — se a fonte estiver corrompida/fora do
  formato, o HOME Menu não inicia.
- Requer **Boot9Strap + backup de NAND + GodMode9** para recuperar.
- Limites do `.bcfnt`: **≤ 1.5 MB** e **bit depth A4 (16 níveis)**. Fora disso,
  brick.

**Regras de segurança (se um dia o usuário fizer isso manualmente):**
1. **Backup de NAND ANTES** (GodMode9), sempre.
2. **Nunca** desinstalar o CFW com arquivos de sistema modificados.
3. Recuperar via **GodMode9** (restaurar a fonte original / a NAND); `ntrboot`
   como último recurso.

## 3. Decisão para o app (rumo à 1.0)

1. **Fonte in-app = 100% segura.** É o foco da 1.0 — o app só desenha com as
   fontes embarcadas (`romfs/fonts/*.bcfnt`), com fallback pra fonte de sistema
   se um `.bcfnt` não carregar (nunca crasha, nunca escreve nada).
2. **System-wide:** o app **JAMAIS** escreve na NAND. No futuro, *talvez*, um
   **export opcional no SD** (`.cia`/`.bcfnt` pronto) que o **usuário instala
   manualmente**, por conta e risco, com **backup de NAND obrigatório**.
   **NÃO implementado agora** — só esta documentação.
3. **LED (`led.c` / mcuHwc):** risco **baixo** — é o LED de notificação (MCU),
   não NAND; um reboot reverte. Mesmo assim o app **restaura o LED ao sair**
   (`ledExit()` limpa o padrão custom).

## 4. O que o app toca (e o que NÃO toca)

| Recurso | Onde escreve | Risco |
|---|---|---|
| Config (tema, accent, fonte, LED, **idioma**) | `sdmc:/3ds/CustomizerDS/config.bin` | nenhum (SD) |
| Fontes | só **lê** `romfs:/fonts/*.bcfnt` (embarcadas) | nenhum |
| LED RGB | MCU (`mcuHwc`), padrão volátil | baixo (reboot reverte; restaurado no exit) |
| **NAND / títulos de sistema** | **NUNCA** | — |

**Garantia de código:** não há nenhuma escrita fora de
`sdmc:/3ds/CustomizerDS/` (ver `config.c::configSave`) e nenhuma chamada a APIs
de NAND/AM de instalação. O único hardware tocado além do SD é o LED de
notificação via MCU, que é volátil e restaurado na saída.
