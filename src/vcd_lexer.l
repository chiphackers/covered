%{ 
/*!
 \file     vcd_lexer.l
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/3/2001
 \brief    Lexer for VCD Format
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "vcd_parser_misc.h"
#include "vcd_parser.h"
#include "vector.h"
#include "db.h"

#define yylval VCDlval

extern YYLTYPE yylloc;

char* vcdfile;
%}

%option stack

%x COMMENT
%x DATE
%x TIMESCALE
%x VERSION

%s SIMULATION
%s DEFINITION

WSPACE [ \t\b\f]+

%%

  /* Whitespace -- we ignore this */
[ \t\b\f\r]   { ; }

  /* Newline character - blank line -- we ignore these */
\n            { yylloc.first_line += 1; }

  /* Ignore comments */
\$comment      { BEGIN( COMMENT );       }
<COMMENT>\$end { BEGIN( 0 );             }
<COMMENT>\n    { yylloc.first_line += 1; }
<COMMENT>.     { yymore();               }

  /* Ignore date */
\$date         { BEGIN( DATE );    }
<DATE>\$end    { BEGIN( 0 );             }
<DATE>\n       { yylloc.first_line += 1; }
<DATE>.        { yymore();               }

  /* Ignore timescale */
\$timescale      { BEGIN( TIMESCALE );     }
<TIMESCALE>\$end { BEGIN( 0 );             }
<TIMESCALE>\n    { yylloc.first_line += 1; }
<TIMESCALE>.     { yymore();               }

  /* Ignore version */
\$version      { BEGIN( VERSION );       }
<VERSION>\$end { BEGIN( 0 );             }
<VERSION>\n    { yylloc.first_line += 1; }
<VERSION>.     { yymore();               }

<DEFINITION>\$end {
     BEGIN( 0 );
     return( V_end );
 }

  /* VCD Keywords */
\$[a-zA-Z]*   {
     if(        strcmp( yytext, "$var"            ) == 0 ) { 
       BEGIN( DEFINITION );
       return( V_var );
     } else if( strcmp( yytext, "$end"            ) == 0 ) { 
       return( V_end );
     } else if( strcmp( yytext, "$scope"          ) == 0 ) { 
       BEGIN( DEFINITION );
       return( V_scope );
     } else if( strcmp( yytext, "$upscope"        ) == 0 ) { 
       BEGIN( DEFINITION );
       return( V_upscope );
     } else if( strcmp( yytext, "$enddefinitions" ) == 0 ) {
       BEGIN( SIMULATION );
       return( V_enddefinitions );
     }
     else if( strcmp( yytext, "$dumpall"        ) == 0 ) { return( V_dumpall );        }
     else if( strcmp( yytext, "$dumpoff"        ) == 0 ) { return( V_dumpoff );        }
     else if( strcmp( yytext, "$dumpon"         ) == 0 ) { return( V_dumpon );         }
     else if( strcmp( yytext, "$dumpvars"       ) == 0 ) { return( V_dumpvars );       }
     else if( strcmp( yytext, "$dumpports"      ) == 0 ) { return( V_dumpports );      }
     else if( strcmp( yytext, "$dumpportsoff"   ) == 0 ) { return( V_dumpportsoff );   }
     else if( strcmp( yytext, "$dumpportson"    ) == 0 ) { return( V_dumpportson );    }
     else if( strcmp( yytext, "$dumpportsall"   ) == 0 ) { return( V_dumpportsall );   }
     else if( strcmp( yytext, "$vcdclose"       ) == 0 ) { return( V_vcdclose );       }
     else {
       yylval.text = strdup( yytext );
       return( VALUE );
     }
 }

  /* Identifiers */
<DEFINITION>[a-zA-Z_][a-zA-Z0-9_]* {
     if(      strcmp( yytext, "event"     ) == 0 ) { return( V_event );     }
     else if( strcmp( yytext, "parameter" ) == 0 ) { return( V_parameter ); }
     else if( strcmp( yytext, "integer"   ) == 0 ) { return( V_integer );   }
     else if( strcmp( yytext, "real"      ) == 0 ) { return( V_real );      }
     else if( strcmp( yytext, "reg"       ) == 0 ) { return( V_reg );       }
     else if( strcmp( yytext, "supply0"   ) == 0 ) { return( V_supply0 );   }
     else if( strcmp( yytext, "supply1"   ) == 0 ) { return( V_supply1 );   }
     else if( strcmp( yytext, "time"      ) == 0 ) { return( V_time );      }
     else if( strcmp( yytext, "tri"       ) == 0 ) { return( V_tri );       }
     else if( strcmp( yytext, "triand"    ) == 0 ) { return( V_triand );    }
     else if( strcmp( yytext, "trior"     ) == 0 ) { return( V_trior );     }
     else if( strcmp( yytext, "trireg"    ) == 0 ) { return( V_trireg );    }
     else if( strcmp( yytext, "tri0"      ) == 0 ) { return( V_tri0 );      }
     else if( strcmp( yytext, "tri1"      ) == 0 ) { return( V_tri1 );      }
     else if( strcmp( yytext, "wand"      ) == 0 ) { return( V_wand );      }
     else if( strcmp( yytext, "wire"      ) == 0 ) { return( V_wire );      }
     else if( strcmp( yytext, "wor"       ) == 0 ) { return( V_wor );       }
     else if( strcmp( yytext, "port"      ) == 0 ) { return( V_port );      } 
     else if( strcmp( yytext, "module"    ) == 0 ) { return( V_module );    }
     else if( strcmp( yytext, "task"      ) == 0 ) { return( V_task );      }
     else if( strcmp( yytext, "function"  ) == 0 ) { return( V_function );  }
     else if( strcmp( yytext, "begin"     ) == 0 ) { return( V_begin );     }
     else if( strcmp( yytext, "fork"      ) == 0 ) { return( V_fork );      }
     else {
       yylval.text = strdup( yytext );
       return IDENTIFIER;
     }
 }

  /* Hierarchical signal identifiers */
<DEFINITION>[a-zA-Z_][a-zA-Z0-9$_]*(\.[a-zA-Z_][a-zA-Z0-9$_]*)+ {
     yylval.text = strdup( yytext );
     return HIDENTIFIER;
 }
  
  /* Signal names starting with a '\' character */
<DEFINITION>\\[^ \t\b\f\r]+  { 
     yylval.text = strdup( yytext + 1 );
     return IDENTIFIER; 
 }

 /* Change value */
<SIMULATION>[01xXzZ][!-~0-9]+ {
     db_set_symbol_char( yytext + 1, yytext[0] );
     return CHANGE;
 }

<SIMULATION>[bB]?[01xXzZ]+{WSPACE}[!-~0-9]+ {
     int        bits = strlen( yytext );
     vector*    vec;
     int        i;
     char*      ptr;
     char*      sym;
     int        sig_size;
     int        value_size;

     value_size = 0;
     ptr        = yytext + 1;

     while( (ptr < (yytext + bits - 1)) && (*ptr != ' ') ) { 
       ptr++;
     }

     assert( ptr != (yytext + bits - 1) );

     *ptr = '\0';
     ptr++;

     db_set_symbol_string( ptr, (yytext + 1) );

     return CHANGE;
 }

<DEFINITION>\[[0-9]+\] {
     char* pos = (char*)malloc_safe( strlen( yytext ) - 1 );
     int   i;
     for( i=1; i<(strlen( yytext ) - 1); i++ ) {
       pos[i-1] = yytext[i];
     }
     pos[i-1] = '\0';
     yylval.sigwidth->width = 1;
     yylval.sigwidth->lsb   = atol( pos );
     free_safe( pos );
     return SIGWIDTH;
 }

<DEFINITION>\[[0-9]+\:[0-9]+\] {
     char* pos1 = (char*)malloc_safe( strlen( yytext ) - 3 );
     char* pos2 = (char*)malloc_safe( strlen( yytext ) - 3 );
     int   i    = 1;
     int   j;
     while( (i < (strlen( yytext ) - 1)) && (yytext[i] != ':') ) {
       pos1[i-1] = yytext[i];
       i++;
     }
     if( i == (strlen( yytext ) - 1) ) {
       fprintf( stderr, "Incorrectly formatted multi-bit select\n" );
       exit( 1 );
     } else {
       pos1[i-1] = '\0';
     }	
     j = i;
     while( i < (strlen( yytext ) - 1) ) {
       pos2[i-j] = yytext[i];
       i++;
     }
     pos2[i-j] = '\0';
     if( atol( pos1 ) >= atol( pos2 ) ) {
       yylval.sigwidth->width = atol( pos1 ) - atol( pos2 );
       yylval.sigwidth->lsb   = atol( pos2 );
     } else {
       yylval.sigwidth->width = atol( pos2 ) - atol( pos1 );
       yylval.sigwidth->lsb   = atol( pos1 );
     }
     free_safe( pos1 );
     free_safe( pos2 );
     return SIGWIDTH;
 }

  /* Lex out timestamps as these will otherwise be confused with strings. */
<SIMULATION>\#[0-9]+  {
     yylval.number = atol( yytext + 1 );
     return TIMESTAMP;
 }

<DEFINITION>[^ \n\t\b\f\r]*   { 
     yylval.text = strdup( yytext );
     return( VALUE );
 }

%%

void reset_vcd_lexer( char* vcd ) {

  vcdfile = strdup( vcd );

  if( (yyin = fopen( vcdfile, "r" )) == NULL ) {
    perror( vcdfile );
    exit( 1 );
  }

}
