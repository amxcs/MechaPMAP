# MechaPMAP

PS2 homebrew that calibrates the disc drive through the MechaCon's own servo adjustment commands:
measure CD / DVD-SL / DVD-DL, edit the servo values, check the laser, and write the EEPROM.
It backs up the NVRAM to USB first and verifies every write.

| MechaCon | Console | Tested on hardware |
|---|---|---|
| 6.10 | SCPH-77004 | yes |
| 6.12 | SCPH-90004 | yes |
| 6.06 | SCPH-75004 | yes |
| 6.00, 6.02, 6.04 | SCPH-7000x / 7500x | no |
| 5.00, 5.02, 5.04, 5.06, 5.06 Mexico, 5.12 | SCPH-5000x | no |

Run `MechaPMAP.elf` from a USB stick and keep the backup it writes there.

Named after SP193's PMAP. Based on [MechaPwn](https://github.com/MechaResearch/MechaPwn). GPL-3.0. Use at your own risk.
