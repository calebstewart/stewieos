SOURCES:=src/main.c
OBJECTS:=$(SOURCES:.c=.o)
CFLAGS:=-g -static
LDFLAGS:=
PROJECT_NAME:=template

CC:=i586-pc-stewieos-gcc

.PHONY: all install clean

all: bin/$(PROJECT_NAME)

bin/$(PROJECT_NAME): $(OBJECTS)
	$(CC) -o bin/$(PROJECT_NAME) $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS)
	rm -f bin/$(PROJECT_NAME)

install:
	cp -f bin/$(PROJECT_NAME) $(STEWIEOS_ROOT)/bin
