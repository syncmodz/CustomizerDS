TARGET := CustomizerDS
BUILD := build
SOURCES := source
DATA := romfs

export DEVKITPRO ?= /opt/devkitpro
DEVKITARM := $(DEVKITPRO)/devkitARM

BANNERTOOL := /home/chicharito/bin/bannertool
MAKEROM := /home/chicharito/bin/makerom
3DSXTOOL := /opt/devkitpro/tools/bin/3dsxtool
MKROMFS := /opt/devkitpro/tools/bin/mkromfs3ds

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

all: $(3DSX) $(CIA)

$(3DSX): $(ELF) $(SMDH) $(ROMFS_BIN)
	$(3DSXTOOL) $< $@ --smdh=$(SMDH) --romfs=$(ROMFS_BIN)

$(ELF): $(BUILD)/main.o $(BUILD)/common.o $(BUILD)/menu.o $(BUILD)/fonts.o $(BUILD)/darkmode.o $(BUILD)/led.o $(BUILD)/theme.o $(BUILD)/anim.o $(BUILD)/ui.o $(BUILD)/input.o $(BUILD)/color_picker.o $(BUILD)/config.o $(BUILD)/icons.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BUILD)/%.o: $(SOURCES)/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(SMDH): $(APP_ICON)
	$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "Personalize seu Nintendo 3DS" -p "$(APP_AUTHOR)" -i $(APP_ICON) -o $@

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

$(ROMFS_BIN): $(DATA)
	@mkdir -p $(BUILD)
	$(MKROMFS) $(DATA) $(BUILD)/romfs_raw.bin
	python3 scripts/mk_ivfc_romfs.py $(BUILD)/romfs_raw.bin $@

$(CIA): $(ELF) $(SMDH) $(APP_RSF) $(BANNER) $(ROMFS_BIN)
	$(CCPREFIX)strip $< -o $(BUILD)/CustomizerDS_stripped.elf
	$(MAKEROM) -f cia -o $@ -rsf $(APP_RSF) -target t -exefslogo -elf $(BUILD)/CustomizerDS_stripped.elf -icon $(SMDH) -banner $(BANNER) -romfs $(ROMFS_BIN)

PREVIEW_3DSX := $(BUILD)/$(TARGET)_preview.3dsx

preview3dsx: $(PREVIEW_3DSX)

$(PREVIEW_3DSX): $(ELF) $(SMDH) $(ROMFS_BIN)
	$(3DSXTOOL) $< $@ --smdh=$(SMDH) --romfs=$(ROMFS_BIN)

clean:
	rm -rf $(BUILD)

.PHONY: all clean preview3dsx
