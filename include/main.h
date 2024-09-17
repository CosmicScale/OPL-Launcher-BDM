#ifndef __MAIN_H
#define __MAIN_H
#include <kernel.h>
#include <gsKit.h>
#include <stdint.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <string.h>
#include <loadfile.h>
#include <stdio.h>
#include <sbv_patches.h>
#include <elf-loader.h>
#include <dirent.h>
#include <tamtypes.h>
#include <debug.h>
#include <unistd.h>
#include <stdlib.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

extern unsigned char ps2dev9_irx[];
extern unsigned int size_ps2dev9_irx;

extern unsigned char ps2atad_irx[];
extern unsigned int size_ps2atad_irx;

extern unsigned char ps2hdd_irx[];
extern unsigned int size_ps2hdd_irx;

extern unsigned char iomanx_irx[];
extern unsigned int size_iomanx_irx;

extern unsigned char filexio_irx[];
extern unsigned int size_filexio_irx;

extern unsigned char ps2fs_irx[];
extern unsigned int size_ps2fs_irx;

#endif