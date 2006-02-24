module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func3.6.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
reg   b;
begin
  b <= 1'b0;
  return_two = 2'b10;
end

endfunction

endmodule

/* HEADER
GROUPS func3.6 all iv vcd lxt
SIM    func3.6 all iv vcd  : iverilog func3.6.v; ./a.out                             : func3.6.vcd
SIM    func3.6 all iv lxt  : iverilog func3.6.v; ./a.out -lxt2; mv func3.6.vcd func3.6.lxt : func3.6.lxt
SCORE  func3.6.vcd     : -t main -vcd func3.6.vcd -o func3.6.cdd -v func3.6.v >& func3.6.err : func3.6.err : 1
SCORE  func3.6.lxt     : -t main -lxt func3.6.lxt -o func3.6.cdd -v func3.6.v >& func3.6.err : func3.6.err : 1
*/

/* OUTPUT func3.6.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func3.6.v, line 16
*/
