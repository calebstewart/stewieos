PROJDIRS:=./src
CFILES:=$(shell find $(PROJDIRS) -type f -name "*.c")
ASMFILES:=$(shell find $(PROJDIRS) -type f -name "*.s")
SRCFILES:= $(CFILES) $(ASMFILES)
OBJFILES:= $(CFILES:.c=.o) $(ASMFILES:.s=.s.o)

WARNINGS:= -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes -Wno-sign-conversion
CFLAGS:=-std=gnu99 -nostdlib -nostartfiles -gdwarf-2 -g3 $(WARNINGS) -I../kernel/src
LDFLAGS:= -r -nostdlib

CC:=i586-pc-stewieos-gcc
LD:=i586-pc-stewieos-gcc
ASM:=nasm

PROJECT_NAME=./bin/test_mod.o

.PHONY: all clean

all: $(PROJECT_NAME)

clean:
	rm -f $(OBJFILES)

$(PROJECT_NAME): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(PROJECT_NAME) $(OBJFILES) $(LDFLAGS)

%.s.o: %.s
	$(ASM) -f elf32 -g $(@:.s.o=.s) -o $@
