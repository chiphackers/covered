%{
/*!
 \file     lexer.l
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/2/2001
 \brief    Lexer for Verilog language
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "defines.h"
#include "signal.h"
#include "parser_misc.h"
#include "parser.h"
#include "util.h"
#include "link.h"
#include "vector.h"
#include "keywords.h"
#include "tree.h"

#define yylval VLlval
#define YYDEBUG 1
#define YYERROR_VERBOSE 1

extern YYLTYPE yylloc;

extern void reset_pplexer( char* filename, FILE* out );

extern int   ignore_mode;
extern char  user_msg[USER_MSG_LENGTH];
extern char* ppfilename;

/*!
 Contains state before entering comment block.
*/
static int  comment_enter;
static void line_directive();
static void line_directive2();
static int  yywrap();

static str_link* filelist_head = 0;
static str_link* filelist_curr = 0;
static int       last_keyword  = 0;
static tnode*    found_modules = NULL;

extern str_link* modlist_head;
extern str_link* modlist_tail;

%}

%option stack

%x LCOMMENT
%x CCOMMENT
%x PCOMMENT
%x CSTRING
%x DEFINE
%x INCLUDE
%x ERROR_LINE
%x IGNORE_MODULE
%s UDPTABLE

%x IFDEF_FALSE
%s IFDEF_TRUE
%x IFDEF_SUPR

%x PPTIMESCALE

WSPACE [ \t\b\f]+

%%

  /* Whitespace -- we ignore this */
[ \t\b\f\r]   { ; }

  /* Newline character - blank line -- we ignore these */
\n            { yylloc.first_line += 1; }

  /* Handle line directives */
^"#line"[ ]+\"[^\"]*\"[ ]+[0-9]+.* { line_directive(); }
^"`line"[ ]+[0-9]+[ ]+\"[^\"]*\".* { line_directive2(); }

  /* Single-line comment -- we ignore this information */
"//".*        { comment_enter = YY_START;  BEGIN( LCOMMENT );    }
<LCOMMENT>.   { yymore();                                        }
<LCOMMENT>\n  { yylloc.first_line += 1;  BEGIN( comment_enter ); }

  /* Multi-line comment */
"/*"           { comment_enter = YY_START;  BEGIN( CCOMMENT ); }
<CCOMMENT>.    { yymore();                                     }
<CCOMMENT>\n   { yylloc.first_line += 1;  yymore();            }
<CCOMMENT>"*/" { BEGIN( comment_enter );                       }

  /* Pragma comment */
"(*"           { comment_enter = YY_START;  BEGIN( PCOMMENT ); }
<PCOMMENT>.    { yymore();                                     }
<PCOMMENT>\n   { yylloc.first_line += 1;  yymore();            }
<PCOMMENT>"*)" { BEGIN( comment_enter );                       }

  /* Multi-character symbols */
"(*"  { return K_PSTAR;   }
"*)"  { return K_STARP;   }
"<<"  { return K_LS;      }
">>"  { return K_RS;      }
"<="  { return K_LE;      }
">="  { return K_GE;      }
"=="  { return K_EQ;      }
"!="  { return K_NE;      }
"===" { return K_CEQ;     }
"!==" { return K_CNE;     }
"||"  { return K_LOR;     }
"&&"  { return K_LAND;    }
"&&&" { return K_TAND;    }
"~|"  { return K_NOR;     }
"~^"  { return K_NXOR;    }
"^~"  { return K_NXOR;    }
"~&"  { return K_NAND;    }
"->"  { return K_TRIGGER; }
"+:"  { return K_PO_POS;  }
"-:"  { return K_PO_NEG;  }
"=>"  { return K_EG;      }
"*>"  { return K_SG;      }

  /* Single character symbols */
[}{;:\[\],()#=.@&!?<>%|^~+*/-] { return yytext[0]; }

  /* C-style strings */
\"            { BEGIN( CSTRING ); }
<CSTRING>\\\" { yymore();         }
<CSTRING>\n   { BEGIN( 0 );
                yylval.text = strdup( yytext );
                /* VLerror( yylloc, "Missing close quote of string." ); */
                yylloc.first_line += 1;
                return 0; }
<CSTRING>\"   { BEGIN( 0 );
                yylval.text = strdup( yytext );
                yylval.text[ strlen( yytext ) - 1 ] = '\0';
                return STRING; }
