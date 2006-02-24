module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func4.2.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin : return_two_block
  @(negedge a);
  return_two = 2'b10;
end

endfunction

endmodule

/* HEADER
GROUPS func4.2 all iv vcd lxt
SIM    func4.2 all iv vcd  : iverilog func4.2.v; ./a.out                             : func4.2.vcd
SIM    func4.2 all iv lxt  : iverilog func4.2.v; ./a.out -lxt2; mv func4.2.vcd func4.2.lxt : func4.2.lxt
SCORE  func4.2.vcd     : -t main -vcd func4.2.vcd -o func4.2.cdd -v func4.2.v >& func4.2.err : func4.2.err : 1
SCORE  func4.2.lxt     : -t main -lxt func4.2.lxt -o func4.2.cdd -v func4.2.v >& func4.2.err : func4.2.err : 1
*/

/* OUTPUT func4.2.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func4.2.v, line 15
*/
