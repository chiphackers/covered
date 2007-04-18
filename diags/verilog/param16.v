/*
 Name:     param16.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     04/13/2007
 Purpose:  
*/

module main;

foo #(      .b(32),.c(64)) b();
foo #(.a(0),.b(16),.c(32)) a();

initial begin
`ifndef VPI
        $dumpfile( "param16.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module foo;

parameter a = 1, b = 1, c = 1;

reg [b-1:0] mem0[0:2], mem1[0:2];

initial begin
        mem0[0] = {b{1'b0}};
end

endmodule
