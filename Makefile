TARGET := CustomizerDS
BUILD := build
SOURCES := source
DATA := romfs

export DEVKITPRO ?= /opt/devkitpro
DEVKITARM := $(DEVKITPRO)/devkitARM

BANNERTOOL := /home/chicharito/bin/bannertool
MAKEROM := /home/chicharito/bin/makerom
THREEDSTOOL := /home/chicharito/bin/3dstool
3DSXTOOL := /opt/devkitpro/tools/bin/3dsxtool
MKROMFS := /opt/devkitpro/tools/bin/mkromfs3ds
TEX3DS := /opt/devkitpro/tools/bin/tex3ds

CCPREFIX := $(DEVKITARM)/bin/arm-none-eabi-
CC := $(CCPREFIX)gcc
CXX := $(CCPREFIX)g++
LD := $(CCPREFIX)gcc
AR := $(CCPREFIX)ar
OBJCOPY := $(CCPREFIX)objcopy
OBJDUMP := $(CCPREFIX)objdump

CFLAGS := -Wall -Wno-unused-variable -O2 -mword-relocations -ffunction-sections -fdata-sections \
          -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -marm \
          -I$(DEVKITPRO)/libctru/include \
          -I$(DEVKITPRO)/portlibs/armv6k/include \
          -I$(SOURCES) \
          -DARM11 -D__3DS__

LDFLAGS := -specs=$(DEVKITARM)/arm-none-eabi/lib/3dsx.specs \
           -g -march=armv6k -mtune=mpcore -mfloat-abi=hard \
           -mfpu=vfp -marm -Wl,--gc-sections -Wl,--build-id=none \
           -L$(DEVKITPRO)/libctru/lib \
           -L$(DEVKITPRO)/portlibs/armv6k/lib

LIBS := -lcitro2d -lcitro3d -lctru -lm

APP_TITLE := CustomizerDS
APP_AUTHOR := Syncmaker
APP_ICON := $(DATA)/icon.png
APP_RSF := $(BUILD)/app.rsf
BANNER := $(BUILD)/banner.bin

ELF := $(BUILD)/$(TARGET).elf
3DSX := $(BUILD)/$(TARGET).3dsx
CIA := $(BUILD)/$(TARGET).cia
SMDH := $(BUILD)/$(TARGET).smdh
ROMFS_BIN := $(BUILD)/romfs.bin
ROMFS_RAW := $(BUILD)/romfs_raw.bin

# PROMPT v14 §4 (hardware) -- .cia de fonte de sistema EMBUTIDOS na romfs
# (romfs/sysfont/), instalados in-app na CTRNAND via AM. Um por fonte custom
# (mesma ordem do app; ui_comfortaa_bold e a fonte do chrome, fica de fora) +
# o STOCK (fonte original extraida da NAND do dono). Gerados por
# scripts/mk_sysfont_cia.py (3dstool+makerom+ncchheader.bin). romfs/sysfont/ e
# scripts/sysfont/stock/ sao gitignored (contem fonte do console = Nintendo).
SYSFONT_HDR := scripts/sysfont/ncchheader.bin
SYSFONT_STOCK_LZ := scripts/sysfont/stock/cbf_std.bcfnt.lz
SYSFONT_STOCK_BCFNT := scripts/sysfont/stock/cbf_std.bcfnt
SYSFONT_TTF_DIR := scripts/sysfont/ttf
SYSFONT_DIR := $(DATA)/sysfont
SYSFONT_FONTS := comfortaa_regular comfortaa_bold made_evolve_regular made_evolve_bold love_house comic_sans_ms3 super_mario_64
# python com freetype-py p/ o merge (venv do projeto); cai pra python3 se faltar.
PYFT := $(if $(wildcard scripts/sysfont/.venv/bin/python),scripts/sysfont/.venv/bin/python,python3)
# CRITICO: a fonte de sistema custom e feita por MERGE na STOCK (mk_sysfont_merge.py)
# -- NUNCA por mkbcfnt cru (95 glifos -> crasha/brick). So embute a fonte cujo
# TTF/OTF existe localmente (gitignored); STOCK so se o .lz existir. Clone publico
# (sem fontes) builda o app sem essas .cia, sem quebrar.
SYSFONT_EMBED_CIAS :=
$(foreach f,$(SYSFONT_FONTS),$(if $(wildcard $(SYSFONT_TTF_DIR)/$(f).*),$(eval SYSFONT_EMBED_CIAS += $(SYSFONT_DIR)/SystemFont_$(f).cia)))
ifneq ($(wildcard $(SYSFONT_STOCK_LZ)),)
SYSFONT_EMBED_CIAS += $(SYSFONT_DIR)/SystemFont_STOCK.cia
endif

