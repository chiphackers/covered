######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

COVERED_SRC   = ../../src
COVERED       = $(COVERED_SRC)/covered
COVERED_GFLAG = -D

IVERILOG       = iverilog
IVERILOG_FLAGS = -y ./lib -I ./include -DRUNTEST
IVERILOG_EXEC  = a.out
IVERILOG_INTER = vvp

VCS       = vcs
#VCS_FLAGS = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS +vpi -load ../../src/vpi/covered.so:covered_register -P ../../src/vpi/covered.tab covered_top.v
VCS_FLAGS = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST
VCS_EXEC  = ./simv +covered_outdb=out.cdd

VERILOG       = $(IVERILOG)
VERILOG_FLAGS = $(IVERILOG_FLAGS)
VERILOG_EXEC  = $(IVERILOG_EXEC)
VERILOG_CMD   = $(IVERILOG_INTER) $(IVERILOG_EXEC)
#VERILOG       = $(VCS)
#VERILOG_FLAGS = $(VCS_FLAGS)
#VERILOG_EXEC  = $(VCS_EXEC)
#VERILOG_CMD   = $(VCS_EXEC)

CDD_DIR       = ../cdd
RPT_DIR       = ../rpt

LOGFILE = regress.log

regress:	$(COVERED) $(DIAGLIST) $(RACELIST) $(MERGELIST) $(ERRSCORELIST) $(ERRMERGELIST)

$(DIAGLIST):
	@echo Running $@
	@$(VERILOG) $(VERILOG_FLAGS) $@.v
	@$(VERILOG_CMD)
	@$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg
	-@$(COVERED) $(COVERED_GFLAG) report -d v -o $@.rptM $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@.rptI $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@.rptWM $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@.rptWI $@.cdd
	@./check_test $@ 1

$(RACELIST):
	@echo Running $@
	@$(VERILOG) $(VERILOG_FLAGS) $@.v
	@$(VERILOG_CMD)
	@$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg
	-@$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -o $@.rptM $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -i -o $@.rptI $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -o $@.rptWM $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -i -o $@.rptWI $@.cdd
	@./check_test $@ 1

$(MERGELIST):
	@echo Running $@
	@$(VERILOG) $(VERILOG_FLAGS) $@a.v
	@$(VERILOG_CMD)
	@$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@a.cfg
	-@$(COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@a.rptI $@a.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@a.rptWM $@a.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@a.rptWI $@a.cdd
	@./check_test $@a 0
	@$(VERILOG) $(VERILOG_FLAGS) $@b.v
	@$(VERILOG_CMD)
	@$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@b.cfg
	-@$(COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@b.rptI $@b.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@b.rptWM $@b.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@b.rptWI $@b.cdd
	@./check_test $@b 0
	@$(COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -d v -o $@.rptM $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@.rptI $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@.rptWM $@.cdd
	-@$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@.rptWI $@.cdd
	@./check_test $@ 1

$(ERRSCORELIST):
	@echo Running $@
	@$(VERILOG) $(VERILOG_FLAGS) $@.v
	@$(VERILOG_CMD)
	@$(COVERED) -D score -f ../regress/$@.cfg | tee $@.err

$(ERRMERGELIST):
	@echo Running $@
	@$(VERILOG) $(VERILOG_FLAGS) $@a.v
	@$(VERILOG_CMD)
	@$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@a.cfg
	@$(VERILOG) $(VERILOG_FLAGS) $@b.v
	@$(VERILOG_CMD)
	@$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@b.cfg
	@$(COVERED) -D merge -o $@.cdd $@a.cdd $@b.cdd | tee $@.err
	@./check_err $@

$(ERRREPORTLIST):
	@echo Running $@
	@$(VERILOG) $(VERILOG_FLAGS) $@.v
	@$(VERILOG_CMD)
	@$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg
	@$(COVERED) -D report -d v $@.cdd | tee $@.err

$(COVERED):
	cd $(COVERED_SRC); make
	
clobber:  clean
	@rm -f *.rpt* *.cdd *.err

clean:
	@rm -f $(LOGFILE)
	@rm -f $(IVERILOG_EXEC) $(VCS_EXEC) *.vcd *.log
	@rm -f core *.o tmp* regress.output
