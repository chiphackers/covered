#include "expr.h"
#include "util.h"

int failed = 0;

int main() {

  expression* tmp    = expression_create( NULL, NULL, EXP_OP_NONE, 0, 0 );
  expression* left;
  expression* right;
  expression* result;
  int     i, j; 

  /* Set expression width */
  tmp->value->width  = 4;

  left   = expression_create( tmp, NULL, EXP_OP_NONE, 1, 0 );
  right  = expression_create( tmp, NULL, EXP_OP_NONE, 2, 0 );
  result = expression_create( right, left, EXP_OP_MULTIPLY, 3, 0 );

  fprintf( stderr, "Running diagnostic: multiply\n" );

  for( i=0; i<16; i++ ) {

    vector_from_int( left->value, i );

    for( j=0; j<16; j++ ) {

      vector_from_int( right->value, j );
  
      expression_operate( result );

      vector_display( result->value );

      if( vector_to_int( result->value ) != ((i * j) & 0xff) ) {
        print_output( "Multiply result does not match expected value", FATAL );
        vector_display( result->value );
        printf( "Expected result: %d\n", ((i * j) & 0xff) );
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
