ifndef DIAGNAME
DIAGNAME = test
endif

ifndef DIAGTYPE
DIAGTYPE = diags1
endif

-include ../regress/Makefile

convert: convert_diags1 convert_diags2 convert_diags3 convert_diags4 convert_merge1 convert_racelist \
         convert_errscore convert_errreport convert_ivonly convert_vcsonly

new:
	./gen_test $(DIAGNAME)
	cp $(DIAGNAME).v ../verilog/$(DIAGNAME).v
	echo "EMPTY-$(DIAGNAME).cdd" > ../cdd/$(DIAGNAME).cdd
	echo "EMPTY-$(DIAGNAME).rptM" > ../rpt/$(DIAGNAME).rptM
	echo "EMPTY-$(DIAGNAME).rptWM" > ../rpt/$(DIAGNAME).rptWM
	echo "EMPTY-$(DIAGNAME).rptI" > ../rpt/$(DIAGNAME).rptI
	echo "EMPTY-$(DIAGNAME).rptWI" > ../rpt/$(DIAGNAME).rptWI
	./convert $(DIAGNAME) $(DIAGTYPE) 
	rm ../regress/$(DIAGNAME).cfg
	rm ../cdd/$(DIAGNAME).cdd
	rm ../rpt/$(DIAGNAME).rptM
	rm ../rpt/$(DIAGNAME).rptWM
	rm ../rpt/$(DIAGNAME).rptI
	rm ../rpt/$(DIAGNAME).rptWI

convert_diags1: $(DIAGS1)

$(DIAGS1) :
	./convert $@ diags1

convert_diags2 : $(DIAGS2)

$(DIAGS2) :
	./convert $@ diags2

convert_diags3 : $(DIAGS3)

$(DIAGS3) :
	./convert $@ diags3

convert_diags4 : $(DIAGS4)

$(DIAGS4) :
	./convert $@ diags4

convert_merge1 : $(MERGE1)

$(MERGE1) :
	./convert $@ merge1

convert_errscore : $(ERRSCORE)

$(ERRSCORE) :
	./convert $@ errscore

convert_errreport : $(ERRREPORT)

$(ERRREPORT) :
	./convert $@ errreport

convert_ivonly : $(IVONLYDIAGS1)

$(IVONLYDIAGS1) :
	./convert $@ ivonly

convert_vcsonly : $(VCSONLYDIAGS1)

$(VCSONLYDIAGS1) :
	./convert $@ vcsonly

