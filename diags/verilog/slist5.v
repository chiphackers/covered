/*
 Name:     slist5.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     10/22/2006
 Purpose:  Verifies that an implicit sensitivity list handles
           things properly when there are no RHS variables in
           its block.
*/

module main;

reg foo;

always @* foo = 1'b0;

endmodule
