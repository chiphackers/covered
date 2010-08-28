######################################################
# Written by:   Trevor Williams  (phase1geo@gmail.com)
#
# Date:         8/26/2002
#
# Purpose:      Runs regression suite for Verilog
######################################################

IVERILOG_FLAGS1 = 
IVERILOG_FLAGS2 = -Tmin
IVERILOG_FLAGS3 = -Ttyp
IVERILOG_FLAGS4 = -Tmax

VCS_FLAGS1 = 
VCS_FLAGS2 = +mindelays
VCS_FLAGS3 = +typdelays
VCS_FLAGS4 = +maxdelays

CVER_FLAGS1 =
CVER_FLAGS2 = +mindelays
CVER_FLAGS3 = +typdelays
CVER_FLAGS4 = +maxdelays

VW_FLAGS1 =
VW_FLAGS2 = +mindelays
VW_FLAGS3 = +typdelays
VW_FLAGS4 = +maxdelays

VCOMP_FLAGS1 =
VCOMP_FLAGS2 = +mindelays
VCOMP_FLAGS3 = +typdelays
VCOMP_FLAGS4 = +maxdelays

CDD_DIR       = ../cdd

LOGFILE = regress.log

DIAGS1        = add1              aedge1            aedge1.1          afunc1            afunc2 \
                always1           always2           always3           always3.1.11A     always4 \
                always5           always5.1         always6           always7           always7.1 \
                always8           always9           always10          always11          always11.1 \
                always11.2        always12          always13          always15          always_comb1 \
                always_comb2      always_ff1        always_latch1     always_latch2     ashift1 \
                ashift2           ashift3           ashift4           assert1           assign1 \
                assign2           assign2.1         assign2.2         assign2.3         assign2.4 \
                assign2.5         assign2.6         assign2.7         assign2.8         assign2.9 \
                assign3.2A        assign5           atask1            bassign1          bassign2 \
                bassign3          block1            bit1              bug2794588        byte1 \
                case1             case1.1           case1.2           case1.3           case2 \
                case3             case3.1           case3.2           case3.3           case3.4 \
                case4             case4.1           case5             case6             casex1 \
                casex1.1          casex1.2          casex1.3          casex1.4          casex2 \
                casex3            casez1            casez1.1          casez1.2          casez1.3 \
                casez1.4          casez2            casez3            casez4            casez5 \
                ceq1              char1             clog2             clog2.1           clog2.2 \
                clog2.3           clog2.4           clog2.5           cne1              compx1 \
                compx1.1          compx1.2          compx1.3          concat1           concat2 \
                concat3           concat4.1         concat4.2         concat5           concat6 \
                concat7           concat8           concat9           concat10          concat10.1 \
                concat10.2        cond1             cond2             cond3             cond3.1 \
                cond3.2           cond3.3           dec1              dec1.1            dec2 \
                dec3              dec3.1            delay1            delay2            define1 \
                define1.1         define1.2         define2           define2.1         define3 \
                define4           define5           define6           define6.1         define7 \
                delay3            disable1          disable1.1        disable2          disable2.1 \
                disable3          display1          div1              dly_assign1       dly_assign1.1 \
                dly_assign1.2     dly_assign1.3     dly_assign1.4     dly_assign2       dly_assign2.1 \
                do_while1         do_while1.1       do_while2         elsif1            elsif1.1 \
                elsif1.2          elsif1.3          elsif1.4          elsif2            elsif2.1 \
                elsif3            endian1           endian2           endian3 \
                endian4           enum1             enum1.1           enum1.2           enum1.3 \
                enum2.1           event1            event1.1          event2            example \
                example1          exclude1          exclude2          exclude3          exclude4 \
                exclude5          exclude5.1        exclude5.2        exclude5.3        exclude6 \
                exclude6.1        exclude7          exclude7.1        exclude7.2        exclude7.3 \
                exclude8          exclude8.1        exclude9          exclude9.1        exclude9.2 \
                exclude9.3        exclude9.4        exclude9.5        exclude9.6        exclude9.7 \
                exclude9.8        exclude9.9        exclude10         exclude10.1       exclude10.2 \
                exclude10.3       exclude10.3.1     exclude10.3.2     exclude10.3.3     exclude10.4 \
                exclude10.5       exclude11         exclude12         exclude12.1       exclude12.2 \
                exclude12.3       exclude12.4       exclude12.5       exclude12.6       exclude13 \
                exclude14         exclude15         expand1           expand2           expand3 \
                expand4           expand4.1         expand4.2         expand4.3         expand4.4 \
                expand6           expand6.1         expand6.2         expand6.3         exponent1 \
                final1            fdisplay1         finish1           fmonitor1 \
                for1              for2              for3              for5              for5.1 \
                for5.2            for5.3            for5.4            for5.5            for5.6 \
                for5.8            for5.9            for5.10           for6              forever1 \
                fork1             fork2             fsm1              fsm1.1            fsm1.2 \
                fsm1.3            fsm1.4            fsm3              fsm4              fsm4.1 \
                fsm5              fsm5.1            fsm5.2            fsm5.3            fsm6 \
                fsm7              fsm7.1            fsm7.2            fsm7.3            fsm7.4 \
                fsm7.5            fsm8              fsm8.1            fsm8.2            fsm8.3 \
                fsm9              fsm9.1            fsm9.2            fsm10             fsm10.1 \
                fsm10.2           fsm11             fsm12             fsm13             fstrobe1 \
                func1             func2 \
                func5             fwrite1           gate1             gate1.1           generate1 \
                generate1.1       generate2         generate2.1       generate3         generate3.1 \
                generate4         generate4.1       generate5         generate5.1       generate5.2 \
                generate5.3       generate6         generate7         generate7.1       generate7.2 \
                generate8         generate8.1       generate8.2       generate8.3       generate8.4 \
                generate8.5       generate8.6       generate8.7       generate8.8       generate8.9 \
                generate9         generate9.1       generate9.2       generate9.3       generate10 \
                generate10.1      generate10.2      generate11        generate11.1      generate11.2 \
                generate11.3      generate11.4      generate12        generate13        generate14 \
                generate15        generate15.1      generate15.2      generate15.3      generate15.4 \
                generate15.5      generate16        generate16.1      generate16.2      generate16.3 \
                generate17        generate18        generate19        generate19.1      generate19.2 \
                generate21        generate22        generate24        hier1             hier1.1 \
                hier2             hier3 \
                hier3.1           hier3.2           hier3.3           hier3.4           hier4 \
                if1               ifdef1            ifdef1.1          ifdef1.2          ifdef2.1 \
                ifdef2.2          ifdef2.3          ifdef3            ifelse1           ifndef1 \
                ifndef2           implicit2         implicit2.1       inc1              inc1.1 \
                inc2              inc3              inc3.1            include1          include1.1 \
                include2          include4          include5          initial1          inline1 \
                inline1.1         inline1.2         inline1.3         inline1.4         inline1.5 \
                inline2           inline3           instance1         instance1.1       instance1.2       instance2 \
                instance3         instance4         instance4.1       instance4.2       instance5 \
                instance6         instance6.1       instance6.2       instance7         int1 \
                line1             localparam1       localparam1.1     localparam1.2     localparam1.3 \
                localparam2       localparam3       logic1            long_exp1 \
                long_exp2         long_exp3         longint1          long_sig          lshift1 \
                lshift1.1         lshift1.2         lshift2           lshift2.1         lshift2.2 \
                lshift2.3         lshift3           lshift3.1         lshift3.2         lshift3.3 \
                lshift4           lshift4.1         lshift4.2         lshift4.3         lshift5 \
                marray1           marray1.1         marray1.2         marray1.3         marray2 \
                marray2.1         marray2.2         marray2.3         mbit_sel1         mbit_sel1.1 \
                mbit_sel2         mbit_sel3         mbit_sel3.1       mbit_sel4         mbit_sel4.1 \
                mbit_sel4.2       mbit_sel4.3       mbit_sel5         mbit_sel5.1       mbit_sel5.2 \
                mbit_sel5.3       mbit_sel5.4       mbit_sel5.5       mbit_sel6         mbit_sel6.1 \
                mbit_sel6.2       mbit_sel6.3       mbit_sel6.4       mbit_sel6.5       mbit_sel7 \
                mbit_sel9         mbit_sel9.1       mbit_sel9.2       mem1              mem2 \
                mem3              mem4              mem4.1            mem5              mem6 \
                mem6.1            mem6.2            mem6.3            mem6.4 \
                mod1              monitor1          monitor1.1        msg1              mult1 \
                mult2             multi_exp1        multi_exp1.1      multi_exp1.2      multi_exp1.3 \
                multi_exp1.4      multi_exp1.5      multi_exp1.6      multi_exp1.7      multi_exp1.8 \
                multi_exp1.9      multi_exp1.10     multi_exp1.11     multi_exp1.12     multi_exp1.13 \
                multi_exp1.14     multi_exp1.15     multi_exp2        multi_exp2.1      multi_exp3 \
                multi_exp4        named_block1      named_block2      named_block3      named_block4 \
                nand1             negate1           negate2           nedge1            nested_block1 \
                nested_block2     nested_block2.1   nested_block3     nor1              not1 \
                null_stmt1        null_stmt1.1      null_stmt1.2      null_stmt1.3      null_stmt1.5 \
                null_stmt1.6      null_stmt1.7      null_stmt1.8      null_stmt1.9      null_stmt1.10 \
                null_stmt2        null_stmt2.1      op_assign1        ovl1              ovl1.1            ovl1.2 \
                ovl1.3            param1            param1.1          param1.2          param1.3 \
                param3            param3.1          param3.2          param3.3          param3.4 \
                param3.5          param4            param4.1          param5            param6 \
                param6.1          param7            param8            param8.1          param8.2 \
                param9            param9.1          param9.2          param10           param10.1 \
                param10.2         param10.3         param10.4         param10.5         param11 \
                param12           param12.1         param13           param13.1         param14 \
                param15           param16           param17           param17.1         pedge1 \
                port1             port2             port3             port4             printtimescale1 \
                profile_err1      pulldown1         pullup1           random1           random1.1 \
                random1.2         random1.3         random2           random2.1   \
                random3           random3.1         random4           rassign1          rassign2 \
                real1             real1.1           real2             real2.1           real3 \
                real3.1           real3.2           real3.3           real3.4           real3.5 \
                real3.6           real3.7           real3.8           real5             real5.1 \
                real5.2           real5.3           real5.4           real5.5           real5.6 \
                real5.7           real5.8           real5.9           real5.10          real5.11 \
                real5.12          real5.13          real5.14          real5.15          real5.16 \
                real6             real6.1           real6.2           real6.3           real6.4 \
                real6.5           real6.6           real6.7           real6.8           real6.9 \
                real6.10          real6.11          real6.12          real6.13          real7 \
                real8             repeat1           repeat2           repeat3           report1 \
                report2           report3           report3.1         report4           report4.1 \
                report4.2         report5           report6           report7 \
                report7.1         report8           rshift1           rshift1.1         rshift1.2 \
                rshift1.3         rshift2           rshift2.1         rshift2.2         rshift2.3 \
                rshift3           rshift3.1         rshift4           rshift4.1         rshift4.2 \
                rshift5           sbit_sel1         sbit_sel1.1       sbit_sel2         sbit_sel2.1 \
                sbit_sel3         sbit_sel3.1       sbit_sel3.2       sbit_sel4         sbit_sel4.1 \
                sbit_sel4.2       sbit_sel4.3       score_env1        score_ts1         shortint1 \
                signed1           signed3           signed3.1         signed3.2         signed3.3 \
                signed3.4         signed3.5         signed3.6         signed4           signed4.1 \
                signed4.2         signed5           signed6           slist1            slist1.1 \
                slist1.2          slist1.3          slist2            slist3            slist3.1 \
                slist3.2          slist3.3          slist4            slist6            static1 \
                static2           static2.1         static2.2         static3           static_afunc1 \
                static_func1      static_func1.1    static_func2      static_func2.1    static_func2.2 \
                static_func2.3    static_func3      static_func4      string1           string2 \
                string2.1         strobe1           supply0           supply1           task1 \
                task1.1           task2             task2.1           task2.2           task2.3 \
                task3             task3.1           task4             task4.1           task6 \
                test_plusargs1    time1             time1.1           time2             timeformat1 \
                timescale1        timescale1.1      timescale1.2      timescale1.3      timescale1.4 \
                timescale1.5      timescale1.6      timescale1.7      timescale1.8      timescale1.9 \
                timescale1.10     timescale1.11     timescale1.12     timescale1.13     timescale1.14 \
                timescale1.15     timescale1.16     timescale1.17     timescale1.18     timescale1.19 \
                timescale1.20     timescale2        timescale2.1      timescale2.2      timescale2.3 \
                timescale2.4      timescale2.5      timescale2.6      timescale2.7      timescale2.8 \
                timescale4        timescale4.1      timescale4.2      timescale4.3      timescale4.4 \
                timescale4.5      timescale4.6      timescale4.7      timescale4.8      timescale4.9 \
                timescale4.10     timescale4.11     timescale4.12     timescale4.13     timescale4.14 \
                timescale4.16     timescale4.17     timescale4.18     timescale4.19     timescale4.20 \
                timescale4.21     toggle            toggle2           toggle2.1         toggle2.2 \
                toggle2.3         tri               tri0              tri1              triand \
                trigger1          trior             trireg            unary1            unary2 \
                unary3            unsigned1         urandom1          urandom1.1        urandom1.2 \
                urandom1.3        urandom2          urandom2.1        urandom3          urandom3.1 \
                urandom_range1    urandom_range1.1  urandom_range1.2  urandom_range2    urandom_range2.1 \
                urandom_range2.2  urandom_range3    value_plusargs1   value_plusargs2   value_plusargs3 \
                value_plusargs4   value_plusargs5   value_plusargs6   value_plusargs7   value_plusargs8 \
                value_plusargs8.1 version1          version1.1        wait1             wait1.1 \
                wait2             wand              while1            while1.1          wor \
                write1

