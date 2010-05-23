######################################################
# Written by:   Trevor Williams  (phase1geo@gmail.com)
#
# Date:         3/22/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

PROCESSOR = $(shell uname -p)

VG_LOG_PREFIX = mem_check
VG_EXEC       = valgrind
VG_FLAGS      = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=15 --log-file=$(VG_LOG_PREFIX) --suppressions=vg_supp -v

COVERED_SRC         = ../../src
COVERED_VPILIB      = ../../lib/vpi
COVERED_RUNLIB      = ../../lib/run
COVERED             = $(COVERED_SRC)/covered
COVERED_GFLAG       = -D
COVERED_SCORE_GFLAG = -P

ifeq ($(COVERED_GFLAG),-D)
VCDDIFF_VERBOSE = -v
endif

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
ifdef FST
CFG_CMD       = fst
DUMP_FMT      = -fst
CHK_CODE      = 5
else
CFG_CMD       = vcd
DUMP_FMT      = -vcd
CHK_CODE      = 0
endif
endif
endif
ifdef INLINE
CHK_CODE      = 5
TOPDIR        = covered/verilog/
endif

SIM_PLUSARGS = +option1 +option2 +optionB=0000000100100011010001010110011110001001101010111100110111101111 +optionO=0123456776543210012345 +optionD=81985529216486895 +optionH=0123456789abcdef +optionE=123.456 +optionF=123.456 +optionG=123.456 +optionS1=\"Small +optionS2=small.dog

# Icarus Verilog support
IVERILOG       = iverilog
IVERILOG_FLAGS = -cconfig_file -I./include -DRUNTEST
ifdef VPI
IVERILOG_FLAGS += -m ../../lib/vpi/covered.vpi
else
IVERILOG_FLAGS += -DDUMP
endif
IVERILOG_EXEC  = a.out $(SIM_PLUSARGS)
IVERILOG_INTER = vvp

# VCS support
VCS       = vcs
VCS_FLAGS = +librescan +libext+.v+.vlib+ -y ./covered/verilog -y ./lib -y ./ovl +incdir+./ovl +incdir+./include +define+RUNTEST +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 +v2k -sverilog -ntb_opts dtm -PP +vcs+lic+wait -v2k_generate
ifdef VPI
VCS_FLAGS += +vpi -load ../../lib/vpi/covered.vcs.so:covered_register
ifeq ($(PROCESSOR),x86_64)
VCS_FLAGS += -full64
endif
else
VCS_FLAGS += +define+DUMP
endif
VCS_EXEC  = ./simv $(SIM_PLUSARGS)

# Cver support
CVER      = cver
CVER_FLAGS = -q +librescan +libext+.v+.vlib+ -y ./covered/verilog -y ./lib -y ./ovl +incdir+./ovl +incdir+./include +define+RUNTEST +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 $(SIM_PLUSARGS)
ifdef VPI
CVER_FLAGS += +loadvpi=../../lib/vpi/covered.cver.so:vpi_compat_bootstrap
else
CVER_FLAGS += +define+DUMP
endif
CVER_EXEC =

# Veriwell support
VERIWELL       = veriwell
VERIWELL_FLAGS = +libext+.v+.vlib -y ./lib -y ./covered/verilog -y ./ovl +incdir+./ovl +incdir+./include +define+RUNTEST +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 +define+DUMP +define+V1995_COV_MODE $(SIM_PLUSARGS)
VERIWELL_EXEC  =

# VComp support
VCOMP_LIB      = $(realpath $(dir $(shell which vcomp))../lib)
VCOMP          = vcomp
VCOMP_FLAGS    = -d0 -logfile foobar +libext+.v+.vlib -y ./covered/verilog -y ./lib -y ./ovl +incdir+./ovl +incdir+./include +define+RUNTEST +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT+15 +define+DUMP -LL $(VCOMP_LIB) -P $(VCOMP_LIB)/vcomp/vcd.tab
VCOMP_EXEC     = ./vrun $(SIM_PLUSARGS)

# Verilator support
VERILATOR       = verilator
# VERILATOR_FLAGS = --cc --trace --Mdir $(DIAG) -Wno-UNOPTFLAT -Wno-WIDTH --exe $(DIAG).cc
VERILATOR_FLAGS = --cc --trace --Mdir $(DIAG) -Wno-WIDTH --exe $(DIAG).cc
VERILATOR_EXEC1 = cd $(DIAG); make -f V$(basename $(DIAG)).mk; ./V$(basename $(DIAG)); cp $(DIAG).vcd ..
ifdef INLINE
VERILATOR_EXEC2 = cd $(DIAG); make -f V$(basename $(DIAG)).mk USER_CPPFLAGS="-I../../../src -DCOVERED_INLINED" LIBS="-L../../../lib/run -lcovered"; ./V$(basename $(DIAG)); cp $(DIAG).vcd ..
else
VERILATOR_EXEC2 = $(VERILATOR_EXEC1)
endif

