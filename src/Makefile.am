bin_PROGRAMS = covered
covered_SOURCES = arc.c \
                  attr.c \
                  parser.c \
		  binding.c \
		  codegen.c \
		  comb.c \
		  db.c \
		  expr.c \
		  fsm.c \
                  fsm_arg.c \
		  fsm_var.c \
		  gui.c \
		  info.c \
		  instance.c \
 		  iter.c \
		  keywords.c \
                  pplexer.c \
		  lexer.c \
		  line.c \
                  link.c \
		  main.c \
		  merge.c \
		  module.c \
		  param.c \
		  parse.c \
		  parser_misc.c \
                  race.c \
		  report.c \
		  score.c \
		  search.c \
		  sim.c \
		  stat.c \
                  static.c \
		  statement.c \
		  symtable.c \
		  tcl_funcs.c \
		  toggle.c \
                  tree.c \
                  util.c \
                  vcd.c \
		  vector.c \
		  vsignal.c

covered_LDADD = $(TCLTK_LIBS)

INCLUDES=	-I..
AM_CPPFLAGS=-DINSTALL_DIR=\"@datadir@/covered\" $(TCLTK_CPPFLAGS)

parser.h parser.c: $(srcdir)/parser.y
	bison --verbose -t -p VL -d -o parser.c $(srcdir)/parser.y
	
pplexer.c: $(srcdir)/pplexer.l
	flex -PPPVL -opplexer.c $(srcdir)/pplexer.l

lexer.c:  $(srcdir)/lexer.l
	flex -PVL -olexer.c $(srcdir)/lexer.l

keywords.c:  keywords.gperf
	gperf -o -i 7 -C -k 1-3,$$ -L ANSI-C -H keyword_hash -N check_identifier -t $(srcdir)/keywords.gperf > keywords.c || (rm -f keywords.c ; false)

clean:
	rm -f pplexer.c lexer.c keywords.c
	rm -f parser.c parser.h parser.output
