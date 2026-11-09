IOP_BIN = pmapproxy.irx
IOP_OBJS = pmapproxy.o imports.o exports.o
IOP_SRC_DIR = src/
IOP_OPTFLAGS = -O2
IOP_CFLAGS += -Wall -Wextra -Werror -Wno-unterminated-string-initialization
IOP_INCS += -I$(IOP_SRC_DIR)
include $(PS2SDK)/Defs.make
include Rules.bin.make
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
obj/pmapproxy.o: ../src/protocol.h src/irx_imports.h
