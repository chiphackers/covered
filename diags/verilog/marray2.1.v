module main;

reg [3:0] a[0:2][3:0], b;
integer i, j, k;

initial begin
        k = 0;
        for( i=2; i>=0; i-- )
          for( j=0; j<4; j++ )
            begin
             a[i][j] = 12'h0;
             a[i][j] = k;
             if( i == 0 )
               b = a[i][j];
             k++;
            end
end

initial begin
        $dumpfile( "marray2.1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
