######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         3/22/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

VG_LOG_PREFIX = mem_check
VG_EXEC       = valgrind
VG_FLAGS      = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=15 --log-file=$(VG_LOG_PREFIX) --suppressions=vg_supp -v

COVERED_SRC   = ../../src
COVERED       = $(COVERED_SRC)/covered
COVERED_GFLAG = -D

ifdef CHECK_MEM
VG_COVERED    = $(VG_EXEC) $(VG_FLAGS) $(COVERED)
else
VG_COVERED    = $(COVERED)
endif

ifdef LEAVE
FINISH_CODE   = 4
else
FINISH_CODE   = 3
endif

ifdef LXT
CFG_COPY_CMD  = sed 's/-vcd/-lxt/'
DUMP_FMT      = -lxt2
CHK_CODE      = 5
else
CFG_COPY_CMD  = cat
DUMP_FMT      = -vcd
CHK_CODE      = 0
endif

IVERILOG       = iverilog
IVERILOG_FLAGS = -cconfig_file -y ./lib -y $(OVL_HOME) -I$(OVL_HOME) -I./include -DRUNTEST
IVERILOG_EXEC  = a.out
IVERILOG_INTER = vvp

VCS       = vcs
#VCS_FLAGS = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS +vpi -load ../../src/vpi/covered.so:covered_register -P ../../src/vpi/covered.tab covered_top.v
VCS_FLAGS = +libext+.v+.vlib+ -y ./lib -y $(OVL_HOME) +incdir+$(OVL_HOME) +incdir+./include +define+RUNTEST +define+OVL_VERILOG +define+OVL_COVER_ON +v2k
VCS_EXEC  = ./simv +covered_cdd=out.cdd

ifdef USE_VCS
VERILOG       = $(VCS)
VERILOG_FLAGS = $(VCS_FLAGS)
VERILOG_EXEC  = $(VCS_EXEC)
VERILOG_CMD   = $(VCS_EXEC)
else
VERILOG       = $(IVERILOG)
VERILOG_FLAGS = $(IVERILOG_FLAGS)
VERILOG_EXEC  = $(IVERILOG_EXEC)
VERILOG_CMD   = $(IVERILOG_INTER) $(IVERILOG_EXEC) $(DUMP_FMT)
endif

CDD_DIR       = ../cdd
RPT_DIR       = ../rpt

LOGFILE = regress.log

.PHONY:	view_vg_errs check-ovl

regress:	$(COVERED) $(DIAGLIST) $(RACELIST) $(MERGELIST) $(MERGE3LIST) $(ERRSCORELIST) $(ERRMERGELIST)

check-ovl:
ifndef OVL_HOME
	@echo "Set OVL_HOME environment variable to point to base OVL vlib directory"; exit 1
endif