DIAGS2        = delay1.1
DIAGS3        = delay1.2
DIAGS4        = delay1.3

BDIAGS1       = bitwise1

MERGE1        = merge1            merge3            merge4            merge5            merge6 \
                merge6.1          merge7            merge7.1          merge7.2          merge7.3 \
                merge7.4          merge7.5          merge7.6          merge7.7          merge7.8 \
                merge8            merge8.1          merge8.2          merge8.3          merge8.6 \
                merge8.7          merge8.8          merge8.9          merge8.10         merge8.11 \
                merge9            merge10           merge11           merge12           fsm2 \
                rank1             rank1.1

MERGE3        = merge2            merge8.4          merge8.5

RACELIST      = race1             race1.1           race1.2           race2.1           race2.2 \
                race2.3           race2.4           race4             race5             race5.1 \
                race5.2           race6             race7             race7.1           race7.2 \
                race8

ERRSCORE      = err1.0            err2.0            err3              err3.1            err3.2 \
                err5              err5.1            err6              err6.1            err6.2 \
                err6.3            err6.4            func4             func4.1           func4.2 \
                func4.3           func4.4           func4.5           func4.6           inlined_err1 \
                score_err1        score_err1.1      score_err1.2      score_err1.3      score_err1.15 \
                score_err1.31     score_help

