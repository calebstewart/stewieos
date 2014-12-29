# This is where you can add project directories to the build-chain
PROJECTS:=INIT sh echo ls write_test cat touch mkdir shutdown syslogd mesg
ALLPROJECTS:=$(PROJECTS:%=all-%)
CLEANPROJECTS:=$(PROJECTS:%=clean-%)
INSTALLPROJECTS:=$(PROJECTS:%=install-%)

.PHONY: $(ALLPROJECTS) $(CLEANPROJECTS) $(INSTALLPROJECTS) all clean install

all: $(ALLPROJECTS)

clean: $(CLEANPROJECTS)

install: $(INSTALLPROJECTS)

$(ALLPROJECTS):
	$(MAKE) -C $(@:all-%=%) all

$(CLEANPROJECTS):
	$(MAKE) -C $(@:clean-%=%) clean
	
$(INSTALLPROJECTS):
	$(MAKE) -C $(@:install-%=%) install
