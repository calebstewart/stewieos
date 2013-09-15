#====================================================================
#
# Author: Caleb Stewart
# Date: 10 September 2013
# Prupose: A top-level make file to build the OS, toolchain, initrd,
# :		etc...
# 
# ===================================================================
# = Change Log                                                      |
# ===================================================================
# =    Date    |    Programmer   |    Description                   |
# ===================================================================
# = 10SEP2013  | Caleb Stewart   | Initial Creation                 |
# =            |                 |                                  |
# ===================================================================
# = 15SEP2013  | Caleb Stewart   | Edited so you have to compile    |
# =            |                 | each project seperately          |
# ===================================================================
#
#====================================================================


# This is where you can add project directories to the source
PROJECTS:=kernel
ALLPROJECTS:=$(PROJECTS:%=all-%)
CLEANPROJECTS:=$(PROJECTS:%=clean-%)
INSTALLPROJECTS:=$(PROJECTS:%=install-%)

.PHONY: $(ALLPROJECTS) $(CLEANPROJECTS) $(INSTALLPROJECTS) all clean test install

all:
	@echo "Please build each subsystem one at time. Here is a list of subsystems to build:\n$(PROJECTS)\nE.g. \"make all-'subsystemname'\""

clean: $(CLEANPROJECTS)

install: $(INSTALLPROJECTS)

$(ALLPROJECTS):
	$(MAKE) -C $(@:all-%=%) all

$(CLEANPROJECTS):
	$(MAKE) -C $(@:clean-%=%) clean
	
$(INSTALLPROJECTS):
	$(MAKE) -C $(@:install-%=%) install

# Here, you can add dependencies for specific projects
# vhd: kernel