######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

COVERED_SRC   = ../../src
COVERED       = $(COVERED_SRC)/covered
# COVERED_FLAGS = -q
COVERED_FLAGS = 

VERILOG       = iverilog
VERILOG_FLAGS =
VERILOG_EXEC  = a.out

CDD_DIR       = ../cdd

LOGFILE = regress.log

DIAGLIST = assign3.2A \
	   always3.1.11A \
           assign1 \
           cond1 \
           mult1 \
           always2 \
           always3 \
           always4

regress:	$(COVERED) clean $(DIAGLIST)

$(DIAGLIST):
	$(VERILOG) $(VERILOG_FLAGS) $@.v
	./$(VERILOG_EXEC)
	# $(COVERED) score -t main -vcd $@.vcd $(COVERED_FLAGS) -o $@.cdd -v $@.v >> $(LOGFILE)
	$(COVERED) score -t main -vcd $@.vcd $(COVERED_FLAGS) -o $@.cdd -v $@.v
	diff $@.cdd $(CDD_DIR)/$@.cdd
	rm -rf a.out $@.vcd $@.cdd

$(COVERED):
	cd $(COVERED_SRC); make
	
clean:
	rm -f $(LOGFILE)
	rm -f a.out *.vcd *.cdd *.log
	rm -f core
