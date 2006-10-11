module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
`ifndef VPI
        $dumpfile( "func4.6.vcd" );
        $dumpvars( 0, main );
`endif
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
