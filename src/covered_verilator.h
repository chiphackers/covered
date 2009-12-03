#ifndef __COVERED_VERILATOR_H__
#define __COVERED_VERILATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include "cexcept.h"

/*!
 This will define the exception type that gets thrown (Covered does not care about this value)
*/
define_exception_type(int);

extern struct exception_context the_exception_context[1];

#ifdef __cplusplus
extern "C" {
  void db_add_line_coverage( uint32_t, uint32_t );
  int db_read( const char*, int );
  void db_write( const char*, int, int );
  void db_close();
  void bind_perform( int, int );
}
#endif

inline void covered_line( uint32_t inst_index, uint32_t expr_index ) {
  db_add_line_coverage( inst_index, expr_index );
}

inline void covered_initialize( const char* cdd_name ) {

  Try {
    db_read( cdd_name, 0 );
  } Catch_anonymous {
    fprintf( stderr, "Covered Error!\n" );
    exit( 1 );
  }

  Try {
    bind_perform( 1, 0 );
  } Catch_anonymous {
    fprintf( stderr, "Covered Error!\n" );
    exit( 1 );
  }

}

inline void covered_close( const char* cdd_name ) {

  /* Write contents to database file */
  Try {
    db_write( cdd_name, 0, 0 );
  } Catch_anonymous {
    fprintf( stderr, "Covered Error!\n" );
    exit( 1 );
  }

  /* Close the database */
  db_close();

}

#endif