ifdef USE_VERILATOR
VERILOG        = $(VERILATOR)
VERILOG_FLAGS  = $(VERILATOR_FLAGS)
VERILOG_EXEC   = $(VERILATOR_EXEC)
VERILOG_PRECMD = $(VERILATOR_EXEC1)
VERILOG_CMD    = $(VERILATOR_EXEC2)
VERILATOR_SET  = 1
else
VERILATOR_SET  = 0
ifdef USE_VERIWELL
VERILOG        = $(VERIWELL)
VERILOG_FLAGS  = $(VERIWELL_FLAGS) $(VERILOG_EXTRA_FLAGS)
VERILOG_EXEC   = $(VERIWELL_EXEC)
VERILOG_PRECMD = $(VERIWELL_EXEC)
VERILOG_CMD    = $(VERIWELL_EXEC)
else
ifdef USE_VCS
VERILOG        = $(VCS)
VERILOG_FLAGS  = $(VCS_FLAGS) $(VERILOG_EXTRA_FLAGS)
VERILOG_EXEC   = $(VCS_EXEC)
VERILOG_PRECMD = $(VCS_EXEC)
VERILOG_CMD    = $(VCS_EXEC)
else
ifdef USE_CVER
VERILOG        = $(CVER)
VERILOG_FLAGS  = $(CVER_FLAGS) $(VERILOG_EXTRA_FLAGS)
VERILOG_EXEC   = $(CVER_EXEC)
VERILOG_PRECMD = $(CVER_EXEC)
VERILOG_CMD    = $(CVER_EXEC)
else
ifdef USE_VCOMP
VERILOG        = $(VCOMP)
VERILOG_FLAGS  = $(VCOMP_FLAGS) $(VERILOG_EXTRA_FLAGS)
VERILOG_EXEC   = $(VCOMP_EXEC)
VERILOG_PRECMD = $(VCOMP_EXEC)
VERILOG_CMD    = $(VCOMP_EXEC)
else
VERILOG        = $(IVERILOG)
VERILOG_FLAGS  = $(IVERILOG_FLAGS) $(VERILOG_EXTRA_FLAGS)
VERILOG_EXEC   = $(IVERILOG_EXEC)
VERILOG_PRECMD = $(IVERILOG_INTER) $(IVERILOG_EXEC) $(DUMP_FMT)
VERILOG_CMD    = $(IVERILOG_INTER) $(IVERILOG_EXEC) $(DUMP_FMT)
endif
endif
endif
endif
endif

CDD_DIR       = ../cdd
RPT_DIR       = ../rpt

export COVERED_TESTMODE=1
export LIBRARY_PATH=lib

CHECKMEM := $(shell ./check_testmode)

LOGFILE = regress.log

.PHONY:	view_vg_errs check-ovl covered

regress:	covered $(DIAGLIST) $(DIAG2LIST) $(RACELIST) $(MERGELIST) $(MERGE3LIST) $(ERRSCORELIST) $(ERRMERGELIST) $(NOSIMLIST) $(NOSIMERRLIST) $(ERRREPORTLIST) $(ERRRANKLIST) $(ERREXCLUDELIST)

check-ovl:
	@if test ! -h ovl; then \
		echo "Create symlink called \"ovl\" in the diags/verilog directory to point to base OVL vlib directory"; \
		exit 1; \
	fi

diagrun:
	@echo Running $(DIAG)
ifdef VPI
ifdef INLINE
	@./convert_cfg "inline -vpi" 0 0 $(DIAG).cfg
else
	@./convert_cfg vpi 0 0 $(DIAG).cfg
endif
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
ifdef USE_CVER
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG) $(VERILOG_FLAGS) $(TOP) covered_vpi.v +covered_cdd=$(DIAG).cdd +covered_debug $(SIM_PLUSARGS); \
	else \
		$(VERILOG) $(VERILOG_FLAGS) $(TOP) covered_vpi.v +covered_cdd=$(DIAG).cdd $(SIM_PLUSARGS); \
	fi
else
	$(VERILOG) $(VERILOG_FLAGS) $(TOP) covered_vpi.v
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd +covered_debug $(SIM_PLUSARGS); \
	else \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd $(SIM_PLUSARGS); \
	fi
endif
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
else
ifdef INLINE
	rm -rf covered
ifeq ($(dir $(TOP)),covered/verilog/)
ifndef LXT
ifdef USE_VERILATOR
	$(VERILOG) $(VERILOG_FLAGS) $(DIAG).verilator.v
	cp $(DIAG).cdd $(DIAG).keep.cdd
else
	$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
endif
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_PRECMD)
endif
endif
	@mv $(DIAG).vcd $(DIAG).vcd.1
