# LAYEREDFS_DEBUG — log do diagnóstico "HUD color não aplica"

Console: 3DS **USA**, Luma **13.4**, `enable_game_patching=1`, sem tema custom.
Path: `sdmc:/luma/titles/0004003000008F02/romfs/hud_LZ.bin`.

## Estado consolidado (não re-testar)
| Fato | Evidência |
|---|---|
| Console USA, TID correto | só extdata `0000008f`/`000002cd` no SD |
| Path byte-perfeito, arquivo LZ11 válido | descomprimido direto do SD, cor confere |
| game_patching ON | menu do Luma (tela) + config.ini |
| Compressão LZ11 real (~10KB) | roundtrip validado; stored (53KB) descartado |
| Teste vermelho (relógio FF0000, LZ real) → **não mudou** | reboot no hardware |
| Não é tema | testado sem tema; tema não pinta HUD do topo |
| BCLIM da bateria = A4 (só alpha) | `hud_inspect diff`; cor vem do mat1 |
| Template `hud_usa.bin` sujo (bateria verde 00EE00) | `hud_inspect list`; stock USA = 23AAE6 |

**Conclusão parcial:** ou o Luma não redireciona o romfs do Home Menu, ou o HM rejeita nosso
arquivo e cai no original. Mesmo sintoma → os testes abaixo separam.

## TESTES (data 2026-07-11)

### T-COOOL — pack completo cooolgamer instalado (discriminador principal)
Estado: pack USA completo (hud 10217B idêntico ao original, launcher 124246B, +common/dialog/
MyMenu/etc.) em `/luma/titles/0004003000008F02/romfs/`. **Aguardando reboot do dono.**
- Bottom screen (launcher) muda drasticamente → **RAMO B** (LayeredFS lê; conteúdo nosso é o problema).
- Nada muda → **RAMO A** (LayeredFS morto no ambiente → Luma config/versão/sysmodule).

### T-0 — arquivo inválido (backup, cirúrgico)
Artefato: `scripts/debug/hud_corrupt.bin` (4 bytes `00000000`). Se instalado no path e a Home
**crashar** = LayeredFS lê o hud (RAMO B). Se abrir stock = não lê (RAMO A).
Status: gerado, NÃO deployado (o T-COOOL é mais seguro e mais informativo).

## Resultado — RAMO A CONFIRMADO (2026-07-11)
- **T-COOOL: NÃO mudou nada** → pack consagrado do cooolgamer não aplica → LayeredFS do
  Home Menu **não engata** neste console.
- 2ª prova independente: teste do relógio vermelho (LZ real) também não mudou.

### Ambiente auditado (tudo CORRETO — não é o app):
- `boot.firm` na raiz do SD: **306176 B, md5 `5af1c64187a296d629861643231be9de` = Luma 13.4
  OFICIAL** (idêntico ao release github.com/LumaTeam/Luma3DS/releases/v13.4). Não é build velha.
- `enable_game_patching = 1`, `config_version 3.13`, sem `luma/sysmodules` nem `luma/plugins`.
- Home Menu stock, TID `0004003000008f02` v30720 (dump em `SD:/00008f02` é inócuo).
- Caminho `luma/titles/0004003000008F02/romfs/` byte-perfeito, arquivos válidos e gravados.

### 🎯 CAUSA RAIZ ENCONTRADA (A2): regressão do Luma 13.4.0
- Rosalina confirmou **13.4 rodando de verdade** (A1 morto).
- A2 (pesquisa de regressão): **[LumaTeam/Luma3DS issue #2242](https://github.com/LumaTeam/Luma3DS/issues/2242)**
  — "Regression with Luma 13.4.0: RomFS loading does not work anymore". Introduzida pelo
  PR #2200, corrigida no PR #2245, **sem release estável com o fix** (o último release É o
  13.4 bugado). Bate 100% com os sintomas: tudo correto e nada aplica.

### Fix aplicado (2026-07-11)
- `boot.firm` do SD trocado para **v13.3.3** (pré-regressão, md5 `a231f278b93b1cdcb3da819433828d56`);
  backup do 13.4 em `SD:/boot.firm.v13.4_bugado`.
- Hud de teste tudo-vermelho (`FF0000`, 22 materiais, do template stock novo) instalado em
  `luma/titles/0004003000008F02/romfs/hud_LZ.bin` para validação visual no primeiro boot.
- Aguardando reboot do dono: HUD vermelho = caso encerrado.


## 🏆 CAUSA FINAL ENCONTRADA (2026-07-12): LZ11 dist=1 + boot frio vs quente
1. **TESTE 0 (hud truncado, boot frio): Home abriu NORMAL** → em boot FRIO o `checkLumaDir`
   do loader nao ve o SD a tempo (Home carrega cedissimo) → LayeredFS nunca aplica em boot
   frio NESTE console. Por isso TODOS os testes com desliga/liga falharam (coool incluso).
2. **Apply pelo app → reboot QUENTE (APT reset) → CRASH** → no warm boot o LayeredFS LE o
   arquivo → o hud gerado pelo nosso compressor crashou o decoder da Home.
3. **Diff estrutural dos streams**: encoder oficial (arquivo cooolgamer): `dist_min=2,
   dist1=0`. NOSSO compressor: `dist_min=1, dist1=110`. **O decoder do Home Menu nao
   suporta distancia 1 (VRAM-safe)** → crash.
4. **Fix**: compressores C (`hudcolor.c`) e Python (`lz11.py`) agora exigem dist>=2;
   template `hud_usa.bin` regenerado (`dist_min=2 dist1=0`); hud de teste idem.
5. Corolario: aplicar pelo app + reboot automatico (quente) e o caminho FELIZ — o
   LayeredFS pega. Boot frio perde o patch neste console (limitacao de timing do SD).
