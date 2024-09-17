FRONTEND_OBJS = $(EE_SRC_DIR)main.o

EECORE_OBJS =	$(EE_C_DIR)ps2dev9.o $(EE_C_DIR)ps2atad.o $(EE_C_DIR)ps2fs.o $(EE_C_DIR)ps2hdd.o \
		$(EE_C_DIR)iomanx.o $(EE_C_DIR)filexio.o

EE_BIN = OPL-Launcher-BDM.elf

EE_SRC_DIR = src/
EE_C_DIR = modules/
EE_OBJS = $(FRONTEND_OBJS) $(EECORE_OBJS)

EE_CFLAGS := -O2 -mgpopt -Wno-stringop-truncation -I$(PS2DEV)/gsKit/include
EE_LDFLAGS += -L$(PS2SDK)/ee/lib -L$(PS2DEV)/gsKit/lib
EE_LIBS += -lfileXio -lpatches -lelf-loader-nocolour -L$(PS2SDK)/ports/lib -L$(PS2DEV)/gsKit/lib/ -ldmakit -lgskit -ldebug -lc
EE_INCS += -I$(PS2SDK)/ports/include -I$(PS2DEV)/gsKit/include
EE_INCS += -I./include

BIN2C = $(PS2SDK)/bin/bin2c

all: create_dirs $(EE_BIN)
	$(EE_STRIP) $(EE_BIN)

create_dirs:
	mkdir -p $(EE_C_DIR)

clean:
	rm -f -r $(EE_C_DIR) $(EE_BIN) $(EE_SRC_DIR)main.o

rebuild: clean all

$(EE_C_DIR)ps2dev9.c: $(PS2SDK)/iop/irx/ps2dev9.irx
	$(BIN2C) $< $@ ps2dev9_irx

$(EE_C_DIR)ps2atad.c: $(PS2SDK)/iop/irx/ps2atad.irx
	$(BIN2C) $< $@ ps2atad_irx

$(EE_C_DIR)ps2fs.c: $(PS2SDK)/iop/irx/ps2fs.irx
	$(BIN2C) $< $@ ps2fs_irx

$(EE_C_DIR)ps2hdd.c: $(PS2SDK)/iop/irx/ps2hdd-osd.irx
	$(BIN2C) $< $@ ps2hdd_irx

$(EE_C_DIR)iomanx.c: $(PS2SDK)/iop/irx/iomanX.irx
	$(BIN2C) $< $@ iomanx_irx

$(EE_C_DIR)filexio.c: $(PS2SDK)/iop/irx/fileXio.irx
	$(BIN2C) $< $@ filexio_irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal_cpp