ifdef USE_VERILATOR
	@rm -rf $(DIAG)
endif
endif
endif
	@./convert_cfg inline 0 $(VERILATOR_SET) $(DIAG).cfg
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
	@./convert_cfg $(CFG_CMD) 1 $(VERILATOR_SET) $(DIAG).cfg
else
	@./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $(DIAG).cfg
endif
	$(VERILOG) $(VERILOG_FLAGS) $(TOP)
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_CMD)
endif
endif
ifdef INLINE
ifeq ($(dir $(TOP)),covered/verilog/)
ifndef LXT
	./vcd_diff $(VCDDIFF_VERBOSE) $(DIAG).vcd $(DIAG).vcd.1 $(DIAG).map
	@rm -f $(DIAG).vcd.1
endif
endif
endif
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
endif
#	@rm -f $(DIAG).cfg
ifdef USE_VERILATOR
	@mv $(DIAG).cdd  $(DIAG).verilator.cdd
	@mv $(DIAG).rptM $(DIAG).verilator.rptM
	@mv $(DIAG).rptI $(DIAG).verilator.rptI
	@./check_test $(DIAG).verilator 1 $(CHK_CODE)
	@rm -rf $(DIAG)
else
	@./check_test $(DIAG) 1 $(CHK_CODE)
endif

$(DIAGLIST): check-ovl
ifdef USE_VERILATOR
	@if test ! -f $@.verilator.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ TOP=$(TOPDIR)$@.verilator.v diagrun; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi
else
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ TOP=$(TOPDIR)$@.v diagrun; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi
endif

diag2run:
	@echo Running $(DIAG)
	@./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $(DIAG).cfg
ifdef VPI
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
ifdef USE_CVER
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v +covered_cdd=$(DIAG).cdd +covered_debug $(SIM_PLUSARGS); \
	else \
		$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v +covered_cdd=$(DIAG).cdd $(SIM_PLUSARGS); \
	fi
else
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd +covered_debug $(SIM_PLUSARGS); \
	else \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd $(SIM_PLUSARGS); \
	fi
endif
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -b -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -i -b -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -b -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -i -b -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
else
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_CMD)
endif
endif   
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -b -e -d v -m ltcfam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -b -e -d v -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -b -e -d v -m ltcefam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -b -e -d v -m ltcefam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
endif
	@rm -f $(DIAG).cfg
	@./check_test $(DIAG) 1 $(CHK_CODE)

$(DIAG2LIST): check-ovl
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ diag2run; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

racerun:
	@echo Running $(DIAG)
	@./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $(DIAG).cfg
ifdef VPI
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
ifdef USE_CVER
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v +covered_cdd=$(DIAG).cdd +covered_debug $(SIM_PLUSARGS); \
	else \
		$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v +covered_cdd=$(DIAG).cdd $(SIM_PLUSARGS); \
	fi
else
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd +covered_debug $(SIM_PLUSARGS); \
	else \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd $(SIM_PLUSARGS); \
	fi
endif
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfamr -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfamr -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefamr -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefamr -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
else
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_CMD)
endif
endif
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfamr -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfamr -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefamr -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefamr -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
endif
	@rm -f $(DIAG).cfg
	@./check_test $(DIAG) 1 $(CHK_CODE)

$(RACELIST):
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ racerun; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

onemergerun:
	@./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $(DIAG).cfg
ifdef VPI
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
ifdef USE_CVER
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v +covered_cdd=$(DIAG).cdd +covered_debug $(SIM_PLUSARGS); \
	else \
		$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v +covered_cdd=$(DIAG).cdd $(SIM_PLUSARGS); \
	fi
else
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v covered_vpi.v
	@if test "$(COVERED_GFLAG)" = "-D"; then \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd +covered_debug $(SIM_PLUSARGS); \
	else \
		$(VERILOG_CMD) +covered_cdd=$(DIAG).cdd $(SIM_PLUSARGS); \
	fi
endif
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
else
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_CMD)
endif
endif
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
endif
	@rm -f $(DIAG).cfg
	@./check_test $(DIAG) 0 $(CHK_CODE)
	
mergerun:
	@echo Running $(DIAG)
	@$(MAKE) DIAG=$(DIAG)a onemergerun
	@$(MAKE) DIAG=$(DIAG)b onemergerun
	$(COVERED) $(COVERED_GFLAG) merge -o $(DIAG).cdd $(DIAG)a.cdd $(DIAG)b.cdd $(CHECKMEM)
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
	@./check_test $(DIAG) 3 $(CHK_CODE)

$(MERGELIST):
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ mergerun; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 3 2; \
	fi

merge3run:
	@$(MAKE) DIAG=$(DIAG)a onemergerun
	@$(MAKE) DIAG=$(DIAG)b onemergerun
	@$(MAKE) DIAG=$(DIAG)c onemergerun
	$(COVERED) $(COVERED_GFLAG) merge -o $(DIAG).cdd $(DIAG)a.cdd $(DIAG)b.cdd $(DIAG)c.cdd $(CHECKMEM)
