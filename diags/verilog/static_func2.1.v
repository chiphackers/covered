module main;

reg [3:0] a;
wire      b2;

foo bar[const_func(2)-1:0] (
  .a(a)
);

assign b2 = bar[3].b | bar[2].b;

initial begin
`ifndef VPI
        $dumpfile( "static_func2.1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'h4;
	#10;
	a = 4'hb;
        #10;
        $finish;
end

function [31:0] const_func;
  input [31:0] bit_to_set;
  begin
    const_func = 0;
    const_func[bit_to_set] = 1'b1;
  end
endfunction

endmodule


module foo ( input wire a );

wire b = ~a;

endmodule