<CSTRING>.    { yymore();      }

  /* Ignore modules that are not needed by the parser. */
<IGNORE_MODULE>endmodule    { BEGIN( 0 ); 
			      return I_endmodule; }
<IGNORE_MODULE>endprimitive { BEGIN( 0 );
                              return( K_endprimitive ); }
<IGNORE_MODULE>\n           { yylloc.first_line += 1;  
			      yymore();           }
<IGNORE_MODULE>.            { yymore();           }

  /* Entries for user defined primitive tables -- not currently supported */
<UDPTABLE>\(\?0\)    { return '_';       }
<UDPTABLE>\(\?1\)    { return '+';       }
<UDPTABLE>\(\?[xX]\) { return '%';       }
<UDPTABLE>\(\?\?\)   { return '*';       }
<UDPTABLE>\(01\)     { return 'r';       }
<UDPTABLE>\(0[xX]\)  { return 'Q';       }
<UDPTABLE>\(0\?\)    { return 'P';       }
<UDPTABLE>\(10\)     { return 'f';       }
<UDPTABLE>\(1[xX]\)  { return 'M';       }
<UDPTABLE>\(1\?\)    { return 'N';       }
<UDPTABLE>\([xX]0\)  { return 'F';       }
<UDPTABLE>\([xX]1\)  { return 'R';       }
<UDPTABLE>\([xX]\?\) { return 'B';       }
<UDPTABLE>[bB]       { return 'b';       }
<UDPTABLE>[lL]       { return 'l';       }
<UDPTABLE>[hH]       { return 'h';       }
<UDPTABLE>[fF]       { return 'f';       }
<UDPTABLE>[rR]       { return 'r';       }
<UDPTABLE>[xX]       { return 'x';       }
<UDPTABLE>[nN]       { return 'n';       }
<UDPTABLE>[pP]       { return 'p';       }
<UDPTABLE>[01\?\*\-] { return yytext[0]; }

  /* Keyword and signal identifiers */
[a-zA-Z_][a-zA-Z0-9$_]* {
     str_link* foundmod;
     int rc = lexer_keyword_code( yytext, yyleng );
     if( rc == IDENTIFIER ) {
       if( ignore_mode == 0 ) {
         yylval.text  = strdup( yytext );
         if( strncmp( yylval.text, "PATHPULSE$", 10) == 0 ) {
           rc = PATHPULSE_IDENTIFIER;
         }
       } else {
         if( strncmp( yytext, "PATHPULSE$", 10) == 0 ) {
           rc = UNUSED_PATHPULSE_IDENTIFIER;
         } else {
           rc = UNUSED_IDENTIFIER;
         }
       }
       if( (last_keyword == K_module) || (last_keyword == K_primitive) ) {
         if( (foundmod = str_link_find( yytext, modlist_head )) == NULL ) {
           BEGIN( IGNORE_MODULE );
           tree_add( yytext, yylloc.text, FALSE, &found_modules );
           rc = IGNORE;
         } else {
           if( filelist_curr != filelist_head ) {
             /* The module has been found, recirculate filelist */
             snprintf( user_msg, USER_MSG_LENGTH, "Parsing file '%s'", yylloc.text );
             print_output( user_msg, NORMAL );
             filelist_curr = filelist_head;
           }
           last_keyword = 0;
         }
       }
     } else {
       last_keyword = rc;
       yylval.text = '\0';
     }
     return( rc );
 }

  /* Signal names starting with a '\' character */
\\[^ \t\b\f\r\n]+  { 
     if( ignore_mode == 0 ) {
       yylval.text = strdup( yytext );
       return IDENTIFIER; 
     } else {
       return UNUSED_IDENTIFIER;
     }
 }

  /* System call lines -- we ignore this information */
\$([a-zA-Z0-9$_]+)   { 
     if( ignore_mode == 0 ) {
       return SYSTEM_IDENTIFIER; 
     } else {
       return UNUSED_SYSTEM_IDENTIFIER;
     }
 }

  /* Decimal number - Size indicated */
