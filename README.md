# OPL-Launcher-BDM

Designed to be used with the [PSBBN Definitive English Patch 2.0](https://github.com/CosmicScale/PSBBN-Definitive-English-Patch)

#### For PS2 games:
Reads `hdd0:/__common/OPL/conf_hdd.cfg` to launch `$OPL/OPNPS2LD.ELF` and uses OPL's `autoLaunchBDMGame` function to load PS2 games from a block device.

#### For PS1 games:
Loads a corresponding `POPStarter.elf` from the `__.POPS` partition.

Supports Game ID for the Pixel FX Retro GEM for both PS2 and PS1 games.

**Note:**
Requires OPL v1.2.0-Beta-2197-d5e0998 or later, which can be downloaded [here](https://github.com/ps2homebrew/Open-PS2-Loader/releases/tag/latest)

## Usage:
Create a file named `launcher.cfg` and place the file in the same location as `OPL-Launcher-BDM.elf` or `OPL-Launcher-BDM.KELF`.

Example file contents for PS2 games:
```
file_name=Ecco the Dolphin.iso
title_id=SLUS_203.94
disc_type=DVD
```

Example file contents for PS1 games:
```
file_name=Ape Escape.VCD
title_id=SCUS_944.23
disc_type=POPS
```

All PS2 games will be launched from the exFAT partition on the internal drive.

For PS1 games, place the .VCD files on the internal drive in the `__.POPS` partition along with a renamed `POPStarter.elf` matching each .VCD file.

You can create "OPL Launcher" partitions on the PS2 drive for compatibility with HDDOSD and PSBBN. To do this:
1. Create a 128 MB partition.
2. Inject `system.cnf`, `icon.sys` and `list.ico` into the partition header.

Example `system.cnf` file:

```ini
BOOT2 = pfs:/OPL-Launcher-BDM.KELF
VER = 1.00
VMODE = NTSC
HDDUNITPOWER = NICHDD
```

Prepare a signed executable (for example, by using [this app](https://www.psx-place.com/resources/kelftool-fmcb-compatible-fork.1104/))

```cmd
kelftool encrypt mbr OPL-Launcher-BDM.elf OPL-Launcher-BDM.KELF
```

Place `OPL-Launcher-BDM.KELF` and `launcher.cfg` files in the root of the partition.

## Credits:
- Written by [CosmicScale](https://github.com/CosmicScale)
- Includes some modified code from [OPL-Launcher](https://github.com/ps2homebrew/OPL-Launcher)
- Uses GameID code based on the [AppCakeLtd](https://github.com/AppCakeLtd) [gameid-with-gem](https://github.com/AppCakeLtd/Open-PS2-Loader/tree/gameid-with-gem) fork of Open PS2 Loader.
