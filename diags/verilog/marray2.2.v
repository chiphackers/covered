module main;

reg [3:0] a[0:2][0:3], b;
integer i, j, k;

initial begin
        k = 0;
        for( i=2; i>=0; i-- ) begin
          for( j=3; j>=0; j-- ) begin
            a[i][j] = 12'h0;
            #1;
            a[i][j] = k;
            if( i == 0 )
              b = a[i][j];
            k++;
          end
          #1;
        end
end

initial begin
`ifdef DUMP
        $dumpfile( "marray2.2.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
