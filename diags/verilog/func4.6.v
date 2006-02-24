module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func4.6.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
reg   b;
begin : return_two_block
  b <= 1'b0;
  return_two = 2'b10;
end

endfunction

endmodule

/* HEADER
GROUPS func4.6 all iv vcd lxt
SIM    func4.6 all iv vcd  : iverilog func4.6.v; ./a.out                             : func4.6.vcd
SIM    func4.6 all iv lxt  : iverilog func4.6.v; ./a.out -lxt2; mv func4.6.vcd func4.6.lxt : func4.6.lxt
SCORE  func4.6.vcd     : -t main -vcd func4.6.vcd -o func4.6.cdd -v func4.6.v >& func4.6.err : func4.6.err : 1
SCORE  func4.6.lxt     : -t main -lxt func4.6.lxt -o func4.6.cdd -v func4.6.v >& func4.6.err : func4.6.err : 1
*/

/* OUTPUT func4.6.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func4.6.v, line 16
*/
