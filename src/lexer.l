%{
/*
 Copyright (c) 2006 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*!
 \file     lexer.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/2/2001
 \brief    Lexer for Verilog language
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "defines.h"
#include "vsignal.h"
#include "parser_misc.h"
#include "parser.h"
#include "util.h"
#include "link.h"
#include "vector.h"
#include "keywords.h"
#include "tree.h"
#include "gen_item.h"
#include "obfuscate.h"
#include "db.h"

#define yylval VLlval
#define YYDEBUG 1
#define YYERROR_VERBOSE 1

extern YYLTYPE yylloc;

extern void reset_pplexer( char* filename, FILE* out );
extern int PPVLlex( void );

extern int        ignore_mode;
extern int        generate_mode;
extern int        attr_mode;
extern char       user_msg[USER_MSG_LENGTH];
extern char*      ppfilename;
extern str_link*  modlist_head;
extern str_link*  modlist_tail;
extern func_unit* curr_funit;
extern sig_range  curr_prange;
extern sig_range  curr_urange;
extern gen_item*  last_gi;

/*!
 Contains state before entering exclusive block.
*/
static int  state_enter;
static void line_directive();
static void line_directive2();
bool process_timescale( const char* txt, bool report );
static int  yywrap();

/*!
 Temporary string container.
*/
static char* tmpstr;

/*!
 Pointer to head of file list to parse.
*/
static str_link* filelist_head = 0;

/*!
 Pointer to tail of file list to parse.
*/
static str_link* filelist_curr = 0;

/*!
 Integer value of last keyword parsed.
*/
static int       last_keyword  = 0;

/*!
 Tree containing all found modules parsed within the design.
*/
static tnode*    found_modules = NULL;

/*!
 Holds the name of the file specified in a `line or #line preprocessor directive.
*/
char*            directive_filename = NULL;

/*!
 Specifies if the current file was previously parsed.  If it was, we need to ignore
 all globally declared stuff.
*/
static bool      file_prev_parsed;

%}

%option stack

%x LCOMMENT
%x CCOMMENT
%x CSTRING
%x ERROR_LINE
%s IGNORE_MODULE
%s IGNORE_ASSERT
%s IGNORE_PROPERTY
%s IGNORE_SEQUENCE
%s IGNORE_GLOBALS
%s IGNORE_PROGRAM
%s PARSE
%s UDPTABLE
%s TIMEUNIT
%s LIBOPTIONS
%s IMEXPORT
%x PPTIMESCALE

WSPACE [ \t\b\f]+

%%

  /* Whitespace -- we ignore this */
[ \t\b\f\r]   {
	yylloc.last_column += strlen( yytext );
 }

  /* Newline character - blank line -- we ignore these */
\n {
	yylloc.first_line += 1;
	yylloc.last_column = 0;
 }

  /* Handle line directives */
^"#line"[ ]+\"[^\"]*\"[ ]+[0-9]+.* {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += strlen( yytext );
	line_directive();
 }

^"`line"[ ]+[0-9]+[ ]+\"[^\"]*\".* {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += strlen( yytext );
	line_directive2();
 }

  /* Single-line comment -- we ignore this information */
"//".*        {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += strlen( yytext );
	state_enter         = YY_START;
	BEGIN( LCOMMENT );
 }
<LCOMMENT>.   {
	yylloc.last_column += 1;
	yymore();
 }
<LCOMMENT>\n  {
	BEGIN( state_enter );
	yylloc.first_line += 1;
	yylloc.last_column = 0;
 }

  /* Multi-line comment */
"/*"           {
	state_enter         = YY_START;
	BEGIN( CCOMMENT );
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += 2;
 }
<CCOMMENT>.    {
	yylloc.last_column += 1;
	yymore();
 }
<CCOMMENT>\n   {
	yylloc.first_line += 1;
	yylloc.last_column = 0;
	yymore();
 }
<CCOMMENT>"*/" {
	yylloc.last_column += 2;
	BEGIN( state_enter );
 }

  /* Multi-character symbols */
