module main;

bit              a = 0;
bit [1:0]        b = 0;
bit signed       c = 0;
bit signed [3:0] d = 0;

initial begin
`ifdef DUMP
        $dumpfile( "bit1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