all: $(3DSX) $(CIA)

$(3DSX): $(ELF) $(SMDH) $(ROMFS_RAW)
	$(3DSXTOOL) $< $@ --smdh=$(SMDH) --romfs=$(ROMFS_RAW)

$(ELF): $(BUILD)/main.o $(BUILD)/common.o $(BUILD)/menu.o $(BUILD)/fonts.o $(BUILD)/darkmode.o $(BUILD)/led.o $(BUILD)/theme.o $(BUILD)/anim.o $(BUILD)/ui.o $(BUILD)/input.o $(BUILD)/color_picker.o $(BUILD)/config.o $(BUILD)/icons.o $(BUILD)/transitions.o $(BUILD)/compositor.o $(BUILD)/lang.o $(BUILD)/sysfont.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BUILD)/%.o: $(SOURCES)/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(SMDH): $(APP_ICON)
	$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "Personalize your Nintendo 3DS" -p "$(APP_AUTHOR)" -i $(APP_ICON) -o $@

$(BANNER): banner_source.png audio.wav
	$(BANNERTOOL) makebanner -i $< -a audio.wav -o $@

$(APP_RSF):
	@mkdir -p $(BUILD)
	@echo 'BasicInfo:' > $@
	@echo '  Title                   : $(APP_TITLE)' >> $@
	@echo '  ProductCode             : CTR-P-CTDS' >> $@
	@echo '  Logo                    : Nintendo' >> $@
	@echo '' >> $@
	@echo 'TitleInfo:' >> $@
	@echo '  Category                : Application' >> $@
	@echo '  UniqueId                : 0x0FF000' >> $@
	@echo '' >> $@
	@echo 'Option:' >> $@
	@echo '  UseOnSD                 : true' >> $@
	@echo '  FreeProductCode         : true' >> $@
	@echo '  EnableCrypt             : false' >> $@
	@echo '  EnableCompress          : true' >> $@
	@echo '' >> $@
	@echo 'AccessControlInfo:' >> $@
	@echo '  CoreVersion                   : 2' >> $@
	@echo '  DescVersion                   : 2' >> $@
	@echo '  ReleaseKernelMajor            : "02"' >> $@
	@echo '  ReleaseKernelMinor            : "33"' >> $@
	@echo '  MemoryType                    : Application' >> $@
	@echo '  SystemMode                    : 64MB' >> $@
	@echo '  IdealProcessor                : 0' >> $@
	@echo '  AffinityMask                  : 1' >> $@
	@echo '  Priority                      : 16' >> $@
	@echo '  HandleTableSize               : 0x200' >> $@
	@echo '  DisableDebug                  : false' >> $@
	@echo '  CanWriteSharedPage            : true' >> $@
	@echo '  PermitMainFunctionArgument    : true' >> $@
	@echo '  CanShareDeviceMemory          : true' >> $@
	@echo '  SpecialMemoryArrange          : true' >> $@
	@echo '' >> $@
	@echo '  ServiceAccessControl:' >> $@
	@echo '   - fs:USER' >> $@
	@echo '   - gsp::Gpu' >> $@
	@echo '   - hid:USER' >> $@
	@echo '   - apt:u' >> $@
	@echo '   - am:APP' >> $@
	@echo '   - am:net' >> $@
	@echo '   - ac:u' >> $@
	@echo '   - pxi:dev' >> $@
	@echo '   - mcu::HWC' >> $@
	@echo '' >> $@
	@echo '  SystemCallAccess:' >> $@
	@echo '    ArbitrateAddress: 34' >> $@
	@echo '    Break: 60' >> $@
	@echo '    CancelTimer: 28' >> $@
	@echo '    ClearEvent: 25' >> $@
	@echo '    ClearTimer: 29' >> $@
	@echo '    CloseHandle: 35' >> $@
	@echo '    ConnectToPort: 45' >> $@
	@echo '    ControlMemory: 1' >> $@
	@echo '    CreateAddressArbiter: 33' >> $@
	@echo '    CreateEvent: 23' >> $@
	@echo '    CreateMemoryBlock: 30' >> $@
	@echo '    CreateMutex: 19' >> $@
	@echo '    CreateSemaphore: 21' >> $@
	@echo '    CreateThread: 8' >> $@
	@echo '    CreateTimer: 26' >> $@
	@echo '    DuplicateHandle: 39' >> $@
	@echo '    ExitProcess: 3' >> $@
	@echo '    ExitThread: 9' >> $@
	@echo '    GetCurrentProcessorNumber: 17' >> $@
	@echo '    GetHandleInfo: 41' >> $@
	@echo '    GetProcessId: 53' >> $@
	@echo '    GetProcessIdOfThread: 54' >> $@
	@echo '    GetProcessInfo: 43' >> $@
	@echo '    GetResourceLimit: 56' >> $@
	@echo '    GetResourceLimitCurrentValues: 58' >> $@
	@echo '    GetResourceLimitLimitValues: 57' >> $@
	@echo '    GetSystemInfo: 42' >> $@
	@echo '    GetSystemTick: 40' >> $@
	@echo '    GetThreadId: 55' >> $@
	@echo '    GetThreadPriority: 11' >> $@
	@echo '    MapMemoryBlock: 31' >> $@
	@echo '    OutputDebugString: 61' >> $@
	@echo '    QueryMemory: 2' >> $@
	@echo '    ReleaseMutex: 20' >> $@
	@echo '    ReleaseSemaphore: 22' >> $@
	@echo '    SendSyncRequest: 50' >> $@
	@echo '    SetThreadPriority: 12' >> $@
	@echo '    SetTimer: 27' >> $@
	@echo '    SignalEvent: 24' >> $@
	@echo '    SleepThread: 10' >> $@
	@echo '    UnmapMemoryBlock: 32' >> $@
	@echo '    WaitSynchronization1: 36' >> $@
	@echo '    WaitSynchronizationN: 37' >> $@
	@echo '' >> $@
	@echo 'SystemControlInfo:' >> $@
	@echo '  StackSize: 0x40000' >> $@
	@echo '' >> $@
	@echo 'RomFs:' >> $@
	@echo '  RootPath: $(DATA)' >> $@

