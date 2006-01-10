######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

VG_LOG_PREFIX = mem_check
VG_EXEC       = valgrind
VG_FLAGS      = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=10 --log-file=$(VG_LOG_PREFIX) --suppressions=vg_supp -v

COVERED_SRC   = ../../src
COVERED       = $(COVERED_SRC)/covered
VG_COVERED    = $(VG_EXEC) $(VG_FLAGS) $(COVERED)
COVERED_GFLAG = -D

IVERILOG       = iverilog
IVERILOG_FLAGS = -y ./lib -I ./include -DRUNTEST
IVERILOG_EXEC  = a.out
IVERILOG_INTER = vvp

VCS       = vcs
#VCS_FLAGS = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS +vpi -load ../../src/vpi/covered.so:covered_register -P ../../src/vpi/covered.tab covered_top.v
VCS_FLAGS = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +v2k
VCS_EXEC  = ./simv +covered_outdb=out.cdd

ifdef USE_VCS
VERILOG       = $(VCS)
VERILOG_FLAGS = $(VCS_FLAGS)
VERILOG_EXEC  = $(VCS_EXEC)
VERILOG_CMD   = $(VCS_EXEC)
else
VERILOG       = $(IVERILOG)
VERILOG_FLAGS = $(IVERILOG_FLAGS)
VERILOG_EXEC  = $(IVERILOG_EXEC)
VERILOG_CMD   = $(IVERILOG_INTER) $(IVERILOG_EXEC)
endif

CDD_DIR       = ../cdd
RPT_DIR       = ../rpt

LOGFILE = regress.log

.PHONY:	view_vg_errs

regress:	$(COVERED) $(DIAGLIST) $(RACELIST) $(MERGELIST) $(ERRSCORELIST) $(ERRMERGELIST)

$(DIAGLIST):
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@.v; \
		$(VERILOG_CMD); \
		vg_exists=`which $(VG_EXEC) | wc -w`; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -o $@.rptM $@.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -o $@.rptM $@.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@.rptI $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@.rptWM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@.rptWI $@.cdd; \
		./check_test $@ 1 0; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(RACELIST):
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@.v; \
		$(VERILOG_CMD); \
		vg_exists=`which $(VG_EXEC) | wc -w`; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -o $@.rptM $@.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -o $@.rptM $@.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -i -o $@.rptI $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -o $@.rptWM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -i -o $@.rptWI $@.cdd; \
		./check_test $@ 1 0; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(MERGELIST):
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@a.v; \
		$(VERILOG_CMD); \
		vg_exists=`which $(VG_EXEC) | wc -w`; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f ../regress/$@a.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@a.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@a.rptI $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@a.rptWM $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@a.rptWI $@a.cdd; \
		./check_test $@a 0 0; \
		$(VERILOG) $(VERILOG_FLAGS) $@b.v; \
		$(VERILOG_CMD); \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f ../regress/$@b.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@b.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@b.rptI $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@b.rptWM $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@b.rptWI $@b.cdd; \
		./check_test $@b 0 0; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -o $@.rptM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@.rptI $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@.rptWM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@.rptWI $@.cdd; \
		./check_test $@ 1 0; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(ERRSCORELIST):
	@if test ! -f $@.done; then \
		echo Running $@ -- should see an error message; \
		$(VERILOG) $(VERILOG_FLAGS) $@.v; \
		$(VERILOG_CMD); \
		$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg | tee $@.err; \
		./check_test $@ 1 1; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(ERRMERGELIST):
	@if test ! -f $@.done; then \
		echo Running $@ -- should see an error message; \
		$(VERILOG) $(VERILOG_FLAGS) $@a.v; \
		$(VERILOG_CMD); \
		$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@a.cfg; \
		$(VERILOG) $(VERILOG_FLAGS) $@b.v; \
		$(VERILOG_CMD); \
		$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@b.cfg; \
		$(COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd | tee $@.err; \
		./check_test $@ 1 1; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(ERRREPORTLIST):
	@if test ! -f $@.done; then \
		echo Running $@ -- should see an error message; \
		$(VERILOG) $(VERILOG_FLAGS) $@.v; \
		$(VERILOG_CMD); \
		$(COVERED) $(COVERED_GFLAG) score -f ../regress/$@.cfg; \
		$(COVERED) $(COVERED_GFLAG) report -d v $@.cdd | tee $@.err; \
		./check_test $@ 1 1 || \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(COVERED):
	cd $(COVERED_SRC); make

eot:
	@./check_test dummy 1 3

view_vg_errs:
	@./view_vg_errs $(VG_LOG_PREFIX)
	
reset:  clobber
	@rm -f *.done

clobber:  clean
	@rm -f *.rpt* *.cdd *.err regress.output regress.failed
	@rm -f $(VG_LOG_PREFIX).*

clean:
	@rm -f $(LOGFILE)
	@rm -f $(IVERILOG_EXEC) $(VCS_EXEC) *.vcd *.log
	@rm -f core *.o
