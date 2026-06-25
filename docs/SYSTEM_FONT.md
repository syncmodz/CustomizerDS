# Fonte de Sistema no 3DS — método seguro (CustomizerDS)

> **Resumo honesto:** trocar a **fonte do sistema** do 3DS é possível e
> **recuperável** (Boot9Strap + NAND backup + GodMode9), MAS exige um arquivo
> no **formato exato** (`A4`, bit depth 16 níveis). Gerar esse arquivo **não é
> automatizável dentro do app** — depende de uma ferramenta de SDK (CTR Font
> Converter) que o `mkbcfnt` do devkitPro **não substitui**. Usar o formato
> errado **embaralha o texto do Home Menu** (o "dano de projeto vibecoded" que
> a comunidade alerta). Por isso o CustomizerDS **não gera nem aplica fonte de
> sistema sozinho** — ele documenta o método provado e (opcionalmente) injeta
> um arquivo **que você já validou**.

## Fatos técnicos confirmados (pesquisa)
- **Título da fonte (padrão):** `0004009B00014002`. Arquivo interno:
  `cbf_std.bcfnt.lz` (BCFNT comprimido com LZ/`lzex`).
- **Bit depth obrigatório:** **A4 (16 níveis de cinza)**. 2-level = texto
  corrompido. O byte de formato de sheet precisa ser o "bom" (`0B`), não `08`.
- **Limite de tamanho:** o `cbf_std.bcfnt.lz` comprimido deve ter **≤ 1.5 MiB**.
- **De onde vem o A4:** da **CTR Font Converter** (ferramenta do CTR SDK da
  Nintendo). O **`mkbcfnt`** (devkitPro, que o app usa pras fontes in-app) **NÃO**
  gera A4 — serve só pra fonte dentro do app, que é 100% segura.
- **FontTool** (astronautlevel2) só faz a parte final: comprime (`lzex`) e
  empacota num `.cia` — ele **não** converte o bit depth.

## Método PROVADO (manual, feito por você no PC)
1. **NAND backup** no GodMode9 (obrigatório — rede de segurança).
2. (Recomendado) Mesclar sua fonte (`.ttf/.otf`) com a fonte oficial do sistema
   no **FontForge**, pra preservar os glifos do sistema (senão faltam caracteres).
3. Converter com a **CTR Font Converter** com **bit depth = 16 levels (A4)** →
   `SystemFont.bcfnt`.
4. Rodar o **FontTool** (`python FontTool.py -font SystemFont.bcfnt`) →
   gera `cbf_std.bcfnt.lz` (≤1.5 MiB) e/ou um `.cia`.
5. Instalar:
   - **Console:** instale o `.cia` pelo GodMode9 (`CIA image options → Install
     game image`), **ou** injete o `cbf_std.bcfnt.lz` no título
     `0004009B00014002` por um script GM9.
   - **Emulador (Azahar/Citra):** coloque o arquivo em
     `/load/mods/0004009B00014002/romfs/cbf_std.bcfnt.lz`.
6. Reiniciar. Se o texto embaralhar/Home não abrir → **restaurar** (abaixo).

## Restaurar a fonte original
- **Console:** instale o `SystemFont.cia` original (ou restaure a NAND backup
  pelo GodMode9).
- **Emulador:** apague o arquivo do `/load/mods/.../romfs/`.

## O papel do CustomizerDS (seguro)
- **Fonte in-app:** 100% segura, é a aba Fontes — só desenha com fontes
  embarcadas, nunca toca NAND. **Inclui a opção "Padrão do Sistema".**
- **Fonte de sistema:** o app **não gera** o arquivo (não dá pra fazer A4 com
  segurança aqui). Se/quando você tiver um `cbf_std.bcfnt.lz` **válido** (feito
  pelo método acima) em `sdmc:/3ds/CustomizerDS/sysfont/`, o app pode gerar o
  **script GodMode9** de injeção + avisar do reboot e do NAND backup. O app
  **nunca** escreve NAND cru nem aplica um arquivo de formato desconhecido.

## Por que não automatizamos
Gerar A4 correto exige a CTR Font Converter (SDK proprietário, GUI Windows) e,
pra cobertura de glifos, a fonte de sistema **dumpada do seu console**. Tentar
hand-rollar isso e aplicar na NAND sem validação é exatamente o caminho que já
causou texto corrompido em consoles reais. A escolha segura é documentar o
método provado e só injetar arquivos que **você** validou.

Refs: aromakitsune *3DS Custom System Fonts* / *System Font Customization*;
GBAtemp *Customising a System Font* e a *PSA* de fontes bugadas;
astronautlevel2 *FontTool*.
