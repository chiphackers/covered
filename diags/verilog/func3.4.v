module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func3.4.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin
  @(posedge a or negedge a);
  return_two = 2'b10;
end

endfunction

endmodule

/* HEADER
GROUPS func3.4 all iv vcd lxt
SIM    func3.4 all iv vcd  : iverilog func3.4.v; ./a.out                             : func3.4.vcd
SIM    func3.4 all iv lxt  : iverilog func3.4.v; ./a.out -lxt2; mv func3.4.vcd func3.4.lxt : func3.4.lxt
SCORE  func3.4.vcd     : -t main -vcd func3.4.vcd -o func3.4.cdd -v func3.4.v >& func3.4.err : func3.4.err : 1
SCORE  func3.4.lxt     : -t main -lxt func3.4.lxt -o func3.4.cdd -v func3.4.v >& func3.4.err : func3.4.err : 1
*/

/* OUTPUT func3.4.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func3.4.v, line 15
*/
