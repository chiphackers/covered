######################################################
# Written by:   Trevor Williams  (trevorw@charter.net)
#
# Date:         8/26/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

IVERILOG        = iverilog
IVERILOG_FLAGS1 = -y ./lib -I ./include -DRUNTEST
IVERILOG_FLAGS2 = -y ./lib -I ./include -DRUNTEST -Tmin
IVERILOG_FLAGS3 = -y ./lib -I ./include -DRUNTEST -Ttyp
IVERILOG_FLAGS4 = -y ./lib -I ./include -DRUNTEST -Tmax
IVERILOG_EXEC   = a.out

VCS       = vcs
VCS_FLAGS = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS
VCS_EXEC  = simv

CDD_DIR       = ../cdd

LOGFILE = regress.log

DIAGS1       = always1        always2        always3        always3.1.11A  always4 \
               always5        always6        assign3.2A     case1          case1.1 \
               case1.2        case1.3        case2          case3          case3.1 \
               case3.2        case3.3        case3.4        casex1         casex1.1 \
               casex1.2       casex1.3       casex1.4       casez1         casez1.1 \
               casez1.2       casez1.3       casez1.4       casez2         concat1 \
               concat2        concat3        concat4.1      concat4.2      cond1 \
               cond2          delay1         define1        define1.1      define2 \
               define2.1      expand1        expand2        expand3        ifdef1 \
               ifdef1.1       ifdef1.2       ifdef2.1       ifdef2.2       ifdef2.3 \
               ifelse1        include1       include1.1     include2       instance1 \
               instance1.1    instance1.2    instance2      instance3      instance4 \
               instance4.1    instance4.2    long_exp1      mbit_sel1      mbit_sel2 \
               mbit_sel3      mbit_sel3.1    mult1          param1         param1.1 \
               param1.2       param1.3       param3         param3.1       param3.2 \
               param3.3       param4         param5         param6         real1.1 \
               sbit_sel1      static1        static2        static2.1      static2.2 \
               supply0        supply1        tri            tri0           tri1
DIAGS2       = delay1.1
DIAGS3       = delay1.2
DIAGS4       = delay1.3
           
IVONLYDIAGS1 = always5.1      assign1        implicit1
IVONLYDIAGS2 = 
IVONLYDIAGS3 = 
IVONLYDIAGS4 = 

VCSONLYDIAGS = real1          triand         trior          trireg         wand \
               wor

iverilog:
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS1)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS1) $(IVONLYDIAGS1)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS2)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS2) $(IVONLYDIAGS2)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS3)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS3) $(IVONLYDIAGS3)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS4)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS4) $(IVONLYDIAGS4)" regress
	@echo REGRESSION PASSED!

vcs:
	@cd ../verilog; \
	$(MAKE) VERILOG=$(VCS) VERILOG_FLAGS="$(VCS_FLAGS)" VERILOG_EXEC=$(VCS_EXEC) DIAGLIST="$(DIAGS) $(VCSONLYDIAGS)" regress
	@echo REGRESSION PASSED!

clean:
	cd ../verilog;  $(MAKE) clean