<PARSE>"(*"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_PSTAR;   }
<PARSE>"*)"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_STARP;   }
<PARSE>"<<"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_LS;      }
<PARSE>"<<<"  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_LSS;     }
<PARSE>">>"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_RS;      }
<PARSE>">>>"  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_RSS;     }
<PARSE>"<="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_LE;      }
<PARSE>">="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_GE;      }
<PARSE>"=="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_EQ;      }
<PARSE>"!="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_NE;      }
<PARSE>"==="  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_CEQ;     }
<PARSE>"!=="  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_CNE;     }
<PARSE>"=?="  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_PEQ;     }
<PARSE>"!?="  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_PNE;     }
<PARSE>"||"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_LOR;     }
<PARSE>"&&"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_LAND;    }
<PARSE>"&&&"  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_TAND;    }
<PARSE>"~|"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_NOR;     }
<PARSE>"~^"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_NXOR;    }
<PARSE>"^~"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_NXOR;    }
<PARSE>"~&"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_NAND;    }
<PARSE>"->"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_TRIGGER; }
<PARSE>"+:"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_PO_POS;  }
<PARSE>"-:"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_PO_NEG;  }
<PARSE>"=>"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_EG;      }
<PARSE>"*>"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_SG;      }
<PARSE>".*"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_PS;      }
<PARSE>"##"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_PP;      }
<PARSE>"+="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_ADD_A;   }
<PARSE>"-="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_SUB_A;   }
<PARSE>"*="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_MLT_A;   }
<PARSE>"/="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_DIV_A;   }
<PARSE>"%="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_MOD_A;   }
<PARSE>"&="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_AND_A;   }
<PARSE>"|="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_OR_A;    }
<PARSE>"^="   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_XOR_A;   }
<PARSE>"<<="  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_LS_A;    }
<PARSE>">>="  { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 3;  return K_RS_A;    }
<PARSE>"<<<=" { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 4;  return K_ALS_A;   }
<PARSE>">>>=" { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 4;  return K_ARS_A;   }
<PARSE>"++"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_INC;     }
<PARSE>"--"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_DEC;     }
<PARSE>"**"   { yylloc.first_column = yylloc.last_column;  yylloc.last_column += 2;  return K_POW;     }

  /* Properly handle all cases of (*) and convert it to a simple '*' */
<PARSE>"("{WSPACE}*"*"{WSPACE}*")" { yylloc.first_column = yylloc.last_column;  yylloc.last_column += strlen( yytext );  return '*'; }

  /* Single character symbols */
<PARSE>[}{;:\[\],()#=.@&!?<>%|^~+*/-] {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += 1;
        if( (yytext[0] == ';') || (yytext[0] == ',') ) {
          curr_prange.clear = TRUE;
          curr_urange.clear = TRUE;
        }
	return yytext[0];
 }

  /* C-style strings */
<PARSE>\"            {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += 1;
        state_enter         = YY_START;
	BEGIN( CSTRING );
 }
<CSTRING>\\\" {
	yylloc.last_column += 2;
	yymore();
 }
<CSTRING>\n   {
	yylloc.last_column = 0;
	BEGIN( state_enter );
        yylloc.first_line += 1;
        return 0;
 }
<CSTRING>\"   { PROFILE(LEXER_CSTRING_A);
	yylloc.last_column += 1;
	BEGIN( state_enter );
        if( ignore_mode == 0 ) {
          yylval.text = strdup_safe( yytext );
          yylval.text[ strlen( yytext ) - 1 ] = '\0';
          tmpstr = yylval.text;
          yylval.number = vector_from_string( &(yylval.text), TRUE );
          free_safe( tmpstr );
          return STRING;
        } else {
          return UNUSED_STRING;
        }
 }
<CSTRING>.    {
	yylloc.last_column += 1;
	yymore();
 }

  /* Ignore modules that are not needed by the parser. */