ERRMERGE      = err5.2            err5.3            err5.4            merge_err1        merge_err1.1 \
                merge_err1.2      merge_err1.3      merge_err2        merge_err2.1      merge_help

ERRREPORT     = err7              err7.1            err8              err8.1            err8.2 \
                err9              report_err1       report_err1.1     report_err1.2 \
                report_err1.3     report_err1.4     report_err1.5     report_err1.6     report_err1.7 \
                report_err1.8     report_help

ERRRANK       = rank_err1         rank_err1.1       rank_err2         rank_err2.1       rank_err3 \
                rank_err4         rank_err4.1       rank_err4.2       rank_err4.3       rank_err5 \
                rank_err5.1       rank_err6         rank_err7         rank_err7.1       rank_err8 \
                rank_err8.1       rank_err9         rank_err9.1       rank_err10        rank_err10.1 \
                rank_err11        rank_err11.1      rank_err12        rank_err12.1      rank_help

ERREXCLUDE    = exclude_err1      exclude_err1.1    exclude_err1.2    exclude_err2      exclude_err3 \
                exclude_err3.1    exclude_help

NOSIMLIST     = gen1              gen1.1            gen1.3            gen1.4            gen1.5 \
                gen2.1            gen3.1            gen4.1            gen5.1            gen6.1 \
                gen7.1            gen8.1            gen9.1            gen10.1           gen11.1 \
                gen12.1           gen13.1           gen14.1           gen15.1           gen16.1 \
                gen17.1           gen18.1           gen19             gen19.1           gen20 \
                gen20.1           gen21             gen21.1           gen22             gen22.1 \
                gen23             gen23.1           gen24             gen24.1           gen25.2 \
                gen26.2           gen27.2           gen28.2           gen29.2           gen30.2 \
                gen31.2           gen32.2           gen33.2           gen34.2           gen35.2 \
                gen36.2           gen37.2           gen38.2           gen39.2           gen40.2

NOSIMERRLIST  = bug2791651        clog2.6           clog2.7           comment1          enum2 \
                err4              err4.1            expand5           for7              for7.1 \
                fsm_err1          func3             func3.1           func3.2           func3.3 \
                func3.4           func3.5           func3.6           gen1.2            gen2 \
                gen3              gen4              gen5              gen6              gen7 \
                gen8              gen9              gen10             gen11             gen12 \
                gen13             gen14             gen15             gen16             gen17 \
                gen18             gen19.2           gen20.2           gen21.2           gen22.2 \
                gen23.2           gen24.2           gen25             gen25.1           gen26 \
                gen26.1           gen27             gen27.1           gen28             gen28.1 \
                gen29             gen29.1           gen30             gen30.1           gen31 \
                gen31.1           gen32             gen32.1           gen33             gen33.1 \
                gen34             gen34.1           gen35             gen35.1           gen36 \
                gen36.1           gen37             gen37.1           gen38             gen38.1 \
                gen39             gen39.1           gen40             gen40.1           generate20 \
                generate20.1      include_err       score_env2        score_err1.4      score_err1.5 \
                score_err1.6      score_err1.7      score_err1.8      score_err1.9      score_err1.10 \
                score_err1.11     score_err1.12     score_err1.13     score_err1.14     score_err1.16 \
                score_err1.17     score_err1.18     score_err1.19     score_err1.20     score_err1.21 \
                score_err1.22     score_err1.23     score_err1.24     score_err1.25     score_err1.26 \
                score_err1.27     score_err1.28     score_err1.29     score_err1.30     task5 \
                timescale3        timescale3.1      timescale3.2      timescale3.3      timescale3.4 \
                timescale3.5      timescale3.6      timescale3.7      timescale3.8      timescale3.9 \
                timescale3.10     timescale3.11     timescale3.12     timescale3.13     timescale3.14
           
