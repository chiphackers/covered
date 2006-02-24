module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func4.1.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin : return_two_block
  @(posedge a);
  return_two = 2'b10;
end

endfunction

endmodule

/* HEADER
GROUPS func4.1 all iv vcd lxt
SIM    func4.1 all iv vcd  : iverilog func4.1.v; ./a.out                             : func4.1.vcd
SIM    func4.1 all iv lxt  : iverilog func4.1.v; ./a.out -lxt2; mv func4.1.vcd func4.1.lxt : func4.1.lxt
SCORE  func4.1.vcd     : -t main -vcd func4.1.vcd -o func4.1.cdd -v func4.1.v >& func4.1.err : func4.1.err : 1
SCORE  func4.1.lxt     : -t main -lxt func4.1.lxt -o func4.1.cdd -v func4.1.v >& func4.1.err : func4.1.err : 1
*/

/* OUTPUT func4.1.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func4.1.v, line 15
*/
