# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
#-----------------------------------------------------------------------
# PS2DEV binary IRX target rules.

IOP_BIN_DIR ?= irx/
IOP_BIN ?= $(shell basename $(CURDIR)).irx
IOP_BIN := $(IOP_BIN:%=$(IOP_BIN_DIR)%)

all:: $(IOP_BIN)

clean::
	rm -f -r $(IOP_OBJS_DIR) $(IOP_BIN_DIR)
