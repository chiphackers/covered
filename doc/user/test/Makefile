all:
	iverilog -v -c libs example.v
	./a.out
	../../../src/covered score -t main -v example.v -vcd example.vcd -A ovl +libext+.vlib+ -y $(OVL_HOME) -y $(OVL_HOME)/vlog95 -I $(OVL_HOME)
	../../../src/covered report -d v -m ltcfra -o example.rptM cov.cdd
	../../../src/covered report -d v -m ltcfra -i -o example.rptI cov.cdd
	./prep_files

clean:
	rm -rf a.out *.cdd *.rptM *.rptI *.vcd
