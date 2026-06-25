# LED de Notificação Persistente no 3DS (CustomizerDS)

> **Resumo dos 3 níveis de persistência:**
> 1. **Enquanto o app roda:** ao vivo (MCU) — sempre funciona.
> 2. **Depois de fechar o app (v1.1):** o app **não apaga mais** o LED ao sair,
>    então o padrão **continua** até um reboot ou uma notificação do sistema.
> 3. **Depois de reiniciar o console:** precisa de um **patch da Luma** no
>    módulo de notificações `0004013000003502` (reaplicado todo boot). **LED é
>    MCU, não NAND → risco de brick ~nulo**; patch ruim no máximo trava o boot e
>    reverte **apagando o `.ips`**.

## ✅ O QUE ATIVAR NA LUMA (obrigatório pra persistência via reboot)
Entre na config da Luma (segure **SELECT** no boot) e ligue:
- [x] **Enable game patching**
- [x] **Enable loading external FIRMs and modules**

Sem essas duas, o patch do módulo `0004013000003502` **não é aplicado** no boot.
Depois de ligar, **reinicie**.

## Como o LED persistente funciona (método provado)
Ferramentas de referência: **CtrRGBPAT2** (Golem642/FarmYard-Gaming) e
**CustomRGBPattern** — ambas geram um **IPS** que a Luma aplica no módulo
`0004013000003502` (news/notificações) todo boot.

- Arquivo do patch vai em **`/luma/titles/0004013000003502/code.ips`** (ou
  `/luma/sysmodules/0004013000003502.ips`).
- Requer, nas **configurações da Luma**: **"Enable game patching"** e
  **"Enable loading external FIRMs and modules"** ligados.
- Depois de copiar o patch + ligar essas opções → **reiniciar**. A Luma reaplica
  o padrão de LED todo boot (inclusive os triggers de boot/saída de sleep).
- **Reverter:** apague `/luma/titles/0004013000003502/code.ips` (e/ou
  `/luma/sysmodules/0004013000003502.ips`) e reinicie.

## Estrutura do padrão
O padrão RGB é o mesmo conceito do controle ao vivo: amostras R/G/B (32) +
parâmetros de delay/smoothing/loop. O IPS escreve esses bytes no offset do
módulo — **e esse offset depende da versão do firmware** do módulo de
notificações, por isso ferramentas como a CtrRGBPAT2 mantêm os offsets certos
por versão. **Gerar o IPS errado (offset de outra versão) pode travar o boot**
(recuperável apagando o `.ips`).

## O papel do CustomizerDS
- **Ao vivo (hoje, seguro):** a aba LED aplica cor/modo na hora via MCU e
  **restaura o LED ao sair** do app.
- **Persistente:** por ser **dependente de versão** e **não testável** sem o seu
  console, o app **não gera o IPS automaticamente às cegas** (um offset errado
  trava o boot). O caminho seguro/provado é usar a **CtrRGBPAT2** (que mantém os
  offsets por firmware) pra gerar o patch e ligar o game patching da Luma. Se
  você quiser, dá pra integrar um gerador no app **depois** de fixarmos os
  offsets da SUA versão e validarmos no seu 3DS.

Refs: CtrRGBPAT2 (Golem642 / FarmYard-Gaming), CustomRGBPattern (GameBrew),
Luma3DS game patching, 3dbrew `MCURTC:SetInfoLEDPattern`.
