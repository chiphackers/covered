module main;

integer i, j;

initial begin
	i = 10 + 5 + 1 + 134;
	j = 10 + 5 + -1 + -134;
end

initial begin
        $dumpfile( "add1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule

/* HEADER
GROUPS add1 iv vcs vcd lxt
COMP   iv : iverilog add1.v
COMP   vcs : vcs add1.v
SIM    iv : ./a.out
SIM    lxt : ./a.out -lxt2
SIM    vcs : ./simv
SCORE  add1 iv vcs vcd : -t main -vcd add1.vcd -v add1.v -o add1.cdd
SCORE  add1 iv lxt : -t main -lxt add1.vcd -v add1.v -o add1.cdd
REPORT add1 iv vcs vcd lxt : -d v -o add1.v.rpt add1.cdd
*/
