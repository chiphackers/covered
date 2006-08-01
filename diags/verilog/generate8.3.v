module main;

parameter A = 0,
          B = 0,
          C = 0;

generate
  begin
   if( A < 1 )
     if( B < 1 )
       begin
        reg a;
        initial begin
                a = 1'b0;
                #10;
                a = 1'b1;
        end 
       end
     else
       reg b;
   if( C < 1 )
     begin
      reg c;
      initial begin
              c = 1'b0;
              #10;
              c = 1'b1;
      end
     end
  end
endgenerate

initial begin
        $dumpfile( "generate8.3.vcd" );
        $dumpvars( 0, main );
        #20;
        $finish;
end

endmodule
