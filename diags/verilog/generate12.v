/*
 Name:     generate12.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     10/13/2006
 Purpose:  
*/

module main;

foo #(2) f1();
foo #(3) f2();

initial begin
`ifndef VPI
        $dumpfile( "generate12.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

//-------------------------------------

module foo;

parameter A = 1;

generate
  genvar i;
  for( i=0; i<A; i=i+1 )
    begin : U
      bar b();
    end
endgenerate

endmodule

//-------------------------------------

module bar;

  reg a = 1'b0;

endmodule

