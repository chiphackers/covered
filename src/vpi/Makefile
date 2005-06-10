CC     = gcc
CFLAGS = -I.. -I../.. -I/usr/X11R6/include -g -O2 -DHAVE_CONFIG_H -DVPI_ONLY
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

cver:	covered.so
	cver +loadvpi=./covered.so:vpi_compat_bootstrap covered_top.v foobar.v

covered.so: vpi_cver.o $(OBJS)
	ld -G -shared -export-dynamic vpi_cver.o $(OBJS) -o covered.so

vpi_cver.o: vpi.c
	gcc -fPIC -g -DCVER -I/home/cheetah4/trevorw/include/cver -I.. -c -o vpi_cver.o vpi.c
	
vcs:	simv
	./simv

simv:	covered_vcs.so vpi.c covered.tab covered_top.v foobar.v
	vcs +vpi -load /home/cheetah4/trevorw/vpi/covered_vcs.so:covered_register -P covered.tab covered_top.v foobar.v

covered_vcs.so: vpi_vcs.o
	ld -G -shared -export-dynamic vpi_vcs.o -o covered_vcs.so

vpi_vcs.o: vpi.c
	gcc -fPIC -g -DVCS -I$(VCS_HOME)/include -I.. -c -o vpi_vcs.o vpi.c

clean:
	rm -f *.so *.o *.log
	rm -rf csrc simv*
	rm -rf core