# Saida crua do mkromfs3ds (so romfs_header + tabelas, sem hash tree): e o
# formato que romfs_dev.c do libctru espera direto no offset 0 ao montar a
# romfs embutida de um .3dsx -- ele nao interpreta IVFC nenhum nesse caminho.
# Sheet separado das sprites novas da v9 (swatches cakeOS + icone in-app). Os
# PNGs-fonte de icons.t3x nao estao no repo, mas os destas estao -- entao este
# .t3x e regenerado a partir do .t3s (atlas). source/extra_gen.h sai junto.
EXTRA_T3S := $(DATA)/gfx/extra.t3s
EXTRA_T3X := $(DATA)/gfx/extra.t3x
$(EXTRA_T3X): $(EXTRA_T3S) $(DATA)/gfx/swatch_ring_thick_3x.png $(DATA)/gfx/swatch_ring_thin_3x.png $(DATA)/gfx/icon_256.png
	$(TEX3DS) -i $(EXTRA_T3S) -o $(EXTRA_T3X) -H source/extra_gen.h

$(ROMFS_RAW): $(DATA) $(EXTRA_T3X) $(SYSFONT_EMBED_CIAS)
	@mkdir -p $(BUILD)
	$(MKROMFS) $(DATA) $@

# BUGFIX v1.1: o .cia agora deixa o PROPRIO makerom construir a RomFS a partir
# da pasta romfs/ (via "RomFs: RootPath" no RSF) -- o metodo padrao/correto.
# O wrapper IVFC artesanal (mk_ivfc_romfs.py) gerava uma RomFS que o makerom
# aceitava mas o 3DS nao conseguia ler -> icones/fontes sumiam SO no .cia (o
# .3dsx usa o romfs cru direto e sempre funcionou). O .t3x tem que existir
# antes do makerom varrer a pasta, dai a dependencia em EXTRA_T3X.
$(CIA): $(ELF) $(SMDH) $(APP_RSF) $(BANNER) $(EXTRA_T3X) $(SYSFONT_EMBED_CIAS)
	$(CCPREFIX)strip $< -o $(BUILD)/CustomizerDS_stripped.elf
	$(MAKEROM) -f cia -o $@ -rsf $(APP_RSF) -target t -exefslogo -elf $(BUILD)/CustomizerDS_stripped.elf -icon $(SMDH) -banner $(BANNER)

