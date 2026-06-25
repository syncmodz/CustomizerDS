# Segurança & Recuperação (CustomizerDS)

> A "doutrina antibrick" exagerada da v9 foi **encolhida**: trocar fonte de
> sistema **não é tabu** — é possível e **recuperável** (Boot9Strap + NAND
> backup + GodMode9). O que continua valendo são as **boas práticas mínimas**.

A documentação detalhada e factual está em:
- **[SYSTEM_FONT.md](SYSTEM_FONT.md)** — fonte de sistema: formato A4, limites,
  método provado (CTR Font Converter + FontTool + GM9), restore. Por que o app
  **não** auto-gera (risco de texto corrompido).
- **[LED_PERSIST.md](LED_PERSIST.md)** — LED persistente via patch da Luma
  (`0004013000003502`), settings necessárias, reverter.

## Boas práticas mínimas (higiene, não medo)
1. **NAND backup ANTES** de qualquer mudança em fonte de sistema (GodMode9).
2. **Validar formato/tamanho** do `cbf_std.bcfnt.lz` (A4, ≤1.5 MiB) — nunca
   aplicar um arquivo de formato desconhecido.
3. Sempre ter um **restore** à mão (CIA original / NAND backup).
4. **LED é MCU, não NAND** — risco de brick nulo; reverte apagando o `.ips`.

## O que o CustomizerDS toca
- **SD:** `sdmc:/3ds/CustomizerDS/` (config, e `sysfont/` se você exportar).
- **MCU (LED ao vivo):** volátil, restaurado ao sair.
- **NAND:** **nunca** escreve direto. Injeção de fonte de sistema só via script
  GodMode9 a partir de um arquivo **que você validou**.
