#include "expr.h"
#include "vector.h"
#include "util.h"

int failed = 0;

void check_multiply( expression* result, char* left_val,char* right_val, char* exp_val );

int main() {

  expression* ltmp = expression_create( NULL, NULL, EXP_OP_NONE, 0, 0 );
  expression* rtmp = expression_create( NULL, NULL, EXP_OP_NONE, 0, 0 );
  expression* left;
  expression* right;
  expression* result;
  int     i, j; 

  /* Set expression width */
  ltmp->value->width  = 4;
  rtmp->value->width  = 4;

  left   = expression_create( ltmp, NULL, EXP_OP_NONE, 1, 0 );
  right  = expression_create( rtmp, NULL, EXP_OP_NONE, 2, 0 );
  result = expression_create( right, left, EXP_OP_MULTIPLY, 3, 0 );

  fprintf( stderr, "Running diagnostic: multiply_x\n" );

  // Multiplication by 0 yields a value of 0.
  check_multiply( result, "4'b01x1", "4'b0000", "4'b0000" );
  check_multiply( result, "4'b0000", "4'b1x10", "4'b0000" );

  // Multiplication by 1 yields the same value.
  check_multiply( result, "4'b1x10", "4'b0001", "4'b1x10" );
  check_multiply( result, "4'b0001", "4'bxx01", "4'bxx01" );

  // Any other multiplication containing X yields a value of X.
  check_multiply( result, "4'bx01x", "4'b0101", "8'bxxxxxxxx" );
  check_multiply( result, "4'b0010", "4'bx0x0", "8'bxxxxxxxx" );
  check_multiply( result, "4'bx0xx", "4'b001x", "8'bxxxxxxxx" );

  if( failed == 0 ) {
    print_output( "PASS", NORMAL );
  } else {
    print_output( "FAIL", NORMAL );
  }

  expression_dealloc( result );
  
  return( 0 );

}

void check_multiply( expression* result, char* left_val,char* right_val, char* exp_val ) {

  vector* vec  = vector_from_string( exp_val, TRUE, BINARY );
  vector* comp = vector_create( 1, 0 );

  vector_dealloc( result->left->value );
  vector_dealloc( result->right->value );

  result->left->value  = vector_from_string( left_val, TRUE, BINARY );
  result->right->value = vector_from_string( right_val, TRUE, BINARY );

  expression_operate( result );

  vector_op_compare( comp, result->value, vec, COMP_CEQ );

  if( vector_to_int( comp ) == 0 ) {
    print_output( "Multiply result does not match expected value", FATAL );
    vector_display( result->value );
    printf( "Expected result: %s\n", exp_val );
    failed++;
  }

  vector_dealloc( vec );
  vector_dealloc( comp );

}