# null_stmt1.4  real4            timescale4.15

# Exclude diagnostics for each simulator type
ifeq ($(MAKECMDGOALS),cver)
EXCLUDE      += afunc1            afunc2            always5.1         always_comb1      always_comb2 \
                always_ff1        always_latch1     always_latch2     assert1           assign1 \
                assign2.8         assign5           atask1            bit1              byte1 \
                char1             clog2             clog2.1           clog2.2           clog2.3 \
                clog2.4           clog2.5           dec1              dec1.1            dec2 \
                dec3              dec3.1 \
                define3           delay3            dly_assign1.1     dly_assign1.2     dly_assign1.3 \
                dly_assign1.4     dly_assign2       dly_assign2.1     do_while1         do_while1.1 \
                do_while2         elsif1            elsif1.1          elsif1.2          elsif1.3 \
                elsif1.4          elsif2            elsif2.1          elsif3            enum1 \
                enum1.1           enum1.2           enum1.3           enum2.1           example \
                example1          exclude5          exclude5.1        exclude5.2        exclude5.3 \
                exclude7.3        exclude10.5       exclude12.5       exponent1         final1 \
                for3              for5              for5.1            for5.2            for5.3 \
                for5.4            for5.5            for5.6            for5.8            for5.9 \
                for5.10           for6              fsm1              fsm1.1            fsm1.2 \
                fsm1.3            fsm1.4            fsm6              fsm7              fsm7.1 \
                fsm7.2            fsm7.3            fsm7.4            fsm7.5            fsm8 \
                fsm8.1            fsm8.2            fsm8.3            fsm9              fsm9.1 \
                fsm9.2            fsm10             fsm10.1           fsm10.2           gate1.1 \
                generate1         generate1.1       generate2         generate2.1       generate3 \
                generate3.1       generate4         generate4.1       generate5         generate5.1 \
                generate5.2       generate5.3       generate6         generate7         generate7.1 \
                generate7.2       generate8         generate8.1       generate8.2       generate8.3 \
                generate8.4       generate8.5       generate8.6       generate8.7       generate8.8 \
                generate8.9       generate9         generate9.1       generate9.2       generate9.3 \
                generate10        generate10.1      generate10.2      generate11        generate11.1 \
                generate11.2      generate11.3      generate11.4      generate12        generate13 \
                generate14        generate15        generate15.1      generate15.2      generate15.3 \
                generate15.4      generate15.5      generate16        generate16.1      generate16.2 \
                generate16.3      generate17        generate18        generate19        generate19.1 \
                generate19.2      generate21        generate22        generate24        hier1.1 \
                hier2             hier3             ifdef1.1          ifdef3            ifndef2 \
                implicit2         inc1              inc1.1            inc2              inc3 \
                inc3.1            inline3           instance7         int1              line1 \
                localparam1       localparam1.1     localparam1.2     localparam1.3     localparam2 \
                localparam3       logic1            longint1          long_sig          marray1 \
                marray1.1         marray1.2         marray1.3         marray2           marray2.1 \
                marray2.2         marray2.3         mbit_sel1         mbit_sel1.1       mbit_sel5 \
                mbit_sel5.1       mbit_sel5.2       mbit_sel5.3       mbit_sel5.4       mbit_sel5.5 \
                mbit_sel6         mbit_sel6.1       mbit_sel6.2       mbit_sel6.3       mbit_sel6.4 \
                mbit_sel6.5       mem3              mem4              mem4.1            mem5 \
                nand1             nor1              op_assign1        ovl1              ovl1.1 \
                ovl1.2            ovl1.3            param8.2          param10.1         param10.5 \
                param12           param12.1         port2             profile_err1      real1.1 \
                real5.11          real5.12          real5.13          real5.14          real5.15 \
                real5.16          real6.10          real6.11          real6.12          real6.13 \
                report4           report4.1         sbit_sel3.2       shortint1         signed4.1 \
                slist3.1          slist3.3          static2           static2.1         static2.2 \
                static_afunc1     static_func1      static_func1.1    static_func2      static_func2.1 \
                static_func2.2    static_func2.3    static_func3      static_func4      timescale1.1 \
                timescale1.2      timescale1.3      timescale1.4      timescale1.5      timescale1.7 \
                timescale1.8      timescale1.10     timescale1.11     timescale1.13     timescale1.14 \
                timescale1.19     timescale2.1      timescale4        timescale4.1      timescale4.2 \
                timescale4.3      timescale4.4      timescale4.5      timescale4.6      timescale4.7 \
                timescale4.8      timescale4.9      timescale4.10     timescale4.11     timescale4.12 \
                timescale4.13     timescale4.14     timescale4.16     timescale4.17     timescale4.18 \
                timescale4.19     timescale4.20     timescale4.21     trireg            unary1 \
                urandom1          urandom1.1        urandom1.2        urandom1.3        urandom2 \
                urandom2.1        urandom3          urandom3.1        urandom_range1    urandom_range1.1 \
                urandom_range1.2  urandom_range2    urandom_range2.1  urandom_range2.2  urandom_range3 \
                value_plusargs1   value_plusargs2   value_plusargs3   value_plusargs4   value_plusargs5 \
                value_plusargs6   value_plusargs7   value_plusargs8   value_plusargs8.1
else
ifeq ($(MAKECMDGOALS),vcs)
EXCLUDE      += always5.1         assign1           assign2.8         clog2             clog2.1 \
                clog2.2           clog2.3           clog2.4           clog2.5           example           example1 \
                fsm1              fsm1.1            fsm1.2            fsm1.3            fsm1.4 \
                fsm6              fsm7              fsm7.1            fsm7.2            fsm7.3 \
                fsm7.4            fsm7.5            fsm8              fsm8.1            fsm8.2 \
                fsm8.3            fsm9              fsm9.1            fsm9.2            fsm10 \
                fsm10.1           fsm10.2           generate1         hier2             ifdef1.1 \
                implicit2         profile_err1      real1.1           report4           report4.1 \
                static2           static2.1         static2.2         timeformat1       unary1
