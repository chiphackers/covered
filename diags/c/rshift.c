#include "expr.h"
#include "util.h"

int failed = 0;

int main() {

  expression* tmp    = expression_create( NULL, NULL, EXP_OP_NONE, 0, 0 );
  expression* left;
  expression* right;
  expression* result;
  int         i, j; 
  int         value;

  /* Set expression width */
  tmp->value->width  = 16;

  left   = expression_create( tmp, NULL, EXP_OP_NONE, 1, 0 );
  right  = expression_create( tmp, NULL, EXP_OP_NONE, 2, 0 );
  result = expression_create( right, left, EXP_OP_RSHIFT, 3, 0 );

  fprintf( stderr, "Running diagnostic: rshift\n" );

  for( i=0; i<16; i++ ) {

    for( j=0; j<16; j++ ) {

      value = random() & 0xffff;

      vector_from_int( right->value, j );
      vector_from_int( left->value, value );
  
      expression_operate( result );

      if( vector_to_int( result->value ) != ((value >> j) & 0xffff) ) {
        print_output( "RShift result does not match expected value", FATAL );
        printf( "Expected result: 0x%x\n", ((value >> j) & 0xffff) );
        vector_display( result );
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