[0-9][0-9_]*[ \t]*\'[sS]?[dD][ \t]*[0-9][0-9_]* {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Binary number - Size indicated */
[0-9][0-9_]*[ \t]*\'[sS]?[bB][ \t]*[0-1xzXZ_\?]+ {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Octal number - Size indicated */
[0-9][0-9_]*[ \t]*\'[sS]?[oO][ \t]*[0-7xzXZ_\?]+ {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Hexidecimal number - Size indicated */
[0-9][0-9_]*[ \t]*\'[sS]?[hH][ \t]*[0-9a-fA-FxzXZ_\?]+ {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Decimal number - unsized */
\'[sS]?[dD][ \t]*[0-9][0-9_]* {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Binary number - unsized */
\'[sS]?[bB][ \t]*[0-1xzXZ_\?]+ {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Octal number - unsized */
\'[sS]?[oO][ \t]*[0-7xzXZ_\?]+ {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Hexidecimal number - unsized */
\'[sS]?[hH][ \t]*[0-9a-fA-FxzXZ_\?]+ {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Unsized decimal number */
[0-9][0-9_]* {
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( yytext );
       assert( yylval.number != NULL );
       return NUMBER;
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Real numbers -- we ignore this information */
[0-9][0-9_]*\.[0-9][0-9_]*([Ee][+-]?[0-9][0-9_]*)? { 
     if( ignore_mode == 0 ) {
       return REALTIME;
     } else {
       return UNUSED_REALTIME;
     }
 }

  /* Real numbers -- we ignore this information */
[0-9][0-9_]*[Ee][+-]?[0-9][0-9_]* { 
     if( ignore_mode == 0 ) {
       return REALTIME;
     } else {
       return UNUSED_REALTIME;
     }
 }

  /* Timescale directive -- we ignore this information */
^{WSPACE}?`timescale { BEGIN( PPTIMESCALE ); }
<PPTIMESCALE>.       { yymore();             }
<PPTIMESCALE>\n      {
     yylloc.first_line += 1;
     BEGIN( 0 );
 }

  /* Miscellaneous directives -- we ignore this information */
^{WSPACE}?`celldefine{WSPACE}?.*             { }
^{WSPACE}?`default_nettype{WSPACE}?.*        { }
^{WSPACE}?`delay_mode_distributed{WSPACE}?.* { }
^{WSPACE}?`delay_mode_unit{WSPACE}?.*        { }
^{WSPACE}?`delay_mode_path{WSPACE}?.*        { }
^{WSPACE}?`disable_portfaults{WSPACE}?.*     { }
^{WSPACE}?`enable_portfaults{WSPACE}?.*      { }
^{WSPACE}?`endcelldefine{WSPACE}?.*          { }
^{WSPACE}?`endprotect{WSPACE}?.*             { }
^{WSPACE}?`nosuppress_faults{WSPACE}?.*      { }
^{WSPACE}?`nounconnected_drive{WSPACE}?.*    { }
^{WSPACE}?`protect{WSPACE}?.*                { }
^{WSPACE}?`resetall{WSPACE}?.*               { }
^{WSPACE}?`suppress_faults{WSPACE}?.*        { }
^{WSPACE}?`unconnected_drive{WSPACE}?.*      { }
^{WSPACE}?`uselib{WSPACE}?.*                 { }

  /* Handle lines with errors */
<ERROR_LINE>.* { yy_pop_state(); }

  /* Final catchall. something got lost or mishandled. */
. { fprintf( stderr, "%d: unmatched character (%c)\n", yylloc.first_line, yytext[0] ); }

%%

void lex_start_udp_table() {

  BEGIN( UDPTABLE );

}

void lex_end_udp_table() {

  BEGIN( INITIAL );

}

static void line_directive() {

  char* qt1;
  char* qt2;
  char* buf;
  
  qt1 = strchr( yytext, '"');
  assert( qt1 );
  qt1 += 1;
  
  qt2 = strchr( qt1, '"' );
  assert( qt2 );
  
  buf = (char*)malloc_safe( qt2 - qt1 + 1 );
  strncpy( buf, qt1, qt2-qt1 );
  buf[qt2-qt1] = '\0';
  
  yylloc.text = buf;
  
  qt2 += 1;
  yylloc.first_line = strtoul( qt2, 0, 0 );
  
}

static void line_directive2() {

  char* cp;
  char* qt1;
  char* qt2;
  char* buf;
  
  assert( strncmp( yytext, "`line", 5 ) == 0 );
  
  cp = yytext + strlen( "`line" );
  cp += strspn( cp, " " );
  yylloc.first_line = strtoul( cp, &cp, 10 ) - 1;
  
  cp += strspn( cp, " " );
  if( *cp == 0 ) return;
  
  qt1 = strchr( yytext, '"' );
  assert( qt1 );
  qt1 += 1;
  
  qt2 = strchr( qt1, '"' );
  assert( qt2 );
  
  buf = (char*)malloc_safe( qt2 - qt1 + 1 );
  strncpy( buf, qt1, qt2 - qt1 );
  buf[qt2-qt1] = '\0';
  
  yylloc.text = buf;
  
}
  