else
ifeq ($(MAKECMDGOALS),veriwell)
EXCLUDE      += afunc1            afunc2            always5.1         always_comb1      always_comb2 \
                always_ff1        always_latch1     always_latch2     ashift1           ashift2 \
                ashift3           ashift4           assert1           assign1           assign5 \
                atask1            bit1              byte1             case5             case6 \
                casex2            casex3            casez3            casez4            casez5 \
                char1             clog2             clog2.1           clog2.2           clog2.3 \
                clog2.4           clog2.5           dec1              dec1.1            dec2 \
                dec3              dec3.1 \
                define3           define5           define6           define6.1         delay3 \
                disable2.1        dly_assign2       dly_assign2.1     do_while1         do_while1.1 \
                do_while2         elsif1            enum1             enum1.1           enum1.2 \
                enum1.3           enum2.1           example           example1          exclude5 \
                exclude5.1        exclude5.2        exclude5.3        exclude7.2        exclude7.3 \
                exclude9.8        exclude9.9        exclude10.2       exclude10.4       exclude10.5 \
                exclude12.2       exclude12.4       exclude12.5       exponent1         final1 \
                for3              for5              for5.1            for5.2            for5.3 \
                for5.4            for5.5            for5.6            for5.7            for5.8 \
                for5.9            for5.10           for6              fsm6              fsm7 \
                fsm7.1            fsm7.2            fsm7.3            fsm7.4            fsm7.5 \
                fsm8              fsm8.1            fsm8.2            fsm8.3            fsm9 \
                fsm9.1            fsm9.2            fsm10.1           fsm10.2           fsm11 \
                fsm12             fsm13             generate1         generate1.1       generate2 \
                generate2.1       generate3 \
                generate3.1       generate4         generate4.1       generate5         generate5.1 \
                generate5.2       generate5.3       generate6         generate7         generate7.1 \
                generate7.2       generate8         generate8.1       generate8.2       generate8.3 \
                generate8.4       generate8.5       generate8.6       generate8.7       generate8.8 \
                generate8.9       generate9         generate9.1       generate9.2       generate9.3 \
                generate10        generate10.1      generate10.2      generate11        generate11.1 \
                generate11.2      generate11.3      generate11.4      generate12        generate13 \
                generate14        generate15        generate15.1      generate15.2      generate15.3 \
                generate15.4      generate15.5      generate16        generate16.1      generate16.2 \
                generate16.3      generate17        generate18        generate19        generate19.1 \
                generate19.2      generate21        generate22        generate24        hier1.1 \
                hier2             hier3 \
                hier3.1           hier3.2           hier3.3           hier3.4           hier4 \
                ifndef1           ifndef2           implicit2.1       inc1              inc1.1 \
                inc2              inc3              inc3.1            inline3           int1              line1 \
                localparam1       localparam1.1     localparam1.2     localparam1.3     localparam2 \
                localparam3       logic1            longint1          marray1           marray1.1 \
                marray1.2         marray1.3         marray2           marray2.1         marray2.2 \
                marray2.3         mbit_sel1         mbit_sel1.1       mbit_sel5         mbit_sel5.1 \
                mbit_sel5.2       mbit_sel5.3       mbit_sel5.4       mbit_sel5.5       mbit_sel6 \
                mbit_sel6.1       mbit_sel6.2       mbit_sel6.3       mbit_sel6.4       mbit_sel6.5 \
                mem2              mem3              mem4              mem4.1            mem5 \
                mem6              mem6.3            named_block2      nand1             nor1 \
                null_stmt2        null_stmt2.1      op_assign1        ovl1              ovl1.1            ovl1.2 \
                ovl1.3            param7            param8            param8.1          param8.2 \
                param9.1          param9.2          param10           param10.1         param10.2 \
                param10.3         param10.4         param10.5         param12           param12.1 \
                param16           port1             port2             printtimescale1   pulldown1 \
                pullup1           random1.2         random1.3         rassign1          rassign2 \
                real2.1           real3             real3.1           real3.2           real3.4 \
                real3.5           real3.6           real3.8           real5.1           real5.2 \
                real5.3           real5.4           real5.5           real5.6           real5.7 \
                real5.8           real5.9           real5.11          real5.12          real5.13 \
                real5.14          real5.15          real5.16          real6             real6.1 \
                real6.2           real6.3           real6.4           real6.5           real6.6 \
                real6.7           real6.8           real6.9           real6.10          real6.11 \
                real6.12          real6.13          repeat3           report4           report4.1 \
                sbit_sel3.2       shortint1         signed1           signed3           signed4 \
                signed4.1         signed4.2         signed5           signed6           slist1 \
                slist1.1          slist1.2          slist1.3          slist2            slist3 \
                slist3.1          slist3.2          slist3.3          slist4            slist6 \
                static_afunc1     static_func1      static_func1.1    static_func2      static_func2.1 \
                static_func2.2    static_func2.3    static_func3      static_func4      supply1 \
                task3             task3.1           task4             task4.1 \
                time1.1           timescale1.3      timescale1.4      timescale1.5      timescale2.6 \
                timescale2.7      timescale4        timescale4.1      timescale4.2      timescale4.3 \
                timescale4.4      timescale4.5      timescale4.6      timescale4.7      timescale4.8 \
                timescale4.9      timescale4.10     timescale4.11     timescale4.12     timescale4.13 \
                timescale4.14     timescale4.16     timescale4.17     timescale4.18     timescale4.19 \
                timescale4.20     timescale4.21     trireg            unsigned1         urandom1 \
                urandom1.1        urandom1.2        urandom1.3        urandom2          urandom2.1 \
                urandom3          urandom3.1        urandom_range1    urandom_range1.1  urandom_range1.2 \
                urandom_range2    urandom_range2.1  urandom_range2.2  urandom_range3    value_plusargs1 \
                value_plusargs2   value_plusargs3   value_plusargs4   value_plusargs5   value_plusargs6 \
                value_plusargs7   value_plusargs8   value_plusargs8.1 version1          version1.1