<IGNORE_MODULE>endmodule    {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += strlen( yytext );
	if( file_prev_parsed ) {
          BEGIN( IGNORE_GLOBALS );
        } else {
  	  BEGIN( PARSE );
 	}
        return I_endmodule;
 }
<IGNORE_MODULE>endprimitive {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += strlen( yytext );
	if( file_prev_parsed ) {
	  BEGIN( IGNORE_GLOBALS );
	} else {
	  BEGIN( PARSE );
	}
        return( K_endprimitive );
 }
<IGNORE_MODULE>. {
 }

  /* Used when we have previously parsed a file and want to ignore all global variables (SystemVerilog mode only) */
<IGNORE_GLOBALS>module {
	yyless( 0 );
	BEGIN( PARSE );
 }
<IGNORE_GLOBALS>primitive {
	yyless( 0 );
	BEGIN( PARSE );
 }

  /* Ignore assertion syntax */
<IGNORE_ASSERT>; {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += strlen( yytext );
	BEGIN( PARSE );
	return ';';
 }
<IGNORE_ASSERT>. {
	yylloc.last_column += 1;
        yymore();
 }

  /* Ignore property syntax */
<IGNORE_PROPERTY>endproperty {
        yylloc.first_column = yylloc.last_column;
        yylloc.last_column += strlen( yytext );
        BEGIN( PARSE );
        return K_endproperty;
 }
<IGNORE_PROPERTY>. {
        yylloc.last_column += 1;
        yymore();
 }

  /* Ignore sequence syntax */
<IGNORE_SEQUENCE>endsequence {
        yylloc.first_column = yylloc.last_column;
        yylloc.last_column += strlen( yytext );
        BEGIN( PARSE );
        return K_endsequence;
 }
<IGNORE_SEQUENCE>. {
        yylloc.last_column += 1;
        yymore();
 }

  /* Ignore program syntax */
<IGNORE_PROGRAM>endprogram {
        yylloc.first_column = yylloc.last_column;
        yylloc.last_column += strlen( yytext );
        BEGIN( PARSE );
        return K_endprogram;
 }
<IGNORE_PROGRAM>. {
        yylloc.last_column += 1;
        yymore();
 }

  /* Entries for user defined primitive tables -- not currently supported */
<UDPTABLE>\(\?0\)    { yylloc.last_column += strlen( yytext );  return '_';       }
<UDPTABLE>\(\?1\)    { yylloc.last_column += strlen( yytext );  return '+';       }
<UDPTABLE>\(\?[xX]\) { yylloc.last_column += strlen( yytext );  return '%';       }
<UDPTABLE>\(\?\?\)   { yylloc.last_column += strlen( yytext );  return '*';       }
<UDPTABLE>\(01\)     { yylloc.last_column += strlen( yytext );  return 'r';       }
<UDPTABLE>\(0[xX]\)  { yylloc.last_column += strlen( yytext );  return 'Q';       }
<UDPTABLE>\(0\?\)    { yylloc.last_column += strlen( yytext );  return 'P';       }
<UDPTABLE>\(10\)     { yylloc.last_column += strlen( yytext );  return 'f';       }
<UDPTABLE>\(1[xX]\)  { yylloc.last_column += strlen( yytext );  return 'M';       }
<UDPTABLE>\(1\?\)    { yylloc.last_column += strlen( yytext );  return 'N';       }
<UDPTABLE>\([xX]0\)  { yylloc.last_column += strlen( yytext );  return 'F';       }
<UDPTABLE>\([xX]1\)  { yylloc.last_column += strlen( yytext );  return 'R';       }
<UDPTABLE>\([xX]\?\) { yylloc.last_column += strlen( yytext );  return 'B';       }
<UDPTABLE>[bB]       { yylloc.last_column += strlen( yytext );  return 'b';       }
<UDPTABLE>[lL]       { yylloc.last_column += strlen( yytext );  return 'l';       }
<UDPTABLE>[hH]       { yylloc.last_column += strlen( yytext );  return 'h';       }
<UDPTABLE>[fF]       { yylloc.last_column += strlen( yytext );  return 'f';       }
<UDPTABLE>[rR]       { yylloc.last_column += strlen( yytext );  return 'r';       }
<UDPTABLE>[xX]       { yylloc.last_column += strlen( yytext );  return 'x';       }
<UDPTABLE>[nN]       { yylloc.last_column += strlen( yytext );  return 'n';       }
<UDPTABLE>[pP]       { yylloc.last_column += strlen( yytext );  return 'p';       }
<UDPTABLE>[zZ]       { yylloc.last_column += strlen( yytext );  return 'z';       }
<UDPTABLE>[01\?\*\-] { yylloc.last_column += strlen( yytext );  return yytext[0]; }

  /* Entries for time units */
