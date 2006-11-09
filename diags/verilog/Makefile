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

ifdef VPI
CFG_CMD       = vpi
DUMP_FMT      =
CHK_CODE      = 5
else
ifdef LXT
CFG_CMD       = lxt
DUMP_FMT      = -lxt2
CHK_CODE      = 5
else
CFG_CMD       = vcd
DUMP_FMT      = -vcd
CHK_CODE      = 0
endif
endif

IVERILOG       = iverilog
IVERILOG_FLAGS = -cconfig_file -y ./lib -I./include -DRUNTEST
ifdef VPI
IVERILOG_FLAGS += -DVPI -m /home/ultrav/trevorw/libexec/covered.vpi covered_vpi.v
endif
IVERILOG_EXEC  = a.out
IVERILOG_INTER = vvp

VCS       = vcs
VCS_FLAGS = +libext+.v+.vlib+ -y ./lib -y ./ovl +incdir+./ovl +incdir+./include +define+RUNTEST +define+OVL_VERILOG +define+OVL_COVER_ON +v2k -sverilog -PP
ifdef VPI
VCS_FLAGS += +define+VPI +vpi -load /home/ultrav/trevorw/libexec/covered.vcs.so:covered_register -P covered_vpi.tab covered_vpi.v
endif
VCS_EXEC  = ./simv

CVER      = cver
CVER_FLAGS = +libext+.v+.vlib+ -y ./lib -y ./ovl +incdir+./ovl +incdir+./include +define+RUNTEST +define+OVL_VERILOG +define+OVL_COVER_ON
ifdef VPI
CVER_FLAGS += +loadvpi=/home/ultrav/trevorw/libexec/covered.cver.so:vpi_compat_bootstrap covered_vpi.v
endif
CVER_EXEC =

ifdef USE_VCS
VERILOG       = $(VCS)
VERILOG_FLAGS = $(VCS_FLAGS) $(VERILOG_EXTRA_FLAGS)
VERILOG_EXEC  = $(VCS_EXEC)
VERILOG_CMD   = $(VCS_EXEC)
else
ifdef USE_CVER
VERILOG       = $(CVER)
VERILOG       = $(CVER_FLAGS) $(VERILOG_EXTRA_FLAGS)
VERILOG_EXEC  = $(CVER_EXEC)
VERILOG_CMD   = $(CVER_EXEC)
else
VERILOG       = $(IVERILOG)
VERILOG_FLAGS = $(IVERILOG_FLAGS) $(VERILOG_EXTRA_FLAGS)
VERILOG_EXEC  = $(IVERILOG_EXEC)
VERILOG_CMD   = $(IVERILOG_INTER) $(IVERILOG_EXEC) $(DUMP_FMT)
endif
endif

CDD_DIR       = ../cdd
RPT_DIR       = ../rpt

LOGFILE = regress.log

.PHONY:	view_vg_errs check-ovl

regress:	$(COVERED) $(DIAGLIST) $(RACELIST) $(MERGELIST) $(MERGE3LIST) $(ERRSCORELIST) $(ERRMERGELIST) $(NOSIMLIST) $(NOSIMERRLIST)

check-ovl:
	@if test ! -h ovl; then \
		echo "Create symlink called \"ovl\" in the diags/verilog directory to point to base OVL vlib directory"; \
		exit 1; \
	fi

diagrun:
	@echo Running $(DIAG)
	@convert_cfg $(CFG_CMD) $(DIAG).cfg
ifdef VPI
	@$(COVERED) $(COVERED_GFLAG) score -f $(DIAG).cfg
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd +covered_debug; \
	else \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd; \
	fi
	@$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfam -o $(DIAG).rptM $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -w -d v -o $(DIAG).rptWM $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $(DIAG).rptWI $(DIAG).cdd
else
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
	@$(VERILOG_CMD)
endif
	@vg_exists=`which $(VG_EXEC) | wc -w`
	@if test "$$vg_exists" = "1"; then \
		$(VG_COVERED) $(COVERED_GFLAG) score -f $(DIAG).cfg; \
		$(VG_COVERED) $(COVERED_GFLAG) report -d v -m ltcfam -o $(DIAG).rptM $(DIAG).cdd; \
	else \
		$(COVERED) $(COVERED_GFLAG) score -f $(DIAG).cfg; \
		$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfam -o $(DIAG).rptM $(DIAG).cdd; \
	fi
	@$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -w -d v -o $(DIAG).rptWM $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $(DIAG).rptWI $(DIAG).cdd
endif
	@rm -f $(DIAG).cfg
	@./check_test $(DIAG) 1 $(CHK_CODE)

$(DIAGLIST): check-ovl
	@if test ! -f $@.done; then \
		$(MAKE) DIAG=$@ diagrun; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

racerun:
	@echo Running $(DIAG)
	@convert_cfg $(CFG_CMD) $(DIAG).cfg
ifdef VPI
	@$(VG_COVERED) $(COVERED_GFLAG) score -f $(DIAG).cfg
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd +covered_debug; \
	else \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd; \
	fi
	@$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -o $(DIAG).rptM $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -i -o $(DIAG).rptI $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -o $(DIAG).rptWM $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -i -o $(DIAG).rptWI $(DIAG).cdd
