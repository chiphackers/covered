#include "vector.h"
#include "util.h"

extern nibble nor_optab[16];

int failed = 0;
void check_nor( char* val1, char* val2, char* value );

int main() {

  fprintf( stderr, "Running diagnostic: nor\n" );

  check_nor( "4'b0000", "4'b0000", "4'b1111" );
  check_nor( "4'b0000", "4'b0001", "4'b1110" );
  check_nor( "4'b0000", "4'b000x", "4'b111x" );
  check_nor( "4'b0000", "4'b000z", "4'b111x" );

  check_nor( "4'b0010", "4'b0000", "4'b1101" );
  check_nor( "4'b0010", "4'b0010", "4'b1101" );
  check_nor( "4'b0010", "4'b00x0", "4'b1101" );
  check_nor( "4'b0010", "4'b00z0", "4'b1101" );

  check_nor( "4'b0x00", "4'b0000", "4'b1x11" );
  check_nor( "4'b0x00", "4'b0100", "4'b1011" );
  check_nor( "4'b0x00", "4'b0x00", "4'b1x11" );
  check_nor( "4'b0x00", "4'b0z00", "4'b1x11" );
  
  check_nor( "4'bz000", "4'b0000", "4'bx111" );
  check_nor( "4'bz000", "4'b1000", "4'b0111" );
  check_nor( "4'bz000", "4'bx000", "4'bx111" );
  check_nor( "4'bz000", "4'bz000", "4'bx111" );

  if( failed == 0 ) {
    print_output( "PASS", NORMAL );
  } else {
    print_output( "FAIL", NORMAL );
  }

  return( 0 );

}

void check_nor( char* val1, char* val2, char* value ) {

  vector* vec1    = vector_from_string( val1, TRUE, BINARY );
  vector* vec2    = vector_from_string( val2, TRUE, BINARY );
  vector* comp    = vector_from_string( value, TRUE, BINARY );
  vector* result1 = vector_create( ((vec1->width > vec2->width) ? vec1->width : vec2->width), 0 );
  vector* result2 = vector_create( 1, 0 );

  vector_bitwise_op( result1, vec1, vec2, nor_optab );
  vector_op_compare( result2, result1, comp, COMP_CEQ );

  if( vector_to_int( result2 ) == 0 ) {
    print_output( "XOR result does not match expected", FATAL );
    vector_display( result1 );
    vector_display( comp );
    failed++;
  }
  
  vector_dealloc( vec1 );
  vector_dealloc( vec2 );
  vector_dealloc( comp );
  vector_dealloc( result1 );
  vector_dealloc( result2 );
 
}
  
