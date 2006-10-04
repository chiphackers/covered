module main;

reg [3:0] a[2:0][0:3], b;
integer i, j, k;

initial begin
        k = 0;
        for( i=0; i<3; i++ )
          for( j=3; j>=0; j-- )
            begin
             a[i][j] = 12'h0;
             a[i][j] = k;
             if( i == 0 )
               b = a[i][j];
             k++;
            end
end

initial begin
        $dumpfile( "marray2.3.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