<TIMEUNIT>step { yylloc.last_column += strlen( yytext );  return K_TU_STEP; }
<TIMEUNIT>s    { yylloc.last_column += strlen( yytext );  return K_TU_S; }
<TIMEUNIT>ms   { yylloc.last_column += strlen( yytext );  return K_TU_MS; }
<TIMEUNIT>us   { yylloc.last_column += strlen( yytext );  return K_TU_US; }
<TIMEUNIT>ns   { yylloc.last_column += strlen( yytext );  return K_TU_NS; }
<TIMEUNIT>ps   { yylloc.last_column += strlen( yytext );  return K_TU_PS; }
<TIMEUNIT>fs   { yylloc.last_column += strlen( yytext );  return K_TU_FS; }

  /* Library options */
<LIBOPTIONS>-incdir { yylloc.last_column += strlen( yytext );  return K_INCDIR; }

  /* Import/Export options */
<IMEXPORT>\"DPI\" { yylloc.last_column += strlen( yytext );  return K_DPI; }

  /* Keyword and signal identifiers */
<PARSE>[a-zA-Z_][a-zA-Z0-9$_]*(\[[^]]*\]\.[a-zA-Z_][a-zA-Z0-9$_]*)* { PROFILE(LEXER_KEYWORD_A);
     str_link* foundmod;
     int       rc   = lexer_keyword_1995_code( yytext, yyleng );
     if( (rc == IDENTIFIER) && parser_check_generation( GENERATION_2001 ) ) {
       rc = lexer_keyword_2001_code( yytext, yyleng );
     }
     if( (rc == IDENTIFIER) && parser_check_generation( GENERATION_SV ) ) {
       rc = lexer_keyword_sv_code( yytext, yyleng );
       if( (rc == IDENTIFIER) && (ignore_mode == 0) ) {
         if( (yylval.typdef = db_find_typedef( yytext )) != NULL ) {
           rc = TYPEDEF_IDENTIFIER;
         }
       }
     }
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( rc == IDENTIFIER ) {
       if( ignore_mode == 0 ) {
         if( (generate_mode == 0) && ((last_gi == NULL) || (last_gi->suppl.part.type != GI_TYPE_INST)) && (strchr( yytext, '.' ) != NULL) ) {
           yylval.text = gen_item_calc_signal_name( yytext, curr_funit, yylloc.first_line, TRUE );
         } else {
           yylval.text = strdup_safe( yytext );
         }
         // printf( "Allocated IDENTIFIER %p  %s, line %d\n", yylval.text, yylval.text, yylloc.first_line );
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
           free_safe( yylval.text );
           BEGIN( IGNORE_MODULE );
           (void)tree_add( yytext, yylloc.text, FALSE, &found_modules );
           rc = IGNORE;
         } else {
           if( (filelist_curr != filelist_head) && (error_count == 0) ) {
             /* The module has been found, recirculate filelist */
             snprintf( user_msg, USER_MSG_LENGTH, "Parsing file '%s'", obf_file( yylloc.text ) );
             print_output( user_msg, NORMAL, __FILE__, __LINE__ );
             filelist_curr = filelist_head;
           }
           last_keyword = 0;
         }
       }
     } else if( (rc == K_endmodule) || (rc == K_endprimitive) ) {
       if( file_prev_parsed ) {
         BEGIN( IGNORE_GLOBALS );
       }
     } else if( rc == K_assert ) {
       BEGIN( IGNORE_ASSERT );
     } else if( rc == K_property ) {
       BEGIN( IGNORE_PROPERTY );
     } else if( rc == K_sequence ) {
       BEGIN( IGNORE_SEQUENCE );
     } else if( rc == K_program ) {
       BEGIN( IGNORE_PROGRAM );
     } else if( rc != TYPEDEF_IDENTIFIER ) {
       last_keyword = rc;
       yylval.text = '\0';
     }
     return( rc );
 }

  /* Signal names starting with a '\' character */
<PARSE>\\[^ \t\b\f\r\n]+  { PROFILE(LEXER_ESCAPED_NAME);
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.text = (char*)malloc_safe( strlen( yytext ) + 2 );
       snprintf( yylval.text, (strlen( yytext ) + 2), "%s ", yytext );
       return IDENTIFIER; 
     } else {
       return UNUSED_IDENTIFIER;
     }
 }

  /* System call lines -- we ignore this information */
<PARSE>\$([a-zA-Z0-9$_]+)   { PROFILE(LEXER_SYSTEM_CALL);
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.text = strdup_safe( yytext );
       return SYSTEM_IDENTIFIER; 
     } else {
       return UNUSED_SYSTEM_IDENTIFIER;
     }
 }

  /* Decimal number - Size indicated */
<PARSE>[0-9][0-9_]*[ \t]*\'[sS]?[dD][ \t]*[0-9][0-9_]* {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Binary number - Size indicated */
<PARSE>[0-9][0-9_]*[ \t]*\'[sS]?[bB][ \t]*[0-1xzXZ_\?]+ {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Octal number - Size indicated */
<PARSE>[0-9][0-9_]*[ \t]*\'[sS]?[oO][ \t]*[0-7xzXZ_\?]+ {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Hexidecimal number - Size indicated */
<PARSE>[0-9][0-9_]*[ \t]*\'[sS]?[hH][ \t]*[0-9a-fA-FxzXZ_\?]+ {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Decimal number - unsized */
<PARSE>\'[sS]?[dD][ \t]*[0-9][0-9_]* {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Binary number - unsized */
<PARSE>\'[sS]?[bB][ \t]*[0-1xzXZ_\?]+ {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Octal number - unsized */
<PARSE>\'[sS]?[oO][ \t]*[0-7xzXZ_\?]+ {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Hexidecimal number - unsized */
<PARSE>\'[sS]?[hH][ \t]*[0-9a-fA-FxzXZ_\?]+ {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Unsized decimal number */
<PARSE>[0-9][0-9_]* {
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       yylval.number = vector_from_string( &yytext, FALSE );
       if( yylval.number != NULL ) {
         return NUMBER;
       } else {
         return UNUSED_NUMBER;
       }
     } else {
       return UNUSED_NUMBER;
     }
 }

  /* Real numbers -- we ignore this information */
<PARSE>[0-9][0-9_]*\.[0-9][0-9_]*([Ee][+-]?[0-9][0-9_]*)? { 
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       return REALTIME;
     } else {
       return UNUSED_REALTIME;
     }
 }

  /* Real numbers -- we ignore this information */
<PARSE>[0-9][0-9_]*[Ee][+-]?[0-9][0-9_]* { 
     yylloc.first_column = yylloc.last_column;
     yylloc.last_column += strlen( yytext );
     if( ignore_mode == 0 ) {
       return REALTIME;
     } else {
       return UNUSED_REALTIME;
     }
 }

  /* Timescale directive -- we ignore this information */
