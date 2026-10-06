# Ndless build of Ashen Depths for the TI-Nspire CX / CX CAS (Gen 1).
# Run inside the Ndless SDK environment (tools/build_nspire.sh uses Docker).
GCC     = nspire-gcc
LD      = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -std=gnu99 -Wall -W -Wno-unused-parameter -marm -O2 -DNDEBUG -ffunction-sections -fdata-sections
LDFLAGS  = -Wl,--gc-sections -lm
ZEHNFLAGS = --name "Ashen Depths" --author "Ashen Depths project" --notice "Original idle ARPG for the TI-Nspire CX"

ROOT    = ../..
SRCS    = $(shell find $(ROOT)/src -name '*.c') main_nspire.c
OBJDIR  = obj
OBJS    = $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRCS)))
VPATH   = $(sort $(dir $(SRCS)))

all: AshenDepths.tns

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(GCC) $(GCCFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

AshenDepths.elf: $(OBJS)
	$(LD) $^ -o $@ $(LDFLAGS)

AshenDepths.tns: AshenDepths.elf
	$(GENZEHN) --input $< --output $@.zehn $(ZEHNFLAGS)
	make-prg $@.zehn $@
	rm $@.zehn

clean:
	rm -rf $(OBJDIR) *.elf *.tns *.zehn

.PHONY: all clean
