PROJDIRS:=./src
CFILES:=$(shell find $(PROJDIRS) -type f -name "*.c")
ASMFILES:=$(shell find $(PROJDIRS) -type f -name "*.s")
SRCFILES:= $(CFILES) $(ASMFILES)
OBJFILES:= $(CFILES:.c=.o) $(ASMFILES:.s=.s.o)

WARNINGS:= -Werror -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes -Wno-sign-conversion -Wno-unused-parameter -Wno-conversion
CFLAGS:=-nostartfiles -std=gnu99 -gdwarf-2 -g3 $(WARNINGS) -DKERNEL_DEBUGGING=1 -I include -D_STEWIEOS -D__KERNEL__
LDFLAGS:=-Tstewieos.ld -lgcc -nostartfiles

KERNEL_VERSION:=0.0.1
KERNEL_ID:=stewieos
KERNEL_NAME:=$(KERNEL_ID)-kernel-$(KERNEL_VERSION)

export CC=$(TARGET)-gcc
export LD=$(TARGET)-ld
#export ASM=$(TARGET)-as
export ASM=nasm
export KERNEL_DIR=$(abspath .)

.PHONY: all install clean libs-all libs-clean

all: libs-all $(KERNEL_NAME)

install:
	cp ./$(KERNEL_NAME) "$(DESTDIR)/boot"

clean: libs-clean
	rm -f $(OBJFILES)

libs-all:
	$(MAKE) -C ./libs all

libs-clean:
	$(MAKE) -C ./libs clean
	

$(KERNEL_NAME): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(KERNEL_NAME) $(OBJFILES:./src/start.s.o=) $(wildcard ./libs/lib*.a) $(LDFLAGS)

%.s.o: %.s
	$(ASM) -f elf32 -g $(@:.s.o=.s) -o $@
