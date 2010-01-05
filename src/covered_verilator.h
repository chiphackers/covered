#ifndef __COVERED_VERILATOR_H__
#define __COVERED_VERILATOR_H__

/*
 Copyright (c) 2006-2010 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
  int db_verilator_initialize( const char* );
  int db_verilator_close( const char* );
  int db_add_line_coverage( uint32_t, uint32_t );
}
#endif /* __cplusplus */

inline void covered_line( uint32_t inst_index, uint32_t expr_index ) {

  if( !db_add_line_coverage( inst_index, expr_index ) ) {
    fprintf( stderr, "Covered Error!\n" );
    exit( 1 );
  }

}

#ifndef COVERED_METRICS_ONLY

inline void covered_initialize_db( const char* cdd_name ) {

  if( !db_verilator_initialize( cdd_name ) ) {
    fprintf( stderr, "Covered Error!\n" );
    exit( 1 );
  }

}

inline void covered_close( const char* cdd_name ) {

  if( !db_verilator_close( cdd_name ) ) {
    fprintf( stderr, "Covered Error!\n" );
    exit( 1 );
  }

}

#endif /* COVERED_METRICS_ONLY */

#endif /* __COVERED_VERILATOR_H__ */
