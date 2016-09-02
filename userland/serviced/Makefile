SOURCES:=src/main.c src/svc.c src/mod.c
OBJECTS:=$(SOURCES:.c=.o)
CFLAGS:=-g -static
LDFLAGS:= -lc -ljsonparser
PROJECT_NAME:=serviced

.PHONY: all install clean

all: bin/$(PROJECT_NAME)

bin/$(PROJECT_NAME): $(OBJECTS)
	@mkdir -p ./bin
	$(CC) -o bin/$(PROJECT_NAME) $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS)
	rm -f bin/$(PROJECT_NAME)

install:
	strip -o $(DESTDIR)/bin/$(PROJECT_NAME) -s bin/$(PROJECT_NAME)