^{WSPACE}?`timescale {
	yylloc.first_column = yylloc.last_column;
	yylloc.last_column += strlen( yytext );
        state_enter         = YY_START;
	BEGIN( PPTIMESCALE );
 }
<PPTIMESCALE>.      {
        yylloc.first_column = yylloc.last_column;
        yylloc.last_column += strlen( yytext );
        yymore();
 }
<PPTIMESCALE>\n      {
     process_timescale( yytext, TRUE );
     yylloc.first_line += 1;
     BEGIN( state_enter );
     yylloc.last_column = 0;
 }

  /* Miscellaneous directives -- we ignore this information */
^{WSPACE}?`celldefine{WSPACE}?.*             { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`default_nettype{WSPACE}?.*        { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`delay_mode_distributed{WSPACE}?.* { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`delay_mode_unit{WSPACE}?.*        { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`delay_mode_path{WSPACE}?.*        { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`disable_portfaults{WSPACE}?.*     { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`enable_portfaults{WSPACE}?.*      { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`endcelldefine{WSPACE}?.*          { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`endprotect{WSPACE}?.*             { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`nosuppress_faults{WSPACE}?.*      { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`nounconnected_drive{WSPACE}?.*    { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`protect{WSPACE}?.*                { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`resetall{WSPACE}?.*               { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`suppress_faults{WSPACE}?.*        { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`unconnected_drive{WSPACE}?.*      { yylloc.last_column = strlen( yytext ); }
^{WSPACE}?`uselib{WSPACE}?.*                 { yylloc.last_column = strlen( yytext ); }

  /* Handle lines with errors */
<ERROR_LINE>.* {
	yy_pop_state();
 }

  /* Handle any characters in ignored space */
<IGNORE_GLOBALS,IGNORE_MODULE,IGNORE_ASSERT,IGNORE_PROPERTY,IGNORE_SEQUENCE>. {
	yylloc.last_column += 1;
	yymore();
 }

  /* Final catchall. something got lost or mishandled. */
. { fprintf( stderr, "%u: unmatched character (%c)\n", yylloc.first_line, yytext[0] ); }

%%

/*!
 Called at the start of UDP table parsing.
*/
void lex_start_udp_table() {

  BEGIN( UDPTABLE );

}

/*!
 Called at the end of UDP table parsing.
*/
void lex_end_udp_table() {

  BEGIN( INITIAL );

}

/*!
 Called at the start of time unit parsing.
*/
void lex_start_time_unit() {

  BEGIN( TIMEUNIT );

}

/*!
 Called at the end of time unit parsing.
*/
void lex_end_time_unit() {

  BEGIN( INITIAL );

}

/*!
 Called at the start of library_argument parsing.
*/
void lex_start_library_options() {

  BEGIN( LIBOPTIONS );

}

/*!
 Called at the end of library argument parsing.
*/
void lex_end_library_options() {

  BEGIN( INITIAL );

}

/*!
 Called at the start of import/export library parsing.
*/
void lex_start_import_export() {

  BEGIN( IMEXPORT );

}

/*!
 Called at the end of import/export library parsing.
*/
void lex_end_import_export() {

  BEGIN( INITIAL );

}


/*!
 Parses and handles a line directive that is in the CPP format.
*/
static void line_directive() { PROFILE(LINE_DIRECTIVE);

  char* qt1;
  char* qt2;
  
  qt1 = strchr( yytext, '"');
  assert( qt1 );
  qt1 += 1;
  
  qt2 = strchr( qt1, '"' );
  assert( qt2 );
  
  directive_filename = (char*)realloc( directive_filename, (qt2 - qt1 + 1) );
  strncpy( directive_filename, qt1, qt2-qt1 );
  directive_filename[qt2-qt1] = '\0';
  
  yylloc.text = directive_filename;
  
  qt2 += 1;
  yylloc.first_line = strtoul( qt2, 0, 0 );
  
}

/*!
 Parses and handles a line directive in the Verilog-2001 format.
*/
static void line_directive2() { PROFILE(LINE_DIRECTIVE2);

  char* cp;
  char* qt1;
  char* qt2;
  
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
  
  directive_filename = (char*)realloc( directive_filename, (qt2 - qt1 + 1) );
  strncpy( directive_filename, qt1, qt2 - qt1 );
  directive_filename[qt2-qt1] = '\0';
  
  yylloc.text = directive_filename;
  
}

/*!
 \param token   User-specified timescale token (either unit or precision information)
 \param type    String specifying timescale token type ("unit" or "precision")
 \param value   Integer value to store shift information in
 \param report  If set to TRUE, calls VLerror with appropriate error information

 Parses either the time unit or time precision token.
*/
bool process_timescale_token( char* token, char* type, int* value, bool report ) { PROFILE(PROCESS_TIMESCALE_TOKEN);

  bool retval = TRUE;         /* Return value for this function */
  int  i      = 3;            /* Loop iterator */
  char msg[USER_MSG_LENGTH];  /* Error message container */

  while( (i > 0) && (strncmp( token, "100", i ) != 0) ) i--;

  if( i == 0 ) {
    if( report ) {
      snprintf( msg, USER_MSG_LENGTH, "Illegal time %s value specified (must be 1, 10 or 100)", type );
      VLerror( msg );
    }
    retval = FALSE;
  } else {
    *value = i - 1;
    token += i;
    if(      strncmp(  "s", token, 1 ) == 0 ) { token += 1; }
    else if( strncmp( "ms", token, 2 ) == 0 ) { token += 2;  *value -= 3; }
    else if( strncmp( "us", token, 2 ) == 0 ) { token += 2;  *value -= 6; }
    else if( strncmp( "ns", token, 2 ) == 0 ) { token += 2;  *value -= 9; }
    else if( strncmp( "ps", token, 2 ) == 0 ) { token += 2;  *value -= 12; }
    else if( strncmp( "fs", token, 2 ) == 0 ) { token += 2;  *value -= 15; }
    else {
      if( report ) {
        snprintf( msg, USER_MSG_LENGTH, "Illegal time %s measurement specified (must be 's', 'ms', 'us', 'ns', 'ps' or 'fs')", type );
        VLerror( msg );
      }
      retval = FALSE;
    }
    if( retval && (*token != '\0') ) {
      if( report ) {
        VLerror( "Illegal timescale directive specified" );
      }
      retval = FALSE;
    }
  }

  return( retval );

}

/*!
 \param txt     String containing timescale information
 \param report  If set to TRUE, reports any error information to the VLerror function

 Processes a text string into timescale information.
*/
bool process_timescale( const char* txt, bool report ) { PROFILE(PROCESS_TIMESCALE);

  bool  retval = TRUE;   /* Return value for this function */
  char* str;             /* String version of text minus spaces */
  char* ptr;             /* Pointer to current token */
  int   i;               /* Loop iterator */
  int   num_spaces = 0;  /* Number of spaces found in given string */
  int   unit = 0;        /* Time unit offset */
  int   prec = 0;        /* Time precision offset */

  /* Copy the given string */
  str = strdup_safe( txt );

  /* Remove all spaces */
  for( i=0; i<(strlen( str )+1); i++ ) {
    if( (str[i] == ' ') || (str[i] == '\t') || (str[i] == '\n') ) {
      num_spaces++;
    } else {
      str[i-num_spaces] = str[i];
    }
  }

  /* Get timescale tokens */
  i = strcspn( str, "/" );
  if( i == strlen( str ) ) {

    if( report ) {
      VLerror( "Timescale missing '/' character" );
    }
    retval = FALSE;

  } else {

    /* Process the time unit and precision tokens */
    str[i] = '\0';
    if( (retval = process_timescale_token( str, "unit", &unit, report )) ) {
      retval = process_timescale_token( (str + i + 1), "precision", &prec, report );
    }

  }
  
  /* Check to make sure that the unit setting is greater than the precision */
  if( retval && (unit < prec) ) {
    if( report ) {
      VLerror( "Invalid timescale settings (unit must be greater than the precision)" );
    }
    retval = FALSE;
  }

  /* Handle the timescale with the database */
  if( retval ) {
    db_set_timescale( unit, prec );
  }

  /* Finally, deallocate memory */
  free_safe( str );

}


  
/*!
 \return Returns 0 if we should continue to parse or 1 if we should stop.

 The lexical analyzer calls this function when the current file
 ends.  Here I pop the include stack and resume the previous file.  If
 there is no previous file, then the main input is ended.
*/
static int yywrap() { PROFILE(LEXER_YYWRAP);

  str_link* curr;
  str_link* tmpm;
  char*     fname = NULL;
  FILE*     out;
  tnode*    node;
  
  /* Close input file */
  if( yyin != 0 ) {
    fclose( yyin );
  }
  
  /* Delete the current buffer */
  yy_delete_buffer( YY_CURRENT_BUFFER );
  
  /*
   If we have no more modules to find in our module list, then remove the
   temporary preprocessor file, deallocate the found_modules tree and
   stop parsing.
  */
  if( modlist_head == NULL ) {
    assert( unlink( ppfilename ) == 0 );
    tree_dealloc( found_modules );
    return( 1 );
  }
    
  /* If there was an error found during parsing, stop now */
  if( error_count > 0 ) {
    return( 1 );
  }

  curr = filelist_curr;
  tmpm = modlist_head;

  while( (tmpm != NULL) && (fname == NULL) ) {
    if( (node = tree_find( tmpm->str, found_modules )) != NULL ) {
      fname = node->value;
      file_prev_parsed = TRUE;
    } else {
      if( (curr = get_next_vfile( curr, tmpm->str )) == NULL ) {
        tmpm = tmpm->next;
        curr = filelist_curr;
      } else {
        fname            = curr->str;
        file_prev_parsed = curr->suppl2;
      }
    }
  }

  /* If the rest of the modules could not be found, display them to the user */
  if( (modlist_head != NULL) && (tmpm == NULL) ) {
    print_output( "Unable to find the following modules:", FATAL, __FILE__, __LINE__ );
    curr = modlist_head;
    while( curr != NULL ) {
      snprintf( user_msg, USER_MSG_LENGTH, "%s", obf_funit( curr->str ) );
      print_output( user_msg, FATAL_WRAP, __FILE__, __LINE__ );
      curr = curr->next;
    }
    assert( unlink( ppfilename ) == 0 );
    exit( EXIT_FAILURE );
  }
  
  /* Create temporary output filename */
  out = fopen( ppfilename, "w" );
  assert( out != NULL );
  
  /* Now run the preprocessor on this file first */
  reset_pplexer( fname, out );
  PPVLlex();
  
  fclose( out );

  yyin = fopen( ppfilename, "r" );
  yyrestart( yyin );
  
  yylloc.first_line  = 1;
  yylloc.last_column = 0;
  yylloc.text        = fname;
  
  filelist_curr = curr->next;

  /* If the current file was previously parsed, set the start state to IGNORE_GLOBALS */
  if( file_prev_parsed ) {
    BEGIN( IGNORE_GLOBALS );
  } else {
    BEGIN( PARSE );
  }
  
  return( 0 );

}

/*!
 \param file_list_head  Pointer to list of files to process.

 This function initializes the whole process. The first file is
 opened, and the lexer is initialized.  The include stack is cleared
 and ready to go.
*/
void reset_lexer( str_link* file_list_head ) { PROFILE(RESET_LEXER);

  FILE*     out;
  str_link* curr = file_list_head;

  if( (curr = get_next_vfile( curr, modlist_head->str )) == NULL ) {
    print_output( "No verilog files specified", FATAL, __FILE__, __LINE__ );
    exit( EXIT_FAILURE );
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
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( EXIT_FAILURE );
  }
  
  /* Now the preprocessor on this file first */
  reset_pplexer( curr->str, out );
  PPVLlex();
  
  fclose( out );

  yyin = fopen( ppfilename, "r" );
  yyrestart( yyin );

  yylloc.text        = curr->str;
  yylloc.first_line  = 1;
  yylloc.last_column = 0;
  
  filelist_head = file_list_head;
  filelist_curr = file_list_head->next;
  file_prev_parsed = FALSE;

  /* Set start state to allow parsing */
  BEGIN( PARSE );

}

