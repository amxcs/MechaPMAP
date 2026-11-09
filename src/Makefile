EE_BIN = MechaPMAP_debug.elf
EE_OBJS = main.o pad.o mecha.o freesio2_irx.o freepad_irx.o iomanX_irx.o fileXio_irx.o usbd_irx.o usbhdfsd_irx.o mechaproxy_irx.o pmapproxy_irx.o font_disp.o font_body.o font_bold.o font_mono.o pwr70k_rgb.o
EE_INCS = -I$(PS2SDK)/ports/include -I$(PS2SDK)/ports/include/freetype2 -I$(GSKIT)/include -I$(PS2SDK)/sbv/include
EE_CFLAGS = -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers -Werror
EE_LDFLAGS = -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib -L$(PS2SDK)/sbv/lib
EE_LIBS = -lcdvd -lpatches -lpadx -lfreetype -lpng -lz -lgskit_toolkit -lgskit -ldmakit -lm -ldebug
all: MechaPMAP.elf
%_irx.c: $(PS2SDK)/iop/irx/%.irx
	bin2c $< $@ $*_irx
mechaproxy_irx.c: ../assets/mechaproxy.irx
	bin2c $< $@ mechaproxy_irx
pmapproxy_irx.c: ../iop/irx/pmapproxy.irx
	bin2c $< $@ pmapproxy_irx
font_disp.c: ../fonts/ChakraPetch-SemiBold.ttf
	bin2c $< $@ font_disp
font_body.c: ../fonts/IBMPlexSans-Medium.ttf
	bin2c $< $@ font_body
font_bold.c: ../fonts/IBMPlexSans-SemiBold.ttf
	bin2c $< $@ font_bold
font_mono.c: ../fonts/IBMPlexMono-Medium.ttf
	bin2c $< $@ font_mono
pwr70k_rgb.c: ../assets/pwr70k.rgb
	bin2c $< $@ pwr70k_rgb
MechaPMAP.elf: $(EE_BIN)
	$(EE_STRIP) -o $@ $<
main.o: *.h
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