$(DIAGLIST): check-ovl
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@.cfg > $@.cfg; \
		vg_exists=`which $(VG_EXEC) | wc -w`; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f $@.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -m ltcfa -o $@.rptM $@.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f $@.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfa -o $@.rptM $@.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfa -i -o $@.rptI $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@.rptWM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@.rptWI $@.cdd; \
		rm -f $@.cfg; \
		./check_test $@ 1 $(CHK_CODE); \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(RACELIST):
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@.cfg > $@.cfg; \
		vg_exists=`which $(VG_EXEC) | wc -w`; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f $@.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -o $@.rptM $@.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f $@.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -o $@.rptM $@.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -i -o $@.rptI $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -o $@.rptWM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -i -o $@.rptWI $@.cdd; \
		rm -f $@.cfg; \
		./check_test $@ 1 $(CHK_CODE); \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(MERGELIST):
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@a.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@a.cfg > $@a.cfg; \
		vg_exists=`which $(VG_EXEC) | wc -w`; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f $@a.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f $@a.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@a.rptI $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@a.rptWM $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@a.rptWI $@a.cdd; \
		rm -f $@a.cfg; \
		./check_test $@a 0 $(CHK_CODE); \
		$(VERILOG) $(VERILOG_FLAGS) $@b.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@b.cfg > $@b.cfg; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f $@b.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f $@b.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@b.rptI $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@b.rptWM $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@b.rptWI $@b.cdd; \
		rm -f $@b.cfg; \
		./check_test $@b 0 $(CHK_CODE); \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -o $@.rptM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@.rptI $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@.rptWM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@.rptWI $@.cdd; \
		./check_test $@ 1 $(CHK_CODE); \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(MERGE3LIST):
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@a.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@a.cfg > $@a.cfg; \
		vg_exists=`which $(VG_EXEC) | wc -w`; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f $@a.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f $@a.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@a.rptI $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@a.rptWM $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@a.rptWI $@a.cdd; \
		rm -f $@a.cfg; \
		./check_test $@a 0 $(CHK_CODE); \
		$(VERILOG) $(VERILOG_FLAGS) $@b.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@b.cfg > $@b.cfg; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f $@b.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f $@b.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@b.rptI $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@b.rptWM $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@b.rptWI $@b.cdd; \
		rm -f $@b.cfg; \
		./check_test $@b 0 $(CHK_CODE); \
		$(VERILOG) $(VERILOG_FLAGS) $@c.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@c.cfg > $@c.cfg; \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) score -f $@c.cfg; \
			$(VG_COVERED) $(COVERED_GFLAG) report -d v -o $@c.rptM $@c.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) score -f $@c.cfg; \
			$(COVERED) $(COVERED_GFLAG) report -d v -o $@c.rptM $@c.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@c.rptI $@c.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@c.rptWM $@c.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@c.rptWI $@c.cdd; \
		rm -f $@c.cfg; \
		./check_test $@c 0 $(CHK_CODE); \
		if test "$$vg_exists" = "1"; then \
			$(VG_COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd $@c.cdd; \
		else \
			$(COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd $@c.cdd; \
		fi; \
		$(COVERED) $(COVERED_GFLAG) report -d v -o $@.rptM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@.rptI $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@.rptWM $@.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@.rptWI $@.cdd; \
		./check_test $@ 1 $(CHK_CODE); \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(ERRSCORELIST):
	@if test ! -f $@.done; then \
		echo Running $@ -- should see an error message; \
		$(VERILOG) $(VERILOG_FLAGS) $@.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@.cfg > $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@.cfg 2> $@.err; \
		rm -f $@.cfg; \
		cat $@.err; \
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
		$(CFG_COPY_CMD) ../regress/$@a.cfg > $@a.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@a.cfg; \
		rm -f $@a.cfg; \
		$(VERILOG) $(VERILOG_FLAGS) $@b.v; \
		$(VERILOG_CMD); \
		$(CFG_COPY_CMD) ../regress/$@b.cfg > $@b.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@b.cfg; \
		$(COVERED) $(COVERED_GFLAG) merge -o $@.cdd $@a.cdd $@b.cdd 2> $@.err; \
		rm -f $@b.cfg; \
		cat $@.err; \
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
		$(CFG_COPY_CMD) ../regress/$@.cfg > $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) report -d v $@.cdd 2> $@.err; \
		rm -f $@.cfg; \
		cat $@.err; \
		./check_test $@ 1 1 || \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(COVERED):
	cd $(COVERED_SRC); make

eot:
	@./check_test dummy 1 $(FINISH_CODE)

view_vg_errs:
	@./view_vg_errs $(VG_LOG_PREFIX)
	
reset:  clobber
	@rm -f *.done

clobber:  clean
	@rm -f *.rpt* *.cdd *.err regress.output regress.failed
	@rm -f $(VG_LOG_PREFIX).*

clean:
	@rm -f $(LOGFILE)
	@rm -f $(IVERILOG_EXEC) $(VCS_EXEC) *.vcd *.log *.cfg
	@rm -f core *.o
