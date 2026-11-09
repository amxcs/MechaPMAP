# Needs ps2dev (PS2SDK, GSKIT). Output: src/MechaPMAP.elf
all:
	$(MAKE) -C iop
	$(MAKE) -C src
clean:
	$(MAKE) -C iop clean
	$(MAKE) -C src clean
	rm -f src/*_irx.c src/font_*.c src/pwr70k_rgb.c
