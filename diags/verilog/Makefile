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

LOGFILE = regress.log

DIAGLIST = addsr \
	   assign3.2A

regress:	$(COVERED) clean $(DIAGLIST)

$(DIAGLIST):
	$(VERILOG) $(VERILOG_FLAGS) $@.v
	./$(VERILOG_EXEC)
	$(COVERED) score -t main -vcd $@.vcd $(COVERED_FLAGS) -o $@.db -v $@.v

$(COVERED):
	cd $(COVERED_SRC); make
	
clean:
	rm -f $(LOGFILE)
	rm -f a.out *.vcd *.db *.log
	rm -f core