ifdef USE_VERILATOR
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcfam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
else
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -o $(DIAG).rptM $(DIAG).cdd $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e -m ltcefam -i -o $(DIAG).rptI $(DIAG).cdd $(CHECKMEM)
endif
	@./check_test $(DIAG) 4 $(CHK_CODE)

$(MERGE3LIST):
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ merge3run; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 4 2; \
	fi

errscorerun:
ifndef VPI
	@echo Running $(DIAG) -- should see an error message
	@./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $(DIAG).cfg
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_CMD)
endif
endif
	-$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP 2> $(DIAG).err $(CHECKMEM)
	@rm -f $(DIAG).cfg
	@cat $(DIAG).err
	@./check_test $(DIAG) 1 1
endif

$(ERRSCORELIST):
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ errscorerun; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

oneerrmergerun:
ifndef VPI
	@./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $(DIAG).cfg
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_CMD)
endif
endif
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
	@rm -f $(DIAG).cfg
endif

errmergerun:
ifndef VPI
	@echo Running $(DIAG) -- should see an error message
	@$(MAKE) DIAG=$(DIAG)a oneerrmergerun
	$(COVERED) $(COVERED_GFLAG) merge -o $(DIAG).cdd $(DIAG)a.cdd $(DIAG)b.cdd.bad 2> $(DIAG).err $(CHECKMEM)
	@cat $(DIAG).err
	@./check_test $(DIAG) 2 1
endif

$(ERRMERGELIST):
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ errmergerun; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 2 2; \
	fi

errreportrun:
ifndef VPI
	@echo Running $(DIAG) -- should see an error message
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_CMD)
endif
endif
	@./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $(DIAG).cfg
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) report -d v -e $(DIAG).cdd 2> $(DIAG).err $(CHECKMEM)
	@rm -f $(DIAG).cfg
	@cat $(DIAG).err
	@./check_test $(DIAG) 1 1
endif

$(ERRREPORTLIST):
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ errreportrun; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(ERRRANKLIST):
	@if test ! -f $@.done; then \
		if test -f ../regress/$@.pl; then \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

errexcluderun:
ifndef VPI
	@echo Running $(DIAG) -- should see an error message
	@$(VERILOG) $(VERILOG_FLAGS) $(DIAG).v
ifndef USE_CVER
ifndef USE_VERIWELL
	@$(VERILOG_CMD)
endif
endif
	@./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $(DIAG).cfg
	$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $(DIAG).cfg -D DUMP $(CHECKMEM)
	$(COVERED) $(COVERED_GFLAG) exclude -p $(DIAG).cdd 2> $(DIAG).err $(CHECKMEM)
	@rm -f $(DIAG).cfg
	@cat $(DIAG).err
	@./check_test $(DIAG) 1 1
endif

$(ERREXCLUDELIST):
	@if test ! -f $@.done; then \
		if test ! -f ../regress/$@.pl; then \
			$(MAKE) DIAG=$@ errexcluderun; \
		else \
			perl ../regress/$@.pl $(MAKEFLAGS); \
		fi; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

$(NOSIMLIST):
	@if test ! -f $@.done; then \
		echo Running $@; \
		./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $@.cfg 2> $@.err $(CHECKMEM); \
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
		./convert_cfg $(CFG_CMD) 0 $(VERILATOR_SET) $@.cfg; \
		$(COVERED) $(COVERED_GFLAG) $(COVERED_SCORE_GFLAG) score -f $@.cfg 2> $@.err $(CHECKMEM); \
		rm -f $@.cfg $@.cdd; \
		cat $@.err; \
		./check_test $@ 1 1; \
	else \
		echo Skipping $@; \
		./check_test $@ 1 2; \
	fi

covered:
ifdef USE_VERILATOR
	cd $(COVERED_RUNLIB); $(MAKE)
endif
ifdef VPI
	cd $(COVERED_VPILIB); $(MAKE)
endif
	cd $(COVERED_SRC); $(MAKE)

eot:
	@./check_test dummy 1 $(FINISH_CODE)

view_vg_errs:
	@./view_vg_errs $(VG_LOG_PREFIX)
	
reset:  clobber
	@rm -f *.done

clobber:  clean
	@rm -f *.rpt* *.cdd *.err regress.output regress.failed
	@rm -f $(VG_LOG_PREFIX).*
	@rm -rf covered

clean:
	@rm -f $(LOGFILE)
	@rm -f $(IVERILOG_EXEC) $(VCS_EXEC) *.vcd *.log *.cfg
	@rm -f core *.o
	@rm -rf $(VCS_EXEC).*
	@rm -f tmp*
