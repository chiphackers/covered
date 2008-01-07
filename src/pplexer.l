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
 \file     pplexer.l
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/2/2001
 \brief    Preprocessor lexer for Verilog language
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "defines.h"
#include "util.h"
#include "tree.h"
#include "obfuscate.h"

#define yylval PPVLlval
#define YYDEBUG 1

extern str_link* inc_paths_head;
extern char      user_msg[USER_MSG_LENGTH];


/*!
 Contains state before entering comment block.
*/
static int  comment_enter;

static void def_match();
static void def_start();
static int  def_is_done();
static void def_finish();
static void def_undefine();
static void do_define();
static int  is_defined( const char* name );

static void include_filename();
static void do_include();
static int  yywrap();

struct include_stack_t {
  char*                   path;
  FILE*                   file;  /*!< If the current input is the file, this member is set. */
  const char*             str;   /*!< If we are reparsing a macro expansion, file is 0 and 
			 	     this member points to the the string in progress */
  unsigned                lineno;
  YY_BUFFER_STATE         yybs;
  struct include_stack_t* next;
};

static struct include_stack_t* istack  = 0;
static struct include_stack_t* standby = 0;

%}

%option stack

%x ATTRIBUTE
%x LCOMMENT
%x CCOMMENT
%x PCOMMENT
%x CSTRING
%x DEFINE
%x INCLUDE
%x ERROR_LINE
%x PROTECTED

%x IFDEF_FALSE
%s IFDEF_TRUE
%x IFDEF_SUPR
%x ELSIF_FALSE

%x PPTIMESCALE

WSPACE [ \t\b\f]+

%%

  /* Whitespace -- we ignore this */
[ \t\b\f\r]   { ECHO; }

  /* Newline character - blank line -- we ignore these */
\n            { istack->lineno += 1;  ECHO; }

  /* Make sure that we don't treat SLIST code as an attribute */
"("{WSPACE}*"*"{WSPACE}*")"        { ECHO; }

  /* Attribute -- we only perform define substitution within strings */
"(*"                               { comment_enter = YY_START; BEGIN( ATTRIBUTE ); ECHO; }
<ATTRIBUTE>`[a-zA-Z_][a-zA-Z0-9_]* { def_match(); }
<ATTRIBUTE>.                       { ECHO; }
<ATTRIBUTE>\n                      { istack->lineno += 1; ECHO; }
<ATTRIBUTE>"*)"                    { BEGIN( comment_enter );  ECHO; }

  /* Single-line comment -- we ignore this information */
"//".*        { comment_enter = YY_START;  BEGIN( LCOMMENT );   ECHO; }
<LCOMMENT>.   {                                                 ECHO; }
<LCOMMENT>\n  { istack->lineno += 1;  BEGIN( comment_enter );   ECHO; }

  /* Multi-line comment */
"/*"           { comment_enter = YY_START;  BEGIN( CCOMMENT );  ECHO; }
<CCOMMENT>.    {                                                ECHO; }
<CCOMMENT>\n   { istack->lineno += 1;                           ECHO; }
<CCOMMENT>"*/" { BEGIN( comment_enter );                        ECHO; }

  /* C-style strings */
\"            { comment_enter = YY_START;  BEGIN( CSTRING );  ECHO; }
<CSTRING>\\\" {                                               ECHO; }
<CSTRING>\n   { istack->lineno += 1;                          ECHO; }
<CSTRING>\"   { BEGIN( comment_enter );                       ECHO; }
<CSTRING>.    {                                               ECHO; }

  /* Include directive */
^{WSPACE}?`include  { yy_push_state( INCLUDE ); }
<INCLUDE>\"[^\"]*\" { include_filename();       }
<INCLUDE>[ \t\b\f]  { ;                         }
<INCLUDE>"//".*     { ;                         }
<INCLUDE>\n         {
     istack->lineno += 1;
     yy_pop_state();
     do_include();
 }
<INCLUDE>\r\n       {
     istack->lineno += 1;
     yy_pop_state();
     do_include();
 }
<INCLUDE>\n\r       {
     istack->lineno += 1;
     yy_pop_state();
     do_include();
 }
<INCLUDE>.          {
     snprintf( user_msg, USER_MSG_LENGTH, "malformed `include directive, line %d, \"%s\"", istack->lineno, obf_file( istack->path ) );
     print_output( user_msg, FATAL, __FILE__, __LINE__ );
     BEGIN( ERROR_LINE );
 }

  /* Define directive */
`define{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*{WSPACE} {
     yy_push_state( DEFINE );
     def_start();
 }
`define{WSPACE}[a-zA-Z_][a-zA-Z0-9_]* {
     yy_push_state( DEFINE );
     def_start();
 }
<DEFINE>.* { 
     do_define();
 }
