inst_LIBRARIES = libcovered.a
instdir = @libdir@/covered
libcovered_a_SOURCES = ../../src/arc.c \
                       ../../src/binding.c \
                       ../../src/db.c \
                       ../../src/expr.c \
                       ../../src/fsm.c \
                       ../../src/func_unit.c \
                       ../../src/genprof.c \
                       ../../src/globals.c \
                       ../../src/info.c \
                       ../../src/instance.c \
                       ../../src/link.c \
                       ../../src/obfuscate.c \
                       ../../src/profiler.c \
                       ../../src/race.c \
                       ../../src/scope.c \
                       ../../src/statement.c \
                       ../../src/static.c \
                       ../../src/tree.c \
                       ../../src/util.c \
                       ../../src/vector.c \
                       ../../src/vsignal.c

INCLUDES=	-I..
AM_CPPFLAGS=-DRUNLIB -DINSTALL_DIR=\"@datadir@/covered\" $(DEBUGDEF) $(PROFILEDEF) @PICFLAG@

