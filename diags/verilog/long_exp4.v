/*
 Name:        long_exp4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/23/2009
 Purpose:     
*/

module main;

reg a1698;
reg [31:0] a1689, a1704, a1693, a1694, a1688;

initial begin
        if ( a1698 )
         begin
           a1689 = ((
            a1704 + { a1693 [27], a1693 [27],
            a1693 [27],
            a1693 , 1'b0}) + {
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 [11],
            a1694 }) +
            a1688 ;
         end
end

initial begin
`ifdef DUMP
        $dumpfile( "long_exp4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
