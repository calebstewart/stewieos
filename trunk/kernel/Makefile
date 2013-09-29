PROJDIRS:=./src
CFILES:=$(shell find $(PROJDIRS) -type f -name "*.c")
ASMFILES:=$(shell find $(PROJDIRS) -type f -name "*.s")
SRCFILES:= $(CFILES) $(ASMFILES)
OBJFILES:= $(CFILES:.c=.o) $(ASMFILES:.s=.s.o)

WARNINGS:=-Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes
CFLAGS:=-nostdlib -std=gnu99 -gdwarf-2 -g3 $(WARNINGS)
LDFLAGS:=-Tstewieos.ld -lgcc

CC:=i586-pc-codeos-gcc
LD:=i586-pc-codeos-gcc
ASM:=nasm

KERNEL_VERSION:=0.0.1
KERNEL_ID:=stewieos
KERNEL_NAME:=$(KERNEL_ID)-kernel-$(KERNEL_VERSION)

.PHONY: all install clean

all: $(KERNEL_NAME)

install:
	cp ./$(KERNEL_NAME) /mnt/boot/

clean:
	rm -f $(OBJFILES)

$(KERNEL_NAME): $(OBJFILES)
	$(LD) -o $(KERNEL_NAME) $(OBJFILES:./src/start.s.o=) $(LDFLAGS)

%.s.o: %.s
	$(ASM) -f elf32 -g $(@:.s.o=.s) -o $@
