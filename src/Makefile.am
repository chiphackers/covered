bin_PROGRAMS = covered
covered_SOURCES = main.c \
		  search.c \
		  parse.c \
                  util.c \
                  link.c \
		  score.c \
		  merge.c \
		  report.c \
		  vector.c \
		  expr.c \
		  signal.c \
		  module.c \
		  db.c \
		  symtable.c \
		  parser_misc.c \
		  parser.c \
		  keywords.c \
		  lexer.c \
		  vcd_parser_misc.c \
		  vcd_parser.c \
		  vcd_lexer.c \
		  instance.c \
		  binding.c \
		  line.c \
		  toggle.c \
		  comb.c \
		  fsm.c \
		  stat.c \
		  codegen.c \
		  statement.c \
		  sim.c

parser.h parser.c: $(srcdir)/parser.y
	bison --verbose -t -p VL -d -o parser.c $(srcdir)/parser.y
	
lexer.c:  $(srcdir)/lexer.l
	flex -PVL -s -olexer.c $(srcdir)/lexer.l

keywords.c:  keywords.gperf
	gperf -o -i 7 -C -k 1-3,$$ -L ANSI-C -H keyword_hash -N check_identifier -t $(srcdir)/keywords.gperf > keywords.c || (rm -f keywords.c ; false)

vcd_parser.h vcd_parser.c: $(srcdir)/vcd_parser.y
	bison --verbose -t -p VCD -d -o vcd_parser.c $(srcdir)/vcd_parser.y

vcd_lexer.c:  $(srcdir)/vcd_lexer.l
	flex -PVCD -s -ovcd_lexer.c $(srcdir)/vcd_lexer.l

clean:
	rm -f lexer.c keywords.c
	rm -f parser.c parser.h parser.output
	rm -f vcd_lexer.c
	rm -f vcd_parser.c vcd_parser.h vcd_parser.output
	rm -rf .deps
