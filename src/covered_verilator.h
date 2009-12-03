#ifndef __COVERED_VERILATOR_H__
#define __COVERED_VERILATOR_H__

#ifdef __cplusplus
extern "C" {
  void db_add_line_coverage( uint32_t, uint32_t );
  int db_read( const char*, int );
  void bind_perform( int, int );
}
#endif

inline void covered_line( uint32_t inst_index, uint32_t expr_index ) {
  db_add_line_coverage( inst_index, expr_index );
}

inline void covered_initialize( const char* cdd_name ) {
  db_read( cdd_name, 0 );
  bind_perform( 1, 0 );
}

#endif