PREVIEW_3DSX := $(BUILD)/$(TARGET)_preview.3dsx

preview3dsx: $(PREVIEW_3DSX)

$(PREVIEW_3DSX): $(ELF) $(SMDH) $(ROMFS_RAW)
	$(3DSXTOOL) $< $@ --smdh=$(SMDH) --romfs=$(ROMFS_RAW)

# .cia de fonte de sistema embutidos -- gerados pelo `all` (prereq de ROMFS_RAW/
# CIA); `make sysfont` regenera so eles. Custom = MERGE na STOCK; STOCK = do .lz.
sysfont: $(SYSFONT_EMBED_CIAS)
	@echo "Fontes de sistema (merge) em $(SYSFONT_DIR)/ (STOCK = fonte original do console)."

# STOCK descomprimida = input do merge (a partir do .lz original).
$(SYSFONT_STOCK_BCFNT): $(SYSFONT_STOCK_LZ)
	$(THREEDSTOOL) -uvf $< --compress-type lzex --compress-out $@

# Regra de MERGE por fonte: STOCK + TTF/OTF -> .bcfnt merge (glyph set completo,
# Latin re-estilizado) -> .cia. mk_sysfont_merge.py precisa de freetype ($(PYFT)).
define SYSFONT_MERGE_RULE
$(SYSFONT_DIR)/SystemFont_$(1).cia: $(wildcard $(SYSFONT_TTF_DIR)/$(1).*) $(SYSFONT_STOCK_BCFNT) $(SYSFONT_HDR) scripts/mk_sysfont_merge.py scripts/mk_sysfont_cia.py
	@mkdir -p $(SYSFONT_DIR) $(BUILD)/mergedfonts
	$(PYFT) scripts/mk_sysfont_merge.py $(SYSFONT_STOCK_BCFNT) $$(firstword $$(wildcard $(SYSFONT_TTF_DIR)/$(1).*)) $(BUILD)/mergedfonts/$(1).bcfnt
	python3 scripts/mk_sysfont_cia.py $(BUILD)/mergedfonts/$(1).bcfnt $$@ --header $(SYSFONT_HDR) --3dstool $(THREEDSTOOL) --makerom $(MAKEROM)
endef
$(foreach f,$(SYSFONT_FONTS),$(eval $(call SYSFONT_MERGE_RULE,$(f))))

# Fonte STOCK -> .cia direto do .lz original (fiel, sem recomprimir, --lz).
$(SYSFONT_DIR)/SystemFont_STOCK.cia: $(SYSFONT_STOCK_LZ) $(SYSFONT_HDR) scripts/mk_sysfont_cia.py
	@mkdir -p $(SYSFONT_DIR)
	python3 scripts/mk_sysfont_cia.py $< $@ --lz --header $(SYSFONT_HDR) --3dstool $(THREEDSTOOL) --makerom $(MAKEROM)

clean:
	rm -rf $(BUILD)

# Remove tambem os .cia de fonte embutidos (artefatos em romfs/).
clean-sysfont:
	rm -rf $(SYSFONT_DIR)

.PHONY: all clean clean-sysfont preview3dsx sysfont
