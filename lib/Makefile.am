libexec_PROGRAMS = $(IVLIB) $(VCSLIB) $(CVERLIB)

if BUILD_CYGWIN
EXTRA_PROGRAMS = covered.vpi covered.cver.dll
else
EXTRA_PROGRAMS = covered.vpi covered.vcs.so covered.cver.so
endif

noinst_LIBRARIES = libcovered.a
libcovered_a_SOURCES = ../src/arc.c \
                       ../src/assertion.c \
                       ../src/binding.c \
                       ../src/codegen.c \
                       ../src/db.c \
                       ../src/enumerate.c \
                       ../src/expr.c \
                       ../src/fsm.c \
                       ../src/func_iter.c \
                       ../src/func_unit.c \
                       ../src/genprof.c \
                       ../src/info.c \
                       ../src/instance.c \
                       ../src/iter.c \
                       ../src/link.c \
                       ../src/obfuscate.c \
                       ../src/ovl.c \
                       ../src/param.c \
                       ../src/parser_misc.c \
                       ../src/profiler.c \
                       ../src/race.c \
                       ../src/reentrant.c \
                       ../src/scope.c \
                       ../src/search.c \
                       ../src/sim.c \
                       ../src/stat.c \
                       ../src/statement.c \
                       ../src/static.c \
                       ../src/stmt_blk.c \
                       ../src/struct_union.c \
                       ../src/symtable.c \
                       ../src/tree.c \
                       ../src/util.c \
                       ../src/vector.c \
                       ../src/vsignal.c

INCLUDES=	-I..
AM_CPPFLAGS=-DVPI_ONLY -DINSTALL_DIR=\"@datadir@/covered\" $(TCLTK_CPPFLAGS) $(DEBUGDEF) $(PROFILEDEF) @PICFLAG@

if BUILD_CYGWIN
covered.cver.dll: vpi.cver.o libcovered.a
	$(CC) @shared@ -o covered.cver.dll -Wl,--out-implib=covered.cver.dll.a \
		-Wl,--export-all @rdynamic@ -Wl,--output-def=covered.cver.def \
		vpi.cver.o -L. -lcovered -lcver
else
covered.vcs.so: vpi.vcs.o libcovered.a
	$(CC) @shared@ @rdynamic@ vpi.vcs.o -L. -lcovered -o covered.vcs.so

covered.cver.so: vpi.cver.o libcovered.a
	$(CC) @shared@ @rdynamic@ vpi.cver.o -L. -lcovered -o covered.cver.so
endif

covered.vpi: vpi.iv.o libcovered.a
	$(CC) @shared@ vpi.iv.o -L. -lcovered -L$(IVDIR)/lib -lveriuser -lvpi -o covered.vpi

vpi.vcs.o: ../src/vpi.c
	$(CC) @PICFLAG@ -g -DVCS -DNOIV $(DEBUGDEF) $(PROFILEDEF) -I$(VCSINC) -I.. -c -o vpi.vcs.o ../src/vpi.c

vpi.cver.o: ../src/vpi.c
	$(CC) @PICFLAG@ -g -DCVER -DNOIV $(DEBUGDEF) $(PROFILEDEF) -I$(CVERINC) -I.. -c -o vpi.cver.o ../src/vpi.c

vpi.iv.o: ../src/vpi.c
	$(CC) @PICFLAG@ -g $(DEBUGDEF) $(PROFILEDEF) -I$(IVDIR)/include -I.. -c -o vpi.iv.o ../src/vpi.c

CLEANFILES = vpi.o vpi.vcs.o vpi.cver.o covered.vcs.so covered.cver.so covered.vpi
