all:
	iverilog example.v
	./a.out
	../../../src/covered score -t main -v example.v -vcd example.vcd
	../../../src/covered report -d v -o example.rptM cov.cdd
	../../../src/covered report -d v -i -o example.rptI cov.cdd
	./prep_files

clean:
	rm -rf a.out *.cdd *.rptM *.rptI *.vcd
