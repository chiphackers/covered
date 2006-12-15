/*
 Name:     atask1.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     12/14/2006
 Purpose:  Basic test to verify automatic task recursion.
*/

module main;

reg bar;

initial begin
	bar = 1'b0;
	#5;
	foo( 1'b1 );
end

initial begin
`ifndef VPI
        $dumpfile( "atask1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

task automatic foo;
  input a;
  begin
   if( a )
     foo( ~a );
   bar = a;
  end
endtask

endmodule
