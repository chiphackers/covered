bin_PROGRAMS = covered
covered_SOURCES = parser.c \
		  binding.c \
		  codegen.c \
		  comb.c \
		  db.c \
		  expr.c \
		  fsm.c \
		  instance.c \
 		  iter.c \
		  keywords.c \
		  lexer.c \
		  line.c \
                  link.c \
		  main.c \
		  merge.c \
		  module.c \
		  param.c \
		  parse.c \
		  parser_misc.c \
		  report.c \
		  score.c \
		  search.c \
		  signal.c \
		  sim.c \
		  stat.c \
                  static.c \
		  statement.c \
		  symtable.c \
		  toggle.c \
                  util.c \
                  vcd.c \
		  vector.c

INCLUDES=	-I..
parser.h parser.c: $(srcdir)/parser.y
	bison --verbose -t -p VL -d -o parser.c $(srcdir)/parser.y
	
lexer.c:  $(srcdir)/lexer.l
	flex -PVL -s -olexer.c $(srcdir)/lexer.l

keywords.c:  keywords.gperf
	gperf -o -i 7 -C -k 1-3,$$ -L ANSI-C -H keyword_hash -N check_identifier -t $(srcdir)/keywords.gperf > keywords.c || (rm -f keywords.c ; false)

clean:
	rm -f lexer.c keywords.c
	rm -f parser.c parser.h parser.output
	rm -rf .deps
