module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func3.2.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin
  @(negedge a);
  return_two = 2'b10;
end

endfunction

endmodule

/* HEADER
GROUPS func3.2 all iv vcd lxt
SIM    func3.2 all iv vcd  : iverilog func3.2.v; ./a.out                             : func3.2.vcd
SIM    func3.2 all iv lxt  : iverilog func3.2.v; ./a.out -lxt2; mv func3.2.vcd func3.2.lxt : func3.2.lxt
SCORE  func3.2.vcd     : -t main -vcd func3.2.vcd -o func3.2.cdd -v func3.2.v >& func3.2.err : func3.2.err : 1
SCORE  func3.2.lxt     : -t main -lxt func3.2.lxt -o func3.2.cdd -v func3.2.v >& func3.2.err : func3.2.err : 1
*/

/* OUTPUT func3.2.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func3.2.v, line 15
*/
