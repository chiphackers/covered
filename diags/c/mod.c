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
  result = expression_create( right, left, EXP_OP_MOD, 3, 0 );

  fprintf( stderr, "Running diagnostic: mod\n" );

  for( i=0; i<16; i++ ) {

    vector_from_int( left->value, i );

    expression_operate( left );

    for( j=1; j<16; j++ ) {

      vector_from_int( right->value, j );

      expression_operate( right );
      expression_operate( result );

      if( vector_to_int( result->value ) != (i % j) ) {
        print_output( "Modulus result does not match expected value", FATAL );
        vector_display( result );
        printf( "Expected result: %d\n", (i % j) );
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