<DEFINE>(\n|"\r\n"|"\n\r") {
   if( def_is_done() ) {
     def_finish();
     istack->lineno += 1;
     yy_pop_state();
   }
   fputc( '\n', yyout );
 }

  /* Undefine directive */
`undef{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*{WSPACE}?.* { 
     def_undefine();
 }

  /* ifdef directive */
`ifdef{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*.* {
     char* name = strchr( yytext, '`' );
     assert( name );
     name += 6;
     name += strspn( name, " \t\b\f" );
     name[ strcspn( name, " \t\b\f" ) ] = '\0';
     if( is_defined( name ) ) {
       yy_push_state( IFDEF_TRUE );
     } else {
       yy_push_state( IFDEF_FALSE );
     }
 }

  /* ifndef directive */
`ifndef{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*.* {
     char* name = strchr( yytext, '`' );
     assert( name );
     name += 7;
     name += strspn( name, " \t\b\f" );
     name[ strcspn( name, " \t\b\f" ) ] = '\0';
     if( !is_defined( name ) ) {
       yy_push_state( IFDEF_TRUE );
     } else {
       yy_push_state( IFDEF_FALSE );
     }
 }

  /* elsif directive after unused block */
<IFDEF_FALSE>`elsif{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*.* {
     char* name = strchr( yytext, '`' );
     assert( name );
     name += 6;
     name += strspn( name, " \t\b\f" );
     name[ strcspn( name, " \t\b\f" ) ] = '\0';
     if( is_defined( name ) ) {
       BEGIN( IFDEF_TRUE );
     } else {
       BEGIN( IFDEF_FALSE );
     }
 }

  /* nested ifdef directive in unused block */
<IFDEF_FALSE,IFDEF_SUPR>^{WSPACE}?`ifdef{WSPACE}.* { yy_push_state( IFDEF_SUPR ); }

  /* nested ifndef directive in unused block */
<IFDEF_FALSE,IFDEF_SUPR>^{WSPACE}?`ifndef{WSPACE}.* { yy_push_state( IFDEF_SUPR ); }

  /* elsif directive after used block */
<IFDEF_TRUE,ELSIF_FALSE>^{WSPACE}?`elsif{WSPACE}.* { BEGIN( ELSIF_FALSE ); }

  /* else directive after used block */
<IFDEF_TRUE>^{WSPACE}?`else.* { BEGIN( IFDEF_FALSE ); }

  /* else directive after unused if block */
<IFDEF_FALSE>^{WSPACE}?`else.* { BEGIN( IFDEF_TRUE ); }

  /* else directive after unused elsif block */
<ELSIF_FALSE>^{WSPACE}?`else.* { BEGIN( IFDEF_FALSE ); }

  /* elsif directive in unused block */
<IFDEF_SUPR>^{WSPACE}?`elsif.* { }

  /* else directive in unused block */
<IFDEF_SUPR>^{WSPACE}?`else.* { }

  /* ignore anything in an unused block */
<IFDEF_FALSE,IFDEF_SUPR,ELSIF_FALSE>.     { }

  /* increment the line number when in an unused block and a newline is seen */
<IFDEF_FALSE,IFDEF_SUPR,ELSIF_FALSE>\n    {
     istack->lineno += 1;
     fputc( '\n', yyout );
 }

  /* endif directive */
<IFDEF_FALSE,IFDEF_TRUE,IFDEF_SUPR,ELSIF_FALSE>^{WSPACE}?`endif.* { yy_pop_state(); }

  /* Timescale directive -- we ignore this information */
`timescale { comment_enter = YY_START;  BEGIN( PPTIMESCALE );  ECHO; }
<PPTIMESCALE>.       { ECHO; }
<PPTIMESCALE>\n      {
     istack->lineno += 1;
     BEGIN( comment_enter );
     fputc( '\n', yyout );
 }

  /* For protected code, we need to completely ignore this information until a endprotected is seen */
`protected.*               { yy_push_state( PROTECTED );  ECHO; }
<PROTECTED>.               { ECHO;                              }
<PROTECTED>\n              { istack->lineno += 1;               }
<PROTECTED>`endprotected.* { yy_pop_state();                    }

  /* Miscellaneous directives -- we ignore this information */
`celldefine{WSPACE}?.*             { ECHO; }
`default_nettype{WSPACE}?.*        { ECHO; }
`delay_mode_distributed{WSPACE}?.* { ECHO; }
`delay_mode_unit{WSPACE}?.*        { ECHO; }
`delay_mode_path{WSPACE}?.*        { ECHO; }
`disable_portfaults{WSPACE}?.*     { ECHO; }
`enable_portfaults{WSPACE}?.*      { ECHO; }
`endcelldefine{WSPACE}?.*          { ECHO; }
`nosuppress_faults{WSPACE}?.*      { ECHO; }
`nounconnected_drive{WSPACE}?.*    { ECHO; }
`resetall{WSPACE}?.*               { ECHO; }
`suppress_faults{WSPACE}?.*        { ECHO; }
`unconnected_drive{WSPACE}?.*      { ECHO; }
`uselib{WSPACE}?.*                 { ECHO; }
`line{WSPACE}?.*                   { ECHO; }

  /* Macro identifier */
`[a-zA-Z_][a-zA-Z0-9_]* { def_match(); }

  /* Handle lines with errors */
<ERROR_LINE>.* { yy_pop_state(); }

  /* Final catchall */
. { ECHO; }

%%

/*!
 Define table.
*/
tnode* def_table = NULL;

/*!
 Specifies if the current definition is complete or that the next line is a
 continuation of the current definition.
*/
static int define_continue_flag = 0;

/*!
 String that contains the current definition value.
*/
static char* define_text = NULL;

/*!
 Specifies the size of the current define string.
*/
static size_t define_cnt = 0;


/*!
 \param name  Name of define to lookup in table
 
 \return Pointer to defined value.

 Looks up the specified define identifier in the binary tree lookup table.
 Returns NULL (0), if the define was not found in the lookup tree; otherwise,
 returns a pointer to the defined value.
*/
static tnode* def_lookup( const char* name ) { PROFILE(DEF_LOOKUP);

  return( tree_find( name, def_table ) );

}

/*!
 \param name  Name of defined value.
 
 \return Returns 1 if define is found in lookup table; otherwise, returns 0.

 Checks lookup table to see if specified define identifier has been
 previously defined.
*/
static int is_defined( const char* name ) { PROFILE(IS_DEFINED);

  return( def_lookup( name ) != NULL );

}

/*!
 When a macro use is discovered in the source, this function is
 used to look up the name and emit the substitution in its
 place. If the name is not found, then the `name string is written
 out instead.
*/

static void def_match() { PROFILE(DEF_MATCH);

  tnode* cur = def_lookup( yytext + 1 );  /* Pointer to define identifier node */
  int    i;                               /* Loop iterator */

  if( cur ) {

    for( i=strlen( cur->value ) - 1; i>=0; i-- ) {
      unput( cur->value[i] );
    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "macro %s undefined (and assumed null) at this point, line %d, \"%s\"",
              yytext, istack->lineno, obf_file( istack->path ) );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

}

/*!
 Holds current define identifier string.
*/
static char def_name[256];

/*!
 Extracts define identifier string from define directive match.
*/
static void def_start() { PROFILE(DEF_START);

  sscanf(yytext, "`define %s", def_name);

}

/*!
 \param name     String value of define identifier
 \param value    String value associated with define identifier

 Stores define identifer with its value in a define node and placed in
 the binary search tree for quick lookup later on.
*/
void define_macro( const char* name, const char* value ) { PROFILE(DEFINE_MACRO);

  tree_add( name, value, TRUE, &def_table );

}

/*!
 \bug This strips trailing line comments out of the definition.
      It's not adequate as the "//" may have been quoted or commented,
      but it will do for now.

 Prepares define value and identifier for storage in the lookup tree and
 then performs the store.
*/
static void do_define() { PROFILE(DO_DEFINE);

  char *cp;  /* Pointer to current token */

  define_continue_flag = 0;

  if( (cp = strstr(yytext, "//")) != NULL ) {
    *cp = 0;
  }

  /* Trim trailing white space. */
  cp = yytext + strlen( yytext );

  while( (cp > yytext) && isspace( *cp ) ) {
    cp -= 1;
    *cp = '\0';
  }

  /*
   Detect the continuation sequence. If I find it, remove it
   and the white space that preceeds it, then replace all that
   with a single newline.
  */
  if( (cp > yytext) && (cp[-1] == '\\') ) {
    cp -= 1;
    cp[0] = 0;
    while( (cp > yytext) && isspace(cp[-1]) ) {
      cp -= 1;
      *cp = 0;
    }
    *cp++ = '\n';
    *cp = 0;
    define_continue_flag = 1;
  }

  /* Accumulate this text into the define_text string. */
  define_text = (char*)realloc( define_text, (define_cnt + (cp-yytext) + 1) );
  strcpy( (define_text + define_cnt), yytext );
  define_cnt += cp - yytext;

}

/*!
 \return Returns true if the definition text is done; otherwise, returns false.
*/
static int def_is_done() { PROFILE(DEF_IS_DONE);

  return( define_continue_flag ? 0 : 1 );

}

/*!
 Completes the define addition phase.
*/
static void def_finish() { PROFILE(DEF_FINISH);

  define_continue_flag = 0;

  if( def_name[0] ) {

    if( define_text ) {
      define_macro( def_name, define_text );
      free_safe( define_text );
      define_text = NULL;
      define_cnt  = 0;
    } else {
      define_macro( def_name, "1" );
    }

    def_name[0] = 0;

  }

}

/*!
 Performs an undefine of an undef indentifier by searching for the
 matching define node and removing it from the tree.
*/
static void def_undefine() { PROFILE(DEF_UNDEFINE);

  /* Get undef directive identifier string */
  sscanf( yytext, "`undef %s", def_name );

  tree_remove( def_name, &def_table );

}

/*!
 Include file handling works by keeping an include stack of the
 files that are opened and being processed. The first item on the
 stack is the current file being scanned. If I get to an include
 statement,
 - open the new file,
 - save the current buffer context,
 - create a new buffer context,
 - and push the new file information.

 When the file runs out, the yywrap closes the file and deletes the
 buffer. If after popping the current file information there is
 another file on the stack, restore its buffer context and resume
 parsing.
*/

/*!
 Adds include directive filename to file parsing stack.
*/
static void include_filename() { PROFILE(INCLUDE_FILENAME);

  assert( standby == 0 );

  standby         = malloc_safe( sizeof(struct include_stack_t) );
  standby->path   = strdup_safe( yytext + 1 );
  standby->path[strlen(standby->path)-1] = 0;
  standby->lineno = 0;

}

/*!
 Performs the file context switch and gets everything ready for parsing
 the new file.
*/
static void do_include() { PROFILE(DO_INCLUDE);

  char      path[4096];  /* Full pathname to include file */
  str_link* curr;        /* Pointer to current include file element */

  if( standby->path[0] == '/') {

    standby->file = fopen(standby->path, "r");

    if( standby->file == 0 ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open `include file \"%s\"", obf_file( standby->path ) );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
    }

  } else {

    standby->file = 0;
    curr          = inc_paths_head;

    while( (curr != NULL) && (standby->file == 0) ) {

      snprintf( path, 4096, "%s/%s", curr->str, standby->path );
      standby->file = fopen(path, "r");
      curr = curr->next;

    }

    if( (curr == NULL) && (standby->file == 0) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to find `include file \"%s\" in include path", obf_file( standby->path ) );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( EXIT_FAILURE );
    }

  }

  assert( standby->file );

  standby->next     = istack;
  istack->yybs      = YY_CURRENT_BUFFER;
  istack->lineno    = istack->lineno;
  istack            = standby;
  standby           = 0;

  snprintf( user_msg, USER_MSG_LENGTH, "Parsing include file '%s'", obf_file( path ) );
  print_output( user_msg, NORMAL, __FILE__, __LINE__ );

  yy_switch_to_buffer( yy_create_buffer( istack->file, YY_BUF_SIZE ) );

  fprintf( yyout, "\n`line %u \"%s\" 1\n", (istack->lineno + 1), istack->path );
  
}

