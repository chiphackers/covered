######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         8/26/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

IVERILOG       = iverilog
IVERILOG_FLAGS = -y ./lib -I ./include -DRUNTEST
IVERILOG_EXEC  = a.out

VCS       = vcs
VCS_FLAGS = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS
VCS_EXEC  = simv

CDD_DIR       = ../cdd

LOGFILE = regress.log

DIAGS    = assign3.2A    always1       always3.1.11A  cond1 \
	   cond2         mult1         always2       always3        always4 \
           always5       case1         case1.1       case2          case3 \
           case3.1       case3.2       case3.3       case3.4        casex1 \
	   casex1.1      casex1.2      casex1.3      casex1.4       casez1 \
	   casez1.1      casez1.2      casez1.3      casez1.4       casez2 \
           ifelse1       define1       define1.1     define2        define2.1 \
           static1       include1      include1.1    ifdef1         ifdef2.1 \
           ifdef2.2      concat1       concat2       expand1        expand2 \
           sbit_sel1     instance1     instance2     instance3      mbit_sel1 \
           param1        concat3       concat4.1     concat4.2      param1.1 \
           param3        param3.1      param3.2      param1.2       always6 \
           param4        param5        include2      mbit_sel2      instance1.1

IVONLYDIAGS = assign1    implicit1

iverilog:
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS) $(IVONLYDIAGS)" regress

vcs:
	@cd ../verilog; \
	$(MAKE) VERILOG=$(VCS) VERILOG_FLAGS="$(VCS_FLAGS)" VERILOG_EXEC=$(VCS_EXEC) DIAGLIST="$(DIAGS)" regress

clean:
	cd ../verilog;  $(MAKE) clean