/*
 * The lexical analyzer calls this function when the current file
 * ends. Here I pop the include stack and resume the previous file. If
 * there is no previous file, then the main input is ended.
 */
static int yywrap() {

  str_link* curr;
  str_link* tmpm;
  FILE*     out;
  bool      found = FALSE;
  tnode*    node;
  
  /* Close input file */
  if( yyin != 0 ) {
    fclose( yyin );
  }
  
  if( modlist_head == NULL ) {
    assert( unlink( ppfilename ) == 0 );
    tree_dealloc( found_modules );
    return( 1 );
  }
    
  curr = filelist_curr;
  tmpm = modlist_head;
  
  while( (tmpm != NULL) && !found ) {
    if( (node = tree_find( tmpm->str, found_modules )) != NULL ) {
      curr->str = node->value;
      found     = TRUE;
    } else {
      if( (curr = get_next_vfile( curr, tmpm->str )) == NULL ) {
        tmpm = tmpm->next;
        curr = filelist_curr;
      } else {
        found = TRUE;
      }
    }
  }

  /* If the rest of the modules could not be found, display them to the user */
  if( (modlist_head != NULL) && (tmpm == NULL) ) {
    print_output( "Unable to find the following modules:", FATAL );
    curr = modlist_head;
    while( curr != NULL ) {
      fprintf( stderr, "      %s\n", curr->str );
      curr = curr->next;
    }
    assert( unlink( ppfilename ) == 0 );
    exit( 1 );
  }
  
  /* Create temporary output filename */
  out = fopen( ppfilename, "w" );
  assert( out != NULL );
  
  /* Now the preprocessor on this file first */
  reset_pplexer( curr->str, out );
  PPVLlex();
  
  fclose( out );
  
  yyrestart( fopen( ppfilename, "r" ) );
  
  yylloc.first_line = 1;
  yylloc.text       = curr->str;
  
  /* Set bit to allow this library file to  */
  curr->suppl = (curr->suppl | 0x4);
  
  filelist_curr = curr->next;
  
  return( 0 );

}

/*
 * This function initializes the whole process. The first file is
 * opened, and the lexor is initialized. The include stack is cleared
 * and ready to go.
 */
void reset_lexer( str_link* file_list_head ) {

  FILE*     out;
  str_link* curr = file_list_head;

  if( (curr = get_next_vfile( curr, modlist_head->str )) == NULL ) {
    print_output( "No verilog files specified", FATAL );
    exit( 1 );
  }

  assert( curr->str != NULL );
  
  /* Create temporary output filename */
  if( ppfilename == NULL ) {
    ppfilename = (char*)malloc_safe( 10 );
    snprintf( ppfilename, 10, "tmpXXXXXX" );
    assert( mkstemp( ppfilename ) != 0 );
  }
  out = fopen( ppfilename, "w" );
  if( out == NULL ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Unable to open temporary file %s for writing", ppfilename );
    print_output( user_msg, FATAL );
    exit( 1 );
  }
  
  /* Now the preprocessor on this file first */
  reset_pplexer( curr->str, out );
  PPVLlex();
  
  fclose( out );
  
  yyrestart( fopen( ppfilename, "r" ) );

  yylloc.text       = curr->str;
  yylloc.first_line = 1;
  
  filelist_head = file_list_head;
  filelist_curr = file_list_head->next;

}

