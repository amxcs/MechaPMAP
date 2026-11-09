# MechaPMAP

PS2 homebrew that runs Sony's PMAP servo/laser calibration on the console through the MechaCon:
measure CD / DVD-SL / DVD-DL, edit the servo values, check the laser, and write the EEPROM only when asked.
It backs up the NVRAM to USB first and verifies every write.

| MechaCon | Console | Tested on hardware |
|---|---|---|
| 6.10 | SCPH-77004 | yes |
| 6.12 | SCPH-90004 | yes |
| 6.06 | SCPH-75004 | yes |
| 6.00, 6.02, 6.04 | SCPH-7000x / 7500x | no |
| 5.00, 5.02, 5.04, 5.06, 5.06 Mexico, 5.12 | SCPH-5000x | no |

Run `MechaPMAP.elf` from a USB stick and keep the backup it writes there.

Build: `make` (ps2dev). The MechaCon code is in `mechacon/`; its encryption key is not included.

Based on [MechaPwn](https://github.com/MechaResearch/MechaPwn). GPL-3.0. Use at your own risk.
