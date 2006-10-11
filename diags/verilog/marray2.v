module main;

reg [3:0] a[2:0][3:0], b;
integer i, j, k;

initial begin
        k = 0;
        for( i=0; i<3; i++ )
          for( j=0; j<4; j++ )
            begin
             a[i][j] = 4'h0;
             a[i][j] = k;
             if( i == 0 )
               b = a[i][j];
             k++;
            end
end

initial begin
`ifndef VPI
        $dumpfile( "marray2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