/*
 * The lexical analyzer calls this function when the current file
 * ends. Here I pop the include stack and resume the previous file. If
 * there is no previous file, then the main input is ended.
 */
static int yywrap() { PROFILE(YYWRAP);

  struct include_stack_t* isp = istack;

  istack = isp->next;

  /* If we are at the EOF and have not finished a define, do so now. */
  if( YY_START == DEFINE ) {
    def_finish();
    yy_pop_state();
  }

  /* If we are at the EOF and have not finished an include, do so now. */
  if( YY_START == INCLUDE ) {
    yy_pop_state();
    do_include();
  }

  /* Delete the current input buffers, and free the cell. */
  yy_delete_buffer( YY_CURRENT_BUFFER );

  if( isp->file ) {
    fclose(isp->file);
    free_safe(isp->path);
  }

  free_safe(isp);

  /* 
   If I am out of include stack, the main input is done.
  */
  if( istack != 0 ) {

    /* Otherwise, resume the input buffer that is the new stack top. */
    snprintf( user_msg, USER_MSG_LENGTH, "Resume parsing '%s'", obf_file( istack->path ) );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    yy_switch_to_buffer( istack->yybs );

    fprintf( yyout, "`line %u \"%s\" 2\n", istack->lineno + 1, istack->path );

    return 0;

  }

  return 1;

}

/*
 * This function initializes the whole process. The first file is
 * opened, and the lexor is initialized. The include stack is cleared
 * and ready to go.
 */
void reset_pplexer( char* filename, FILE* out ) { PROFILE(RESET_PPLEXER);

  struct include_stack_t* isp = malloc_safe( sizeof( struct include_stack_t ) );

  assert( filename != NULL );

  isp->path   = strdup_safe( filename );
  isp->file   = fopen( isp->path, "r" );
  isp->lineno = 0;
  isp->str    = 0;
  isp->next   = 0;

  if( isp->file == 0 ) {
    perror( isp->path );
    exit( EXIT_FAILURE );
  }

  yyout = out;
  yyin  = isp->file;

  yyrestart( yyin );

  istack = isp;

}

