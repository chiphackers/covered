######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs C regression suite.
######################################################

SRCDIR   = ../../src

SRCLIBS  = $(SRCDIR)/vector.c \
       	   $(SRCDIR)/util.c \
	   $(SRCDIR)/expr.c \
	   funcs.c

DIAGLIST = xor \
	   and \
	   or \
	   nand \
	   nxor \
	   nor \
	   add \
	   subtract \
	   mod \
	   multiply \
	   multiply_x \
	   multiply_z \
	   divide \
	   lshift \
	   rshift \
	   uinv \
	   uand \
	   uor \
	   uxor \
	   unand \
	   unor \
	   unxor

LOGFILE  = regress.log
CXX      = gcc
CFLAGS   = -I$(SRCDIR) -g

.SILENT:

regress:	clean $(DIAGLIST)
	grep PASS $(LOGFILE) | wc -l >> $(LOGFILE)
	echo out of >> $(LOGFILE)
	ls $(DIAGLIST) | wc -l >> $(LOGFILE)
	echo diagnostics passed >> $(LOGFILE)
	tail -4 $(LOGFILE)

$(DIAGLIST):	$(SRCLIBS)
	echo Running diagnostic $@ >> $(LOGFILE)
	$(CXX) $(CFLAGS) -o $@ $@.c $(SRCLIBS)
#	./$@ | tee -a $(LOGFILE)
	./$@ >> $(LOGFILE)

clean:
	rm -f $(DIAGLIST)
	rm -f *.log core
