#include "expr.h"
#include "vector.h"
#include "util.h"

int failed = 0;

void check_uinv( expression* expr, char* val_str, char* exp_str );

int main() {

  expression* tmp    = expression_create( NULL, NULL, EXP_OP_NONE, 0, 0 );
  expression* left;
  expression* right;
  expression* result;
  int         i, j; 
  int         value;

  /* Set expression width */
  tmp->value->width  = 4;

  left   = expression_create( tmp, NULL, EXP_OP_NONE, 1, 0 );
  right  = expression_create( tmp, NULL, EXP_OP_NONE, 2, 0 );
  result = expression_create( right, left, EXP_OP_UINV, 3, 0 );

  fprintf( stderr, "Running diagnostic: uinv\n" );

  check_uinv( result, "4'b0000", "4'b1111" );
  check_uinv( result, "4'b0010", "4'b1101" );
  check_uinv( result, "4'b0x00", "4'b1x11" );
  check_uinv( result, "4'bz000", "4'bx111" );

  if( failed == 0 ) {
    print_output( "PASS", NORMAL );
  } else {
    print_output( "FAIL", NORMAL );
  }
  
  return( 0 );

}

void check_uinv( expression* expr, char* val_str, char* exp_str ) {

  vector* exp;
  vector* comp = vector_create( 1, 0 );

  vector_dealloc( expr->right->value );

  expr->right->value = vector_from_string( val_str, TRUE, BINARY );
  exp                = vector_from_string( exp_str, TRUE, BINARY );

  expression_operate( expr );

  vector_op_compare( comp, expr->value, exp, COMP_CEQ );

  if( vector_to_int( comp ) == 0 ) {
    print_output( "UINV did not match compare value", FATAL );
    vector_display( expr->value );
    vector_display( exp );
    failed++;
  }

}
