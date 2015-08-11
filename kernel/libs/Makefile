PROJS:=acpica
ALL_PROJ:=$(PROJS:%=all-%)
CLEAN_PROJ:=$(PROJS:%=clean-%)
LIBS:=$(PROJS:%=$(BINDIR)/lib%.a)


.PHONY: all clean

all: $(ALL_PROJ)

clean: $(CLEAN_PROJ)

$(ALL_PROJ):
	$(MAKE) -C $(@:all-%=%) all

$(CLEAN_PROJ):
	$(MAKE) -C $(@:clean-%=%) clean