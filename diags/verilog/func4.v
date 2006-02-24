module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func4.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin : return_two_block
  #10;
  return_two = 2'b10;
end

endfunction

endmodule

/* HEADER
GROUPS func4 all iv vcd lxt
SIM    func4 all iv vcd  : iverilog func4.v; ./a.out                             : func4.vcd
SIM    func4 all iv lxt  : iverilog func4.v; ./a.out -lxt2; mv func4.vcd func4.lxt : func4.lxt
SCORE  func4.vcd     : -t main -vcd func4.vcd -o func4.cdd -v func4.v >& func4.err : func4.err : 1
SCORE  func4.lxt     : -t main -lxt func4.lxt -o func4.cdd -v func4.v >& func4.err : func4.err : 1
*/

/* OUTPUT func4.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func4.v, line 15
*/
