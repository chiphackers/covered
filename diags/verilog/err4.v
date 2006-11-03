/*
 Name:     err4.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/03/2006
 Purpose:  Used to verify top-level checker with presence/absence
           of the -i option.
*/

module main;

reg x, y, z;

foo a (
  .x( x ),
  .y( y ),
  .z( z )
);

initial begin
	x = 1'b0;
	y = 1'b0;
	z = 1'b0;
	#10;
	$finish;
end

endmodule

//------------------------------------

module foo (
  input wire x,
  input wire y,
  input wire z
);

wire a = x | y | z;

endmodule
