#include "vector.h"
#include "util.h"

int failed = 0;

int main() {

  vector* vec1   = vector_create( 4, 0 );
  vector* vec2   = vector_create( 4, 0 );
  vector* result = vector_create( 4, 0 );
  int     i, j; 

  fprintf( stderr, "Running diagnostic: subtract\n" );

  for( i=0; i<16; i++ ) {

    vector_from_int( vec1, i );

    for( j=0; j<16; j++ ) {

      vector_from_int( vec2, j );
  
      vector_op_subtract( result, vec1, vec2 );

      if( vector_to_int( result ) != (((i + 16) - j) % 16) ) {
        print_output( "Difference does not match expected value", FATAL );
        vector_display( result );
        printf( "Expected result: %d\n", (((i + 16) - j) % 16) );
        failed++;
      }

    }

  }

  if( failed == 0 ) {
    print_output( "PASS", NORMAL );
  } else {
    print_output( "FAIL", NORMAL );
  }
  
  return( 0 );

}
