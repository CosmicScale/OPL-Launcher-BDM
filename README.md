# OPL-Launcher-BDM

OPL-Launcher-BDM reads `hdd0:/__common/OPL/conf_hdd.cfg` to launch `$OPL/OPNPS2LD.ELF` and uses OPL's `autoLaunchBDMGame` function.

**Note:**
This is currently a test app, as OPL's `autoLaunchBDMGame` function is not yet fully functional.

## Usage:
Create a `.cfg` file with a gameid and filename that matches the game you wish to launch, using the suffix `CD` or `DVD` depending on the game. For example:

To launch the game `Mister Mosquito.iso`:

- `SLUS_203.75.Mister Mosquito.iso-CD.cfg`

or to launch the game `Ecco the Dolphin.iso`:

- `SLUS_203.94.Ecco the Dolphin.iso-DVD.cfg`

Place the `.cfg` file in the same location as `OPL-Launcher-BDM.elf`. All games will be launched from `mass0`.

You can also create a "Launcher Partition" on the PS2 HDD for compatibility with HDDOSD and PSBBN. Create a 128 MB partition and inject `system.cnf` and `icon.sys` into the header of the partition.

Prepare a signed executable (for example, by using [this app](https://www.psx-place.com/resources/kelftool-fmcb-compatible-fork.1104/))

```cmd
        kelftool encrypt mbr OPL-Launcher-BDM.elf EXECUTE.KELF
```

Place `EXECUTE.KELF` and `.cfg` file in the root of the partition.

## Credits:
- Written by [CosmicScale](https://github.com/CosmicScale)
- Includes some modified code from [OPL-Launcher](https://github.com/ps2homebrew/OPL-Launcher)