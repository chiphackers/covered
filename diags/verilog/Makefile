######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

COVERED_SRC   = ../../src
COVERED       = $(COVERED_SRC)/covered
COVERED_FLAGS = -q
# COVERED_FLAGS = 

VERILOG       = iverilog
VERILOG_FLAGS =
VERILOG_EXEC  = a.out

CDD_DIR       = ../cdd

LOGFILE = regress.log

DIAGLIST = assign3.2A \
	   always3.1.11A \
           assign1 \
           cond1 \
	   cond2 \
           mult1 \
           always1 \
           always2 \
           always3 \
           always4 \
           always5 \
           case1 \
           case2 \
           case3 \
           case3.1 \
           case3.2 \
           case3.3 \
           case3.4 \
	   casex1 \
	   casex1.1 \
	   casex1.2 \
	   casex1.3 \
	   casex1.4 \
	   casez1 \
	   casez1.1 \
	   casez1.2 \
	   casez1.3 \
	   casez1.4 \
	   casez2

regress:	$(COVERED) clean $(DIAGLIST)

$(DIAGLIST):
	$(VERILOG) $(VERILOG_FLAGS) $@.v
	./$(VERILOG_EXEC)
	$(COVERED) score -t main -vcd $@.vcd $(COVERED_FLAGS) -o $@.cdd -v $@.v >> $(LOGFILE)
	diff $@.cdd $(CDD_DIR)/$@.cdd
	rm -rf a.out $@.vcd $@.cdd

$(COVERED):
	cd $(COVERED_SRC); make
	
clean:
	rm -f $(LOGFILE)
	rm -f a.out *.vcd *.cdd *.log
	rm -f core
