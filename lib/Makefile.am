libexec_PROGRAMS = $(IVLIB) $(VCSLIB) $(CVERLIB)
EXTRA_PROGRAMS = covered.vpi covered.vcs.so covered.cver.so

noinst_LIBRARIES = libcovered.a
libcovered_a_SOURCES = ../src/arc.c \
                       ../src/assertion.c \
         	       ../src/attr.c \
                       ../src/binding.c \
                       ../src/codegen.c \
                       ../src/db.c \
                       ../src/enumerate.c \
                       ../src/expr.c \
                       ../src/fsm.c \
                       ../src/fsm_arg.c \
                       ../src/fsm_var.c \
                       ../src/func_unit.c \
                       ../src/info.c \
                       ../src/instance.c \
                       ../src/iter.c \
                       ../src/link.c \
                       ../src/obfuscate.c \
                       ../src/ovl.c \
                       ../src/param.c \
                       ../src/race.c \
                       ../src/scope.c \
                       ../src/search.c \
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
AM_CPPFLAGS=-DVPI_ONLY -DINSTALL_DIR=\"@datadir@/covered\" $(TCLTK_CPPFLAGS) $(DEBUGDEF)

covered.vcs.so: vpi.vcs.o libcovered.a
	ld -G -shared -export-dynamic vpi.vcs.o -L. -lcovered -o covered.vcs.so

covered.cver.so: vpi.cver.o libcovered.a
	ld -G -shared -export-dynamic vpi.cver.o -L. -lcovered -o covered.cver.so

vpi.vcs.o: ../src/vpi.c
	gcc -fPIC -g -DVCS -DNOIV $(DEBUGDEF) -I$(VCSINC) -I.. -c -o vpi.vcs.o ../src/vpi.c

vpi.cver.o: ../src/vpi.c
	gcc -fPIC -g -DCVER -DNOIV $(DEBUGDEF) -I$(CVERINC) -I.. -c -o vpi.cver.o ../src/vpi.c

covered.vpi: $(libcovered_a_OBJECTS) ../src/vpi.c
	iverilog-vpi --name=covered $(libcovered_a_OBJECTS) ../src/vpi.c

CLEANFILES = vpi.o vpi.vcs.o vpi.cver.o covered.vcs.so covered.cver.so covered.vpi
