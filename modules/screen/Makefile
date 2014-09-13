PROJDIRS:=./src
CFILES:=$(shell find $(PROJDIRS) -type f -name "*.c")
ASMFILES:=$(shell find $(PROJDIRS) -type f -name "*.s")
SRCFILES:= $(CFILES) $(ASMFILES)
OBJFILES:= $(CFILES:.c=.o) $(ASMFILES:.s=.s.o)

WARNINGS:= -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes -Wno-sign-conversion
CFLAGS:=-std=gnu99 -nostdlib -nostartfiles -gdwarf-2 -g3 $(WARNINGS) -I$(KERNEL_DIR)/src
LDFLAGS:= -r -d -nostdlib -Wl,-R,$(STEWIEOS_CURRENT)

CC:=i586-pc-stewieos-gcc
LD:=i586-pc-stewieos-gcc
ASM:=nasm

PROJECT_NAME:=screen

OUTPUT_FILE=./bin/$(PROJECT_NAME).o

.PHONY: all clean install $(PROJECT_NAME)

all: $(PROJECT_NAME)

$(PROJECT_NAME): $(OUTPUT_FILE)

install:
	strip -S -o $(MODULE_INSTALL_DIR)/$(PROJECT_NAME).o $(OUTPUT_FILE)

clean:
	rm -f $(OBJFILES) $(OUTPUT_FILE)

$(OUTPUT_FILE): $(OBJFILES) $(STEWIEOS_CURRENT)
	$(CC) $(CFLAGS) -o $(OUTPUT_FILE) $(OBJFILES) $(LDFLAGS)

%.s.o: %.s
	$(ASM) -f elf32 -g $(@:.s.o=.s) -o $@
