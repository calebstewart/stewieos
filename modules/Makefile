#====================================================================
#
# Author: Caleb Stewart
# Date: 05 June 2014
# Prupose: A top-level Makefile to build the modules for the kernel
#
# ===================================================================
# = Change Log                                                      |
# ===================================================================
# =    Date    |    Programmer   |    Description                   |
# ===================================================================
# = 05JUN2014  | Caleb Stewart   | Initial Creation                 |
# =            |                 |                                  |
#====================================================================


# This is where you can add modules to the build list
# just add the directory name underwhich they reside
# and make sure there is makefile within that has
# all, clean and install targets.
PROJECTS:=vtty ext2fs
ALLPROJECTS:=$(PROJECTS:%=all-%)
CLEANPROJECTS:=$(PROJECTS:%=clean-%)
INSTALLPROJECTS:=$(PROJECTS:%=install-%)

.PHONY: $(ALLPROJECTS) $(CLEANPROJECTS) $(INSTALLPROJECTS) all clean test install

export MODULE_INSTALL_DIR:=/mnt/bin

all: $(ALLPROJECTS)

clean: $(CLEANPROJECTS)

install: $(INSTALLPROJECTS)

$(ALLPROJECTS):
	$(MAKE) -C $(@:all-%=%) all

$(CLEANPROJECTS):
	$(MAKE) -C $(@:clean-%=%) clean
	
$(INSTALLPROJECTS):
	$(MAKE) -C $(@:install-%=%) install
