# This is where you can add modules to the build list
# just add the directory name underwhich they reside
# and make sure there is makefile within that has
# all, clean and install targets.
PROJECTS:=vtty atkbd
ALLPROJECTS:=$(PROJECTS:%=all-%)
CLEANPROJECTS:=$(PROJECTS:%=clean-%)
INSTALLPROJECTS:=$(PROJECTS:%=install-%)

.PHONY: $(ALLPROJECTS) $(CLEANPROJECTS) $(INSTALLPROJECTS) all clean test install

export CC=$(TARGET)-gcc
export LD=$(TARGET)-ld
export KERNEL_DIR=$(abspath ../kernel)

all: $(ALLPROJECTS)

clean: $(CLEANPROJECTS)

install: $(INSTALLPROJECTS)

$(ALLPROJECTS):
	$(MAKE) -C $(@:all-%=%) all

$(CLEANPROJECTS):
	$(MAKE) -C $(@:clean-%=%) clean
	
$(INSTALLPROJECTS):
	mkdir -p $(DESTDIR)/lib/modules
	$(MAKE) -C $(@:install-%=%) DESTDIR=$(DESTDIR)/lib/modules install