else
ifeq ($(MAKECMDGOALS),vcomp)
EXCLUDE      +=
else
ifeq ($(MAKECMDGOALS),verilator)
DIAGS1       := $(filter $(basename $(basename $(notdir $(wildcard ../verilog/*.verilator.v)))), $(DIAGS1))
DIAGS1       += mem7
EXCLUDE      += aedge1            aedge1.1          always10          assign2.1         assign2.8 \
                assign2.9         case4.1
else
EXCLUDE      += afunc1            afunc2            always_comb1      always_comb2      always_ff1 \
                always_latch1     always_latch2     assert1           atask1            bit1 \
                byte1             char1             dec1              dec1.1            dec2 \
                dec3              dec3.1            dly_assign1.1     dly_assign1.2     dly_assign1.3 \
                dly_assign1.4     dly_assign2       dly_assign2.1     do_while1         do_while1.1 \
                do_while2         elsif1            elsif1.1          elsif1.2          elsif1.3 \
                elsif1.4          elsif2            elsif2.1          enum1             enum1.1 \
                enum1.2           enum1.3           enum2.1           exclude5          exclude5.1 \
                exclude5.2        exclude5.3        exponent1         final1            fmonitor1 \
                for3              for5              for5.1            for5.2            for5.3 \
                for5.4            for5.5            for5.6            for5.8            for5.9 \
                for5.10           for6              gate1.1           generate1.1       generate2 \
                generate3         generate3.1       generate6         generate7         generate7.1 \
                generate7.2       generate8.1       generate8.2       generate8.3       generate8.4 \
                generate8.5       generate8.6       generate8.7       generate8.8       generate9 \
                generate9.1       generate9.2       generate9.3       generate11.4      generate13 \
                generate14        generate16        hier1.1           inc1              inc1.1 \
                inc2              inc3              inc3.1            int1              line1 \
                logic1            longint1          long_sig          marray1           marray1.1 \
                marray1.2         marray1.3         marray2           marray2.1         marray2.2 \
                marray2.3         mbit_sel5.4       mbit_sel6.4       mem3              mem5 \
                null_stmt2        null_stmt2.1      op_assign1        param8.2 \
                param12           param12.1         port4             random1.2         random1.3 \
                real5.11          real5.12          real5.13          real5.14          real5.15 \
                real5.16          real6.10          real6.11          real6.12          real6.13 \
                real8             sbit_sel2.1       sbit_sel3.2       shortint1         signed4.1 \
                slist3.1          slist3.3          static_afunc1     static_func1      static_func1.1 \
                static_func2      static_func2.1    static_func2.2    static_func2.3    static_func3 \
                static_func4      timescale1.16     timescale1.17     timescale1.18 \
                timescale1.20     timescale2.5      timescale2.6      timescale2.7      timescale2.8 \
                timescale4        timescale4.1      timescale4.9      timescale4.10     timescale4.11 \
                timescale4.12     timescale4.13     timescale4.14     timescale4.16     trireg \
                urandom1.2        urandom1.3        urandom_range1    urandom_range2
endif
endif
endif
endif
endif

# List of diagnostics that should not be run if we are using VCD dumpfiles
ifdef VCD
EXCLUDE      +=  
endif

# List of diagnostics that should not be run if we are using LXT dumpfiles (because they don't output any value changes after time 0)
ifdef LXT
EXCLUDE      += add1              always7           always7.1         always11.2        ashift1 \
                ashift2           bassign2          bassign3          case5             casex2 \
                casez3            concat6           define5           disable1          disable2.1 \
                exclude1          exclude9.7        exclude9.8        exclude9.9        finish1 \
                for1              fsm11             func5             generate2.1       generate5.1 \
                generate8         generate10        generate10.1      generate10.2      generate16 \
                hier1             hier2             hier4             localparam1       localparam1.1 \
                localparam1.2     localparam1.3     localparam2       localparam3       long_exp1 \
                long_exp2         long_exp3         mbit_sel3         mbit_sel3.1       mbit_sel4 \
                mbit_sel4.1       mbit_sel4.2       mbit_sel4.3       mem1              mem2 \
                mem6              mem6.3            merge1            merge2            merge5 \
                merge6            merge6.1          merge7            merge7.1          merge7.2 \
                merge7.3          merge7.4          merge7.5          merge7.6          merge7.7 \
                merge7.8          multi_exp1        multi_exp1.1      multi_exp1.2      multi_exp1.3 \
                multi_exp1.4      multi_exp1.5      multi_exp1.6      multi_exp1.7      multi_exp1.8 \
                multi_exp3        named_block2      nested_block1     nested_block2     nested_block2.1 \
                param4            param6            param6.1          param7            param8 \
                param8.1          param9            param9.1          param9.2          param10 \
                param10.1         param10.2         param10.3         param10.4         param10.5 \
                param13           param13.1         param14           param16           port1 \
                port2             port3             race1             race1.1           race1.2 \
                rassign1          real1             repeat1           sbit_sel1.1       sbit_sel2.1 \
                score_err1.14     score_err1.15     score_err1.16     signed1           signed3 \
                signed4           signed4.2         slist2            static1           static2 \
                static2.1         static2.2         task3.1           timescale1.16     timescale1.17 \
                timescale1.18     timescale1.19     timescale2.8      trigger1          unary1 \
                wait1             wait1.1
endif

# List of diagnostics that should not be run if we are using FST dumpfiles
ifdef FST
EXCLUDE      += always7           generate2.1       generate5.1       generate11.3      hier4 \
                localparam1       localparam1.1     localparam1.2     localparam1.3     localparam2 \
                localparam3       mem2              mem6.3            merge5            merge6 \
                merge6.1          merge7            merge7.1          merge7.2          merge7.3 \
                merge7.4          merge7.5          merge7.6          merge7.7          merge7.8 \
                named_block2      param9            param9.1          param9.2          param14 \
                param16           score_err1.14     score_err1.15     score_err1.16     signed4.2 \
                trigger1
endif

# List of diagnostics that should not be run if we are using VPI
ifdef VPI
EXCLUDE      += err5.2            err5.3            err5.4            score_err1.14     score_err1.16
ifeq ($(MAKECMDGOALS),cver)
EXCLUDE      +=
else
ifeq ($(MAKECMDGOALS),vcs)
EXCLUDE      +=
else
ifeq ($(MAKECMDGOALS),veriwell)
EXCLUDE      +=
else
ifeq ($(MAKECMDGOALS),vcomp)
EXCLUDE      +=
else
EXCLUDE      += generate16.2      generate16.3      generate19        generate19.1      generate19.2 \
                generate22        generate24
endif
endif
endif
endif
endif

# List of diagnostics that should not be run if we are running in inlined mode
ifdef INLINE
DIAGS1       += assign4           example2          example2.1        example2.3        example2.4 \
                inline3.1         inline3.2         inline3.3         inline3.4         inline3.5 \
                inline3.6         inline3.7         inline4           inline5           inline6 \
                inline7

EXCLUDE      += bug2794588        case4.1           case5             casex2            casez3 \
                example1          exclude5.1        exclude5.3        inline3           mem5 \
                merge6.1          param10.1         param10.5         sbit_sel3.2       signed4.1 \
                static_func4 \
generate5    generate5.2    generate5.3    generate8.9
ifdef VPI
EXCLUDE      += fork2             generate4         generate4.1       generate8
endif

ifeq ($(MAKECMDGOALS),cver)
EXCLUDE      += always15          define4           example2.4        include1          include1.1 \
                include2          include4          include5          inline3.1         inline3.2 \
                inline3.3         inline3.4         inline3.5         inline3.6         inline3.7 \
                instance7         random1.2         real3.4           real3.6           real6 \
                sbit_sel2.1       timescale2.8
else
ifeq ($(MAKECMDGOALS),vcs)
EXCLUDE      += afunc1            afunc2            for5              for5.1            for5.2 \
                for5.3            for5.4            for5.5            for5.6            for5.8 \
                for5.9            for5.10           for6              timescale2.8      urandom1 \
                urandom1.1        urandom1.2        urandom1.3        urandom2.1        urandom_range1.1 \
                urandom_range1.2
else
ifeq ($(MAKECMDGOALS),veriwell)
EXCLUDE      += always15          compx1            compx1.1          compx1.2          compx1.3 \
                concat7           define4           elsif1.4          elsif2.1          example2 \
                example2.1        example2.3        example2.4        func1             func2 \
                ifelse1           include1          include1.1        include2          include4 \
                include5          inline3.1         inline3.2         inline3.3         inline3.4 \
                inline3.5         inline3.6         inline3.7         lshift3.3         inline5 \
                inline7           negate1           real3.3           real3.7           real5 \
                real5.10          real7             real8             rshift1.2         rshift2.2 \
                timescale1.16     timescale1.17     timescale1.18     timescale2.5      timescale2.8
else
ifeq ($(MAKECMDGOALS),vcomp)
EXCLUDE      +=
else
ifeq ($(MAKECMDGOALS),verilator)
DIAGS1       := $(filter $(basename $(basename $(notdir $(wildcard ../verilog/*.verilator.v)))), $(DIAGS1))
EXCLUDE      +=
else
EXCLUDE      += case4.1           func1             func2             nested_block2     nested_block2.1 \
                random4
endif
endif
endif
endif
endif
endif
                 
# Remove filtered diagnostics
DIAGS1         := $(filter-out $(EXCLUDE), $(DIAGS1))
DIAGS2         := $(filter-out $(EXCLUDE), $(DIAGS2))
DIAGS3         := $(filter-out $(EXCLUDE), $(DIAGS3))
DIAGS4         := $(filter-out $(EXCLUDE), $(DIAGS4))
MERGE1         := $(filter-out $(EXCLUDE), $(MERGE1))
MERGE3         := $(filter-out $(EXCLUDE), $(MERGE3))
RACELIST       := $(filter-out $(EXCLUDE), $(RACELIST))
ERRSCORE       := $(filter-out $(EXCLUDE), $(ERRSCORE))
ERRMERGE       := $(filter-out $(EXCLUDE), $(ERRMERGE))
ERRRANK        := $(filter-out $(EXCLUDE), $(ERRRANK))
NOSIMLIST      := $(filter-out $(EXCLUDE), $(NOSIMLIST))
NOSIMERRLIST   := $(filter-out $(EXCLUDE), $(NOSIMERRLIST))

iverilog:
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= VERILOG_EXTRA_FLAGS="$(IVERILOG_FLAGS1)" DIAGLIST="$(DIAGS1)" DIAG2LIST="$(BDIAGS1)" MERGELIST="$(MERGE1)" MERGE3LIST="$(MERGE3)" ERRSCORELIST="$(ERRSCORE)" ERRMERGELIST="$(ERRMERGE)" ERRREPORTLIST="$(ERRREPORT)" ERRRANKLIST="$(ERRRANK)" ERREXCLUDELIST="$(ERREXCLUDE)" RACELIST="$(RACELIST)" NOSIMLIST="$(NOSIMLIST)" NOSIMERRLIST="$(NOSIMERRLIST)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= VERILOG_EXTRA_FLAGS="$(IVERILOG_FLAGS2)" DIAGLIST="$(DIAGS2)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= VERILOG_EXTRA_FLAGS="$(IVERILOG_FLAGS3)" DIAGLIST="$(DIAGS3)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= VERILOG_EXTRA_FLAGS="$(IVERILOG_FLAGS4)" DIAGLIST="$(DIAGS4)" regress
	@cd ../verilog;  $(MAKE) eot;  $(MAKE) clean

vcs:
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VCS=1 VERILOG_EXTRA_FLAGS="$(VCS_FLAGS1)" DIAGLIST="$(DIAGS1)" DIAG2LIST="$(BDIAGS1)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VCS=1 VERILOG_EXTRA_FLAGS="$(VCS_FLAGS2)" DIAGLIST="$(DIAGS2)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VCS=1 VERILOG_EXTRA_FLAGS="$(VCS_FLAGS3)" DIAGLIST="$(DIAGS3)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VCS=1 VERILOG_EXTRA_FLAGS="$(VCS_FLAGS4)" DIAGLIST="$(DIAGS4)" regress
	@cd ../verilog;  $(MAKE) eot;  $(MAKE) clean

cver:
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_CVER=1 VERILOG_EXTRA_FLAGS="$(CVER_FLAGS1)" DIAGLIST="$(DIAGS1)" DIAG2LIST="$(BDIAGS1)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_CVER=1 VERILOG_EXTRA_FLAGS="$(CVER_FLAGS2)" DIAGLIST="$(DIAGS2)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_CVER=1 VERILOG_EXTRA_FLAGS="$(CVER_FLAGS3)" DIAGLIST="$(DIAGS3)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_CVER=1 VERILOG_EXTRA_FLAGS="$(CVER_FLAGS4)" DIAGLIST="$(DIAGS4)" regress
	@cd ../verilog;  $(MAKE) eot;  $(MAKE) clean

veriwell:
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VERIWELL=1 VERILOG_EXTRA_FLAGS="$(VW_FLAGS1)" DIAGLIST="$(DIAGS1)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VERIWELL=1 VERILOG_EXTRA_FLAGS="$(VW_FLAGS2)" DIAGLIST="$(DIAGS2)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VERIWELL=1 VERILOG_EXTRA_FLAGS="$(VW_FLAGS3)" DIAGLIST="$(DIAGS3)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VERIWELL=1 VERILOG_EXTRA_FLAGS="$(VW_FLAGS4)" DIAGLIST="$(DIAGS4)" regress
	@cd ../verilog;  $(MAKE) eot;  $(MAKE) clean

vcomp:
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VCOMP=1 VERILOG_EXTRA_FLAGS="$(VCOMP_FLAGS1)" DIAGLIST="$(DIAGS1)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VCOMP=1 VERILOG_EXTRA_FLAGS="$(VCOMP_FLAGS2)" DIAGLIST="$(DIAGS2)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VCOMP=1 VERILOG_EXTRA_FLAGS="$(VCOMP_FLAGS3)" DIAGLIST="$(DIAGS3)" regress
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VCOMP=1 VERILOG_EXTRA_FLAGS="$(VCOMP_FLAGS4)" DIAGLIST="$(DIAGS4)" regress
	@cd ../verilog;  $(MAKE) eot;  $(MAKE) clean

verilator:
	@cd ../verilog; \
	$(MAKE) COVERED_GFLAG=-Q COVERED_SCORE_GFLAG= USE_VERILATOR=1 DIAGLIST="$(DIAGS1)" regress
	@cd ../verilog;  $(MAKE) eot;  $(MAKE) clean

all-vcd-quick:
	$(MAKE) iverilog LEAVE=1
	$(MAKE) cver LEAVE=1
	$(MAKE) veriwell LEAVE=1
	$(MAKE) vcs

all-vcd-inline-quick:
	$(MAKE) iverilog LEAVE=1 INLINE=1
	$(MAKE) cver LEAVE=1 INLINE=1
	$(MAKE) veriwell LEAVE=1 INLINE=1
	$(MAKE) vcs INLINE=1

all-vcd-full:
	$(MAKE) iverilog
	$(MAKE) cver
	$(MAKE) veriwell
	$(MAKE) vcs

all-vcd-inline-full:
	$(MAKE) iverilog INLINE=1
	$(MAKE) cver INLINE=1
	$(MAKE) veriwell INLINE=1
	$(MAKE) vcs INLINE=1

all-lxt-quick:
	$(MAKE) iverilog LXT=1
	$(MAKE) veriwell LXT=1

all-lxt-inline-quick:
	$(MAKE) iverilog LXT=1 INLINE=1
	$(MAKE) veriwell LXT=1 INLINE=1

all-lxt-full:
	$(MAKE) iverilog LXT=1
	$(MAKE) veriwell LXT=1

all-lxt-inline-full:
	$(MAKE) iverilog LXT=1 INLINE=1
	$(MAKE) veriwell LXT=1 INLINE=1

all-fst-quick:
	$(MAKE) iverilog FST=1

all-fst-inline-quick:
	$(MAKE) iverilog FST=1 INLINE=1

all-fst-full:
	$(MAKE) iverilog FST=1

all-fst-inline-full:
	$(MAKE) iverilog FST=1 INLINE=1

all-vpi-quick:
	$(MAKE) iverilog VPI=1 LEAVE=1
	$(MAKE) cver VPI=1 LEAVE=1
	$(MAKE) vcs VPI=1

all-vpi-inline-quick:
	$(MAKE) iverilog VPI=1 LEAVE=1 INLINE=1
	$(MAKE) cver VPI=1 LEAVE=1 INLINE=1
	$(MAKE) vcs VPI=1 INLINE=1

all-vpi-full:
	$(MAKE) iverilog VPI=1
	$(MAKE) cver VPI=1
	$(MAKE) vcs VPI=1

all-vpi-inline-full:
	$(MAKE) iverilog VPI=1 INLINE=1
	$(MAKE) cver VPI=1 INLINE=1
	$(MAKE) vcs VPI=1 INLINE=1

all-iv:
	$(MAKE) iverilog
	$(MAKE) iverilog LXT=1
	$(MAKE) iverilog FST=1
	$(MAKE) iverilog VPI=1
	$(MAKE) iverilog INLINE=1
	$(MAKE) iverilog INLINE=1 LXT=1
	$(MAKE) iverilog INLINE=1 FST=1
	$(MAKE) iverilog INLINE=1 VPI=1

all-cver:
	$(MAKE) cver
	$(MAKE) cver VPI=1
	$(MAKE) cver INLINE=1
	$(MAKE) cver INLINE=1 VPI=1

all-veriwell:
	$(MAKE) veriwell
	$(MAKE) veriwell INLINE=1

all-vcs:
	$(MAKE) vcs
	$(MAKE) vcs VPI=1
	$(MAKE) vcs INLINE=1
	$(MAKE) vcs INLINE=1 VPI=1

all-verilator:
	$(MAKE) verilator
	$(MAKE) verilator INLINE=1

all-quick:
	$(MAKE) all-vcd-quick
	$(MAKE) all-vcd-inline-quick
	$(MAKE) all-lxt-quick
	$(MAKE) all-lxt-inline-quick
	$(MAKE) all-fst-quick
	$(MAKE) all-fst-inline-quick
	$(MAKE) all-vpi-quick
	$(MAKE) all-vpi-inline-quick

all all-full:
	$(MAKE) all-vcd-full
	$(MAKE) all-vcd-inline-full
	$(MAKE) all-lxt-full
	$(MAKE) all-lxt-inline-full
	$(MAKE) all-fst-full
	$(MAKE) all-fst-inline-full
	$(MAKE) all-vpi-full
	$(MAKE) all-vpi-inline-full

clean:
	@cd ../verilog;  $(MAKE) clobber

reset:
	@cd ../verilog;  $(MAKE) reset
