######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

COVERED_SRC   = ../../src
COVERED       = $(COVERED_SRC)/covered
COVERED_GFLAG = -Q

IVERILOG       = iverilog
IVERILOG_FLAGS = -y ./lib -I ./include -DRUNTEST
IVERILOG_EXEC  = a.out

VCS       = vcs
VCS_FLAGS = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS
VCS_EXEC  = simv

VERILOG       = $(IVERILOG)
VERILOG_FLAGS = $(IVERILOG_FLAGS)
VERILOG_EXEC  = $(IVERILOG_EXEC)

CDD_DIR       = ../cdd

LOGFILE = regress.log

regress:	$(COVERED) clean $(DIAGLIST)

$(DIAGLIST):
	@echo Running $@
	@$(VERILOG) $(VERILOG_FLAGS) $@.v
	@./$(VERILOG_EXEC)
	@$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg
	@diff $@.cdd $(CDD_DIR)/$@.cdd
	@rm -rf a.out $@.vcd $@.cdd

$(COVERED):
	cd $(COVERED_SRC); make
	
clean:
	@rm -f $(LOGFILE)
	@rm -f $(IVERILOG_EXEC) $(VCS_EXEC) *.vcd *.cdd *.log
	@rm -f core
