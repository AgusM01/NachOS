# Copyright (c) 1992      The Regents of the University of California.
#               2016-2021 Docentes de la Universidad Nacional de Rosario.
# All rights reserved.  See `copyright.h` for copyright notice and
# limitation of liability and disclaimer of warranty provisions.

MAKE = make
LPR  = lpr
SH   = bash

.PHONY: all clean

all:
	@echo ":: Making $$(tput bold)threads$$(tput sgr0)"
	@$(MAKE) -C threads depend
	@$(MAKE) -C threads all
	@echo ":: Making $$(tput bold)userprog$$(tput sgr0)"
	@$(MAKE) -C userprog depend
	@$(MAKE) -C userprog all
	@echo ":: Making $$(tput bold)vmem$$(tput sgr0)"
	@$(MAKE) -C vmem depend
	@$(MAKE) -C vmem all
	@echo ":: Making $$(tput bold)filesys$$(tput sgr0)"
	@$(MAKE) -C filesys depend
	@$(MAKE) -C filesys all
	@echo ":: Making $$(tput bold)bin$$(tput sgr0)"
	@$(MAKE) -C bin

clean:
	@echo ":: Cleaning everything"
	@$(MAKE) -C threads clean
	@$(MAKE) -C userprog clean
	@$(MAKE) -C vmem clean
	@$(MAKE) -C filesys clean
	@$(MAKE) -C bin clean
	@$(MAKE) -C userland clean