else
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
	@$(VERILOG_CMD)
endif
	@vg_exists=`which $(VG_EXEC) | wc -w`
	@if test "$$vg_exists" = "1"; then \
		$(VG_COVERED) $(COVERED_GFLAG) score -f $(DIAG).cfg; \
		$(VG_COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -o $(DIAG).rptM $(DIAG).cdd; \
	else \
		$(COVERED) $(COVERED_GFLAG) score -f $(DIAG).cfg; \
		$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -o $(DIAG).rptM $(DIAG).cdd; \
	fi
	@$(COVERED) $(COVERED_GFLAG) report -d v -m ltcfr -i -o $(DIAG).rptI $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -o $(DIAG).rptWM $(DIAG).cdd
	@$(COVERED) $(COVERED_GFLAG) report -w -d v -m ltcfr -i -o $(DIAG).rptWI $(DIAG).cdd
endif
	@rm -f $(DIAG).cfg
	@./check_test $(DIAG) 1 $(CHK_CODE)

$(RACELIST):
	@if test ! -f $@.done; then \
		make DIAG=$@ racerun; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(MERGELIST):
ifdef VPI
	@if test ! -f $@.done; then \
		echo Running $@; \
		convert_cfg $(CFG_CMD) $@a.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@a.cfg; \
		$(VERILOG) $(VERILOG_FLAGS) $@a.v; \
		$(VERILOG_CMD) +covered_cdd=$@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@a.rptI $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@a.rptWM $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@a.rptWI $@a.cdd; \
		rm -f $@a.cfg; \
		./check_test $@a 0 $(CHK_CODE); \
		convert_cfg $(CFG_CMD) $@b.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@b.cfg; \
		$(VERILOG) $(VERILOG_FLAGS) $@b.v; \
		$(VERILOG_CMD) +covered_cdd=$@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd; \
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
else
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@a.v; \
		$(VERILOG_CMD); \
		convert_cfg $(CFG_CMD) $@a.cfg; \
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
		convert_cfg $(CFG_CMD) $@b.cfg; \
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
endif

$(MERGE3LIST):
ifdef VPI
	@if test ! -f $@.done; then \
		echo Running $@; \
		convert_cfg $(CFG_CMD) $@a.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@a.cfg; \
		$(VERILOG) $(VERILOG_FLAGS) $@a.v; \
		$(VERILOG_CMD) +covered_cdd=$@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -o $@a.rptM $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@a.rptI $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@a.rptWM $@a.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@a.rptWI $@a.cdd; \
		rm -f $@a.cfg; \
		./check_test $@a 0 $(CHK_CODE); \
		convert_cfg $(CFG_CMD) $@b.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@b.cfg; \
		$(VERILOG) $(VERILOG_FLAGS) $@b.v; \
		$(VERILOG_CMD) +covered_cdd=$@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -o $@b.rptM $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -i -o $@b.rptI $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -o $@b.rptWM $@b.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -w -d v -i -o $@b.rptWI $@b.cdd; \
		rm -f $@b.cfg; \
		./check_test $@b 0 $(CHK_CODE); \
		convert_cfg $(CFG_CMD) $@c.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@c.cfg; \
		$(VERILOG) $(VERILOG_FLAGS) $@c.v; \
		$(VERILOG_CMD) +covered_cdd=$@c.cdd; \
		$(COVERED) $(COVERED_GFLAG) report -d v -o $@c.rptM $@c.cdd; \
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
else
	@if test ! -f $@.done; then \
		echo Running $@; \
		$(VERILOG) $(VERILOG_FLAGS) $@a.v; \
		$(VERILOG_CMD); \
		convert_cfg $(CFG_CMD) $@a.cfg; \
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
		convert_cfg $(CFG_CMD) $@b.cfg; \
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
		convert_cfg $(CFG_CMD) $@c.cfg; \
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
endif

$(ERRSCORELIST):
	@if test ! -f $@.done; then \
		echo Running $@ -- should see an error message; \
		$(VERILOG) $(VERILOG_FLAGS) $@.v; \
		$(VERILOG_CMD); \
		convert_cfg $(CFG_CMD) $@.cfg; \
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
		convert_cfg $(CFG_CMD) $@a.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@a.cfg; \
		rm -f $@a.cfg; \
		$(VERILOG) $(VERILOG_FLAGS) $@b.v; \
		$(VERILOG_CMD); \
		convert_cfg $(CFG_CMD) $@b.cfg; \
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
		convert_cfg $(CFG_CMD) $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) report -d v $@.cdd 2> $@.err; \
		rm -f $@.cfg; \
		cat $@.err; \
		./check_test $@ 1 1; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(NOSIMLIST):
	@if test ! -f $@.done; then \
		echo Running $@; \
		convert_cfg $(CFG_CMD) $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@.cfg 2> $@.err; \
		rm -f $@.cfg $@.cdd; \
		cat $@.err; \
		./check_test $@ 1 1; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(NOSIMERRLIST):
	@if test ! -f $@.done; then \
		echo Running $@ -- should see an error message; \
		convert_cfg $(CFG_CMD) $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) score -f $@.cfg 2> $@.err; \
		rm -f $@.cfg; \
		cat $@.err; \
		./check_test $@ 1 1; \
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
