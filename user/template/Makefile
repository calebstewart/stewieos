SOURCES:=src/main.c
OBJECTS:=$(SOURCES:.c=.o)
CFLAGS:=-g -static -std=gnu11
LDFLAGS:=
PROJECT_NAME:=template

.PHONY: all install clean

all: bin/$(PROJECT_NAME)

bin/$(PROJECT_NAME): $(OBJECTS)
	@mkdir -p ./bin
	$(CC) -o bin/$(PROJECT_NAME) $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS)
	rm -f bin/$(PROJECT_NAME)

install:
	strip -o $(STEWIEOS_ROOT)/bin/$(PROJECT_NAME) -s bin/$(PROJECT_NAME)
#	cp -f bin/$(PROJECT_NAME) $(STEWIEOS_ROOT)/bin