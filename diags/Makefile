####################################################################
# Written by:  Trevor Williams  (trevorw@charter.net)
#
# Date:        3/22/2002
#
# Purpose:     Makefile for running Covered regression suite.  See
#	       README for details.
####################################################################

CDIR = ./c
VDIR = ./regress

regress:	regress-c regress-v

regress-c:
	cd $(CDIR);  make regress
	cp $(CDIR)/regress.log c-regress.log

regress-v:
	cd $(VDIR);  make
	cp $(VDIR)/regress.log v-regress.log

distclean:  clean

clean:	clean-c clean-v
	rm -f *.log
	
clean-c:
	cd $(CDIR);  make clean

clean-v:
	cd $(VDIR);  make clean
	
