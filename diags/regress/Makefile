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

VCS        = vcs
VCS_FLAGS1 = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS
VCS_FLAGS2 = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS +mindelays
VCS_FLAGS3 = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS +typdelays
VCS_FLAGS4 = +libext+.v+ -y ./lib +incdir+./include +define+RUNTEST +define+VCS +maxdelays
VCS_EXEC   = simv

CDD_DIR       = ../cdd

LOGFILE = regress.log

DIAGS1        = always1        always2        always3        always3.1.11A  always4 \
                always5        always6        always7        always8        always9 \
                always10       always11       always11.1     always11.2     always12 \
                always13       assign2 \
                assign2.1      assign2.2      assign2.3      assign2.9      assign3.2A \
                block1         case1          case1.1        case1.2        case1.3 \
                case2          case3          case3.1        case3.2        case3.3 \
                case3.4        case4          case4.1        casex1         casex1.1 \
                casex1.2       casex1.3       casex1.4       casez1         casez1.1 \
                casez1.2       casez1.3       casez1.4       casez2         concat1 \
                concat2        concat3        concat4.1      concat4.2      concat5 \
                concat6        cond1          cond2          delay1         delay2 \
                define1        define1.1      define1.2      define2        define2.1 \
                define3        define4        expand1        expand2        expand3 \
                expand4        expand4.1      expand4.2      expand4.3      expand4.4 \
                fsm1           fsm1.1         fsm1.2         fsm1.3         fsm1.4 \
                fsm3           fsm4           fsm4.1         fsm5           fsm5.1 \
                fsm5.2         fsm5.3         fsm10          gate1          hier1 \
                ifdef1         ifdef1.2       ifdef2.1       ifdef2.2       ifdef2.3 \
                ifelse1        include1       include1.1     include2       instance1 \
                instance1.1    instance1.2    instance2      instance3      instance4 \
                instance4.1    instance4.2    instance5      instance6      instance6.1 \
                instance6.2    instance7      long_exp1      long_exp2      long_exp3 \
                long_sig       mbit_sel1      mbit_sel2      mbit_sel3      mbit_sel3.1 \
                mbit_sel4      mbit_sel4.1    mbit_sel4.2    mbit_sel4.3    mem1 \
                mult1          multi_exp1     multi_exp1.1   multi_exp1.2   multi_exp1.3 \
                multi_exp1.4   multi_exp1.5   multi_exp1.6   multi_exp1.7   multi_exp1.8 \
                multi_exp1.9   multi_exp1.10  multi_exp1.11  multi_exp1.12  multi_exp1.13 \
                multi_exp1.14  multi_exp1.15  multi_exp2     multi_exp2.1   multi_exp3 \
                param1         param1.1       param1.2       param1.3       param3 \
                param3.1       param3.2       param3.3       param3.4       param3.5 \
                param4         param4.1       param5         param6         param6.1 \
                param7         pulldown1      pullup1        race1          race1.1 \
                race1.2        sbit_sel1 \
                sbit_sel1.1    static1        string1        supply0        supply1 \
                toggle         tri            tri0           tri1
DIAGS2        = delay1.1
DIAGS3        = delay1.2
DIAGS4        = delay1.3

MERGE1        = merge1         fsm2

ERRSCORE      = err1.0         err2.0
ERRMERGE      =
ERRREPORT     = err2.1
           
IVONLYDIAGS1  = always5.1      assign1        assign2.8      example        fsm6 \
                fsm7           fsm7.1         fsm7.2         fsm7.3         fsm7.4 \
                fsm7.5         fsm8           fsm8.1         fsm8.2         fsm8.3 \
                fsm9           fsm9.1         fsm9.2         fsm10.1        fsm10.2 \
                ifdef1.1       implicit1      implicit2      real1.1        static2 \
                static2.1      static2.2      unary1
IVONLYDIAGS2  = 
IVONLYDIAGS3  = 
IVONLYDIAGS4  = 

VCSONLYDIAGS1 = always7.1      assign2.4      assign2.5      assign2.6      assign2.7 \
                event1         gate1.1        implicit2.1    real1          triand \
                trior          trireg         wand           wor
VCSONLYDIAGS2 = 
VCSONLYDIAGS3 = 
VCSONLYDIAGS4 = 

iverilog:    clean
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS1)" VERILOG_CMD="vvp $(IVERILOG_EXEC)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS1) $(IVONLYDIAGS1)" MERGELIST="$(MERGE1)" ERRSCORELIST="$(ERRSCORE)" ERRMERGELIST="$(ERRMERGE)" ERRREPORTLIST="$(ERRREPORT)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS2)" VERILOG_CMD="vvp $(IVERILOG_EXEC)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS2) $(IVONLYDIAGS2)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS3)" VERILOG_CMD="vvp $(IVERILOG_EXEC)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS3) $(IVONLYDIAGS3)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(IVERILOG) VERILOG_FLAGS="$(IVERILOG_FLAGS4)" VERILOG_CMD="vvp $(IVERILOG_EXEC)" VERILOG_EXEC=$(IVERILOG_EXEC) DIAGLIST="$(DIAGS4) $(IVONLYDIAGS4)" regress
	@echo
	@cat ../verilog/regress.output
	@echo
	@cd ../verilog;  $(MAKE) clean

vcs:         clean
	@cd ../verilog; \
	$(MAKE) VERILOG=$(VCS) VERILOG_FLAGS="$(VCS_FLAGS1)" VERILOG_CMD=$(VCS_EXEC) VERILOG_EXEC=$(VCS_EXEC) DIAGLIST="$(DIAGS1) $(VCSONLYDIAGS1)" MERGELIST="$(MERGE1)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(VCS) VERILOG_FLAGS="$(VCS_FLAGS2)" VERILOG_CMD=$(VCS_EXEC) VERILOG_EXEC=$(VCS_EXEC) DIAGLIST="$(DIAGS2) $(VCSONLYDIAGS2)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(VCS) VERILOG_FLAGS="$(VCS_FLAGS3)" VERILOG_CMD=$(VCS_EXEC) VERILOG_EXEC=$(VCS_EXEC) DIAGLIST="$(DIAGS3) $(VCSONLYDIAGS3)" regress
	@cd ../verilog; \
	$(MAKE) VERILOG=$(VCS) VERILOG_FLAGS="$(VCS_FLAGS4)" VERILOG_CMD=$(VCS_EXEC) VERILOG_EXEC=$(VCS_EXEC) DIAGLIST="$(DIAGS4) $(VCSONLYDIAGS4)" regress
	@echo
	@cat ../verilog/regress.output
	@echo
	@cd ../verilog;  $(MAKE) clean

clean:
	@cd ../verilog;  $(MAKE) clobber
