#
# $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/Makefile,v $
# $Author: fredk $
# $Date: 2007/11/15 21:44:15 $
# $Revision: 1.6 $
#
# Author: Fred Kuhns
#         fredk@arl.wustl.edu
# Organization: Applied Research Laboratory
#               Washington UNiversity in St. Louis
#

include ./Makefile.inc

SUBDIRS = wulib wuPP cmd
.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@echo "$(CURDIR): $(MyID) building on host $(MyHost), OS type is $(MyOS)"
	$(MAKE) -C $@;\

.PHONY: tags
tags :
	ctags -f tags -R ${WUSRC}/wulib ${WUSRC}/wuPP

all :
	@echo "$(CURDIR): $(MyID) building on host $(MyHost), OS type is $(MyOS)"
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

ixp :
	@echo "$(CURDIR): $(MyID) building on host $(MyHost), OS type is $(MyOS)"
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

ixp-clean :
	@echo "$(CURDIR): $(MyID) building on host $(MyHost), OS type is $(MyOS)"
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

install :
	@echo "$(CURDIR): $(MyID) building on host $(MyHost), OS type is $(MyOS)"
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

clean :
	@echo "$(CURDIR): $(MyID) cleaning on host $(MyHost), OS type is $(MyOS)"
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

realclean :
	@echo "$(CURDIR): $(MyID) cleaning on host $(MyHost), OS type is $(MyOS)"
	/bin/rm -f *~
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

depend :
	@echo "$(CURDIR): $(MyID) cleaning on host $(MyHost), OS type is $(MyOS)"
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

% :
	$(MAKE) -C $% $@; \
