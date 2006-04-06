libexec_PROGRAMS = $(IVLIB) $(VCSLIB) $(CVERLIB)
EXTRA_PROGRAMS = covered.vpi covered.vcs.so covered.cver.so

noinst_LIBRARIES = libcovered.a
libcovered_a_SOURCES = ../src/arc.c \
                       ../src/assertion.c \
         	       ../src/attr.c \
                       ../src/binding.c \
                       ../src/codegen.c \
                       ../src/db.c \
                       ../src/expr.c \
                       ../src/fsm.c \
                       ../src/fsm_arg.c \
                       ../src/fsm_var.c \
                       ../src/link.c \
                       ../src/info.c \
                       ../src/instance.c \
                       ../src/iter.c \
                       ../src/param.c \
                       ../src/func_unit.c \
                       ../src/race.c \
                       ../src/scope.c \
                       ../src/sim.c \
                       ../src/stat.c \
                       ../src/statement.c \
                       ../src/static.c \
                       ../src/stmt_blk.c \
                       ../src/symtable.c \
                       ../src/tree.c \
                       ../src/util.c \
                       ../src/vector.c \
                       ../src/vsignal.c

INCLUDES=	-I..
AM_CPPFLAGS=-DVPI_ONLY -DINSTALL_DIR=\"@datadir@/covered\" $(TCLTK_CPPFLAGS)

covered.vcs.so: vpi.vcs.o libcovered.a
	ld -G -shared -export-dynamic vpi.vcs.o -L. -lcovered -o covered.so

covered.cver.so: vpi.cver.o libcovered.a
	ld -G -shared -export-dynamic vpi.cver.o -L. -lcovered -o covered.so

vpi.vcs.o: ../src/vpi.c
	gcc -fPIC -g -DVCS -I$(VCS_INC) -I.. -c -o vpi.vcs.o ../src/vpi.c
	rm -f vpi.vcs.o

vpi.cver.o: ../src/vpi.c
	gcc -fPIC -g -DCVER -I$(CVER_INC) -I.. -c -o vpi.cver.o ../src/vpi.c
	rm -f vpi.cver.o

covered.vpi: $(libcovered_a_OBJECTS) ../src/vpi.c
	iverilog-vpi --name=covered $(libcovered_a_OBJECTS) ../src/vpi.c
	rm -f vpi.o

CLEANFILES = covered.vcs.so covered.cver.so covered.vpi
