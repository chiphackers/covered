####################################################################
# Written by:  Trevor Williams  (trevorw@charter.net)
#
# Date:        3/22/2002
#
# Purpose:     Makefile for running Covered regression suite.  See
#	       README for details.
####################################################################

VDIR = ./regress

all:

regress:	regress-v

regress-v:
	cd $(VDIR);  make
	cp $(VDIR)/regress.log v-regress.log

distclean:  clean

clean:	clean-v
	rm -f *.log
	
clean-v:
	cd $(VDIR);  make clean
	
