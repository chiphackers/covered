%{
/*!
 \file     vcd_parser.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/2/2001
*/

#include <stdio.h>

#include "defines.h"
#include "db.h"
#include "vector.h"
#include "vcd_parser_misc.h"

/* Uncomment these lines to turn debuggin on */
//#define YYDEBUG 1
//#define YYERROR_VERBOSE 1
//int yydebug = 1;
%}

%union {
  char*          text;
  unsigned long  number;
  sigwidth*      sigwidth;
};

%token <text>     VALUE IDENTIFIER HIDENTIFIER
%token <sigwidth> SIGWIDTH
%token            CHANGE
%token <number>   TIMESTAMP
%token V_var V_end V_scope V_upscope
%token V_comment V_date V_dumpall V_dumpoff V_dumpon
%token V_dumpvars V_enddefinitions
%token V_dumpports V_dumpportsoff V_dumpportson V_dumpportsall
%token V_timescale V_version V_vcdclose
%token V_event V_parameter V_unknown
%token V_integer V_real V_reg V_supply0
%token V_supply1 V_time V_tri V_triand V_trior
%token V_trireg V_tri0 V_tri1 V_wand V_wire V_wor V_port
%token V_module V_task V_function V_begin V_fork

%type <text> signame 

%%

main
	: dumpcode
	|
	;

dumpcode
	: definitions 
	  V_enddefinitions V_end
	  changelists
	;

definitions
	: definition
	| definitions definition
	;

definition
	: V_scope scope_type HIDENTIFIER V_end
		{
		  db_set_vcd_scope( $3 );
		  free_safe( $3 );
		}
	| V_scope scope_type IDENTIFIER V_end
		{
		  db_set_vcd_scope( $3 );
		  free_safe( $3 );
		}
	| V_upscope V_end
		{
		  db_vcd_upscope();
		}
	| V_var supported_vartype VALUE VALUE signame V_end
		{
		  db_assign_symbol( $5, $4 );
		  free_safe( $4 );
		  free_safe( $5 );
		}
        | V_var supported_vartype VALUE IDENTIFIER signame V_end
                {
                  db_assign_symbol( $5, $4 );
                  free_safe( $4 );
                  free_safe( $5 );
                }
	| V_var unsupported_vartype VALUE VALUE signame V_end
		{
		  free_safe( $4 );
		}
        | V_var unsupported_vartype VALUE IDENTIFIER signame V_end
                {
                  free_safe( $4 );
                }
	| V_unknown
		{
		  fprintf( stderr, "Error:  Unknown VCD keyword, line %d\n", @1.first_line );
		  exit( 1 );
		}
	;

scope_type
	: V_module
	| V_task
	| V_function
	| V_begin
	| V_fork
	;

supported_vartype
	: V_reg
	| V_wire
	;

unsupported_vartype
	: V_event
	| V_parameter
	| V_integer
	| V_real
	| V_supply0
	| V_supply1
	| V_time
	| V_tri
	| V_triand
	| V_trior
	| V_trireg
	| V_tri0
	| V_tri1
	| V_wand
	| V_wor
	| V_port
	;

signame
	: IDENTIFIER
		{
		  $$ = $1;
		}
	| IDENTIFIER SIGWIDTH
		{
		  $$ = $1;
		}
	| HIDENTIFIER
		{
		  $$ = $1;
		}
	| HIDENTIFIER SIGWIDTH
		{
		  $$ = $1;
		}
	;

changelists
	: changelists changelist
	| changelist
	;

changelist
	: TIMESTAMP changes
		{
		  /* Need to perform timestep */
		  db_do_timestep( $1 );
		}
	| TIMESTAMP V_dumpvars changes V_end
		{
		  /* Need to perform timestep */
		  db_do_timestep( $1 );
		}
	| TIMESTAMP V_dumpall changes V_end
		{
		  /* Need to perform timestep */
		  db_do_timestep( $1 );
		}
	| TIMESTAMP V_dumpon changes V_end
		{
		  /* Need to perform timestep */
		  db_do_timestep( $1 );
		}
	| TIMESTAMP V_dumpoff changes V_end
		{
		  /* Need to perform timestep */
		  db_do_timestep( $1 );
		}
	;

changes
	: CHANGE
	| CHANGE changes
	;

%%
