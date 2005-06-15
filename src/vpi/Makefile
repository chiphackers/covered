#!/sw/dvtools/gnu/bin/make

ifdef VCS
VPI_FLAGS     = -DVCS -I$(VCS_HOME)/include
LAST_COMPFILE = tmp.vcs
RUNLINE       = vcs +vpi -load /home/cheetah4/trevorw/covered/src/vpi/covered.so:covered_register -P covered.tab covered_top.v foobar.v; ./simv -l run.log +covered_outdb=out.cdd
else
VPI_FLAGS     = -DCVER -I/home/cheetah4/trevorw/include/cver
LAST_COMPFILE = tmp.cver
RUNLINE       = cver +loadvpi=./covered.so:vpi_compat_bootstrap covered_top.v foobar.v
endif

CC     = gcc
CFLAGS = -I.. -I../.. -I/usr/X11R6/include $(VPI_FLAGS) -g -O2 -DHAVE_CONFIG_H -DVPI_ONLY
OBJS   = arc.o \
         attr.o \
         binding.o \
         codegen.o \
         db.o \
         expr.o \
         fsm.o \
         fsm_arg.o \
         fsm_var.o \
         link.o \
         info.o \
         instance.o \
         iter.o \
         param.o \
         module.o \
         race.o \
         sim.o \
         stat.o \
         statement.o \
         static.o \
         symtable.o \
         util.o \
         vector.o \
         vsignal.o

all:	clean covered.so
	$(RUNLINE)

covered.so: vpi.o $(OBJS)
	ld -G -shared -export-dynamic vpi.o $(OBJS) -o covered.so

vpi.o: vpi.c
	gcc -fPIC -g $(VPI_FLAGS) -I.. -c -o vpi.o vpi.c
	
clean:
	if [ ! -f "$(LAST_COMPFILE)" ] ; then \
		rm -f *.so *.o *.log; \
		rm -rf csrc simv*; \
		rm -rf core; \
		rm -rf tmp.*; \
		touch $(LAST_COMPFILE); \
		echo "Cleaned house"; \
	fi

clobber:
	rm -f tmp.*
	$(MAKE) clean
