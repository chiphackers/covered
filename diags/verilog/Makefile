######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

COVERED_SRC   = ../../src
COVERED       = $(COVERED_SRC)/covered
COVERED_FLAGS = -y ./lib -I ./include -D RUNTEST
# COVERED_FLAGS = -y ./lib -I ./include

VERILOG       = iverilog
VERILOG_FLAGS = -y ./lib -I ./include -DRUNTEST
VERILOG_EXEC  = a.out

CDD_DIR       = ../cdd

LOGFILE = regress.log


DIAGLIST = assign3.2A    assign1       always1       always3.1.11A  cond1 \
	   cond2         mult1         always2       always3        always4 \
           always5       case1         case1.1       case2          case3 \
           case3.1       case3.2       case3.3       case3.4        casex1 \
	   casex1.1      casex1.2      casex1.3      casex1.4       casez1 \
	   casez1.1      casez1.2      casez1.3      casez1.4       casez2 \
           ifelse1       define1       define1.1     define2        define2.1 \
           static1       include1      include1.1    ifdef1         ifdef2.1 \
           ifdef2.2      concat1       concat2       expand1        expand2 \
           sbit_sel1     instance1     instance2     instance3      mbit_sel1 \
           implicit1

.SILENT:

regress:	$(COVERED) clean $(DIAGLIST)
	echo REGRESSION PASSED!

$(DIAGLIST):
	echo Running $@.v
	$(VERILOG) $(VERILOG_FLAGS) $@.v
	./$(VERILOG_EXEC)
	$(COVERED) -Q score -t main -vcd $@.vcd -o $@.cdd -v $@.v $(COVERED_FLAGS)
	diff $@.cdd $(CDD_DIR)/$@.cdd
	rm -rf a.out $@.vcd $@.cdd

$(COVERED):
	cd $(COVERED_SRC); make
	
clean:
	rm -f $(LOGFILE)
	rm -f a.out *.vcd *.cdd *.log
	rm -f core
