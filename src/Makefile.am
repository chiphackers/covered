bin_PROGRAMS = covered
covered_SOURCES = arc.c \
                  assertion.c \
                  attr.c \
                  parser.c \
                  static_parser.c \
		  binding.c \
		  codegen.c \
		  comb.c \
		  db.c \
                  enumerate.c \
                  exclude.c \
		  expr.c \
		  fsm.c \
                  fsm_arg.c \
		  fsm_var.c \
		  func_unit.c \
                  gen_item.c \
		  gui.c \
		  info.c \
		  instance.c \
 		  iter.c \
		  keywords_1995.c \
		  keywords_2001.c \
		  keywords_sv.c \
                  pplexer.c \
		  lexer.c \
		  line.c \
                  link.c \
		  lxt.c \
		  lxt2_read.c \
		  main.c \
                  memory.c \
		  merge.c \
                  obfuscate.c \
                  ovl.c \
		  param.c \
		  parse.c \
		  parser_misc.c \
		  perf.c \
                  race.c \
		  report.c \
		  scope.c \
		  score.c \
		  search.c \
		  sim.c \
		  stat.c \
                  static.c \
                  static_lexer.c \
		  statement.c \
		  stmt_blk.c \
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
AM_CPPFLAGS=-DINSTALL_DIR=\"@datadir@/covered\" $(TCLTK_CPPFLAGS) $(DEBUGDEF)

parser.h parser.c: $(srcdir)/parser.y
	bison --verbose -t -p VL -d -o parser.c $(srcdir)/parser.y

static_parser.h static_parser.c: $(srcdir)/static_parser.y
	bison --verbose -t -p SE -d -o static_parser.c $(srcdir)/static_parser.y
	
pplexer.c: $(srcdir)/pplexer.l
	flex -PPPVL -opplexer.c $(srcdir)/pplexer.l

lexer.c:  $(srcdir)/lexer.l
	flex -PVL -olexer.c $(srcdir)/lexer.l

static_lexer.c: $(srcdir)/static_lexer.l
	flex -PSE -ostatic_lexer.c $(srcdir)/static_lexer.l

keywords_1995.c:  keywords_1995.gperf
	gperf -o -i 7 -C -k 1-3,$$ -L ANSI-C -H keyword_hash_1995 -N check_identifier_1995 -t $(srcdir)/keywords_1995.gperf > keywords_1995.c || (rm -f keywords_1995.c ; false)

keywords_2001.c:  keywords_2001.gperf
	gperf -o -i 7 -C -k 1-3,$$ -L ANSI-C -H keyword_hash_2001 -N check_identifier_2001 -t $(srcdir)/keywords_2001.gperf > keywords_2001.c || (rm -f keywords_2001.c ; false)

keywords_sv.c:  keywords_sv.gperf
	gperf -o -i 7 -C -k 1-3,$$ -L ANSI-C -H keyword_hash_sv -N check_identifier_sv -t $(srcdir)/keywords_sv.gperf > keywords_sv.c || (rm -f keywords_sv.c ; false)

CLEANFILES = pplexer.c lexer.c static_lexer.c keywords_1995.c keywords_2001.c keywords_sv.c parser.c parser.h parser.output static_parser.c static_parser.h static_parser.output
