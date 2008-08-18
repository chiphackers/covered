OVL_HOME = ../../../diags/verilog/ovl

all:
	iverilog -v -c libs example.v
	./a.out
	../../../src/covered -D score -t main -v example.v -vcd example.vcd -A ovl +libext+.vlib+ -y $(OVL_HOME) -y $(OVL_HOME)/vlog95 -I $(OVL_HOME)
	../../../src/covered report -d v -m ltcfram -o example.rptM cov.cdd
	../../../src/covered report -d v -m ltcfram -i -o example.rptI cov.cdd
	./prep_files

clean:
	rm -rf a.out *.cdd *.rptM *.rptI *.vcd
