PROJDIRS:=./src
CFILES:=$(shell find $(PROJDIRS) -type f -name "*.c")
ASMFILES:=$(shell find $(PROJDIRS) -type f -name "*.s")
SRCFILES:= $(CFILES) $(ASMFILES)
OBJFILES:= $(CFILES:.c=.o) $(ASMFILES:.s=.s.o)

CFLAGS:=-nostdlib -std=c99 -ggdb
LDFLAGS:=-Tstewieos.ld

CC:=i586-pc-codeos-gcc
LD:=i586-pc-codeos-ld
ASM:=nasm

KERNEL_VERSION:=0.0.1
KERNEL_ID:=stewieos
KERNEL_NAME:=$(KERNEL_ID)-kernel-$(KERNEL_VERSION)

.PHONY: all install

all: $(KERNEL_NAME)

install:
	losetup /dev/loop0 ../stewieos.vhd
	kpartx -v -a /dev/loop0
	mount /dev/mapper/loop0p1 /mnt
	cp ./$(KERNEL_NAME) /mnt/boot/
	umount /mnt
	kpartx -v -d /dev/loop0
	losetup -d /dev/loop0

$(KERNEL_NAME): $(OBJFILES)
	$(LD) -o $(KERNEL_NAME) $(OBJFILES) $(LDFLAGS)

%.s.o: %.s
	$(ASM) -felf $(@:.s.o=.s) -o $@