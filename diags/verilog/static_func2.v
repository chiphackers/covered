module main;

reg [const_func(4)-1:0] a;

foo bar[const_func(4)-1:0] (
  .a(a)
);

initial begin
`ifndef VPI
        $dumpfile( "static_func2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function [31:0] const_func;
  input [31:0] size;
  begin
    const_func = 0;
    const_func[size] = 1;
  end
endfunction

endmodule


module foo ( input wire a );

wire b = ~a;

endmodule
