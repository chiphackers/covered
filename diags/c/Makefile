######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs C regression suite.
######################################################

CULIDIR  = /home/trevorw/include

CULLDIR  = /home/trevorw/lib

CULLIB   = TestHarness

SRCDIR   = ../../src

SRCLIBS  = $(SRCDIR)/vector.c \
       	   $(SRCDIR)/util.c \
	   $(SRCDIR)/expr.c \
	   funcs.c

CDIAGLIST = xor \
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

CPPDIAGLIST = param

LOGFILE  = regress.log
CXX      = g++
CFLAGS   = -I$(SRCDIR) -I$(CULIDIR) -g -L$(CULLDIR) -l$(CULLIB)

.SILENT:

regress:	clean $(CDIAGLIST) $(CPPDIAGLIST)
	grep PASS $(LOGFILE) | wc -l >> $(LOGFILE)
	echo out of >> $(LOGFILE)
	ls $(CDIAGLIST) $(CPPDIAGLIST) | wc -l >> $(LOGFILE)
	echo diagnostics passed >> $(LOGFILE)
	tail -4 $(LOGFILE)

$(CDIAGLIST):	$(SRCLIBS)
	echo Running diagnostic $@ >> $(LOGFILE)
	$(CXX) $(CFLAGS) -o $@ $@.c $(SRCLIBS)
#	./$@ | tee -a $(LOGFILE)
	./$@ >> $(LOGFILE)

$(CPPDIAGLIST):	$(SRCLIBS)
	echo Running diagnostic $@ >> $(LOGFILE)
	$(CXX) $(CFLAGS) -o $@ $@.cc $(SRCLIBS)
#	./$@ | tee -a $(LOGFILE)
	./$@ >> $(LOGFILE)

clean:
	rm -f $(CDIAGLIST)
	rm -f *.log core
