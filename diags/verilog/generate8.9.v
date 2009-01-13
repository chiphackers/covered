/*
 Name:     generate8.9.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     01/12/2009
 Purpose:  Verifies that named blocks within generate statements produce the correct code.
*/

module main;

parameter A = 0,
          B = 0;

generate
 if( A < 1 )
   begin : bar
    reg a;
    initial begin
            a = 1'b0;
            #10;
            a = 1'b1;
    end 
   end
 else
   reg b;
 if( B < 1 )
   begin : foo
    reg c;
    initial begin
            c = 1'b0;
            #10;
            c = 1'b1;
    end
   end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate8.9.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
