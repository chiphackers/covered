%{
/*!
 \file     pplexer.l
 \author   Trevor Williams  (trevorw@charter.net)
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

char ppfilename[10];

%}

%option stack

%x LCOMMENT
%x CCOMMENT
%x PCOMMENT
%x CSTRING
%x DEFINE
%x INCLUDE
%x ERROR_LINE

%x IFDEF_FALSE
%s IFDEF_TRUE
%x IFDEF_SUPR

%x PPTIMESCALE

WSPACE [ \t\b\f]+

%%

  /* Whitespace -- we ignore this */
[ \t\b\f\r]   { ECHO; }

  /* Newline character - blank line -- we ignore these */
\n            { istack->lineno += 1;  ECHO; }

  /* Single-line comment -- we ignore this information */
"//".*        { comment_enter = YY_START;  BEGIN( LCOMMENT );   ECHO; }
<LCOMMENT>.   {                                                 ECHO; }
<LCOMMENT>\n  { istack->lineno += 1;  BEGIN( comment_enter );   ECHO; }

  /* Multi-line comment */
"/*"           { comment_enter = YY_START;  BEGIN( CCOMMENT );  ECHO; }
<CCOMMENT>.    {                                                ECHO; }
<CCOMMENT>\n   { istack->lineno += 1;                           ECHO; }
<CCOMMENT>"*/" { BEGIN( comment_enter );                        ECHO; }

  /* Pragma comments */
"(*"           { comment_enter = YY_START;  BEGIN( PCOMMENT );  ECHO; }
<PCOMMENT>.    {                                                ECHO; }
<PCOMMENT>\n   { istack->lineno += 1;                           ECHO; }
<PCOMMENT>"*)" { BEGIN( comment_enter );                        ECHO; }

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
     snprintf( user_msg, USER_MSG_LENGTH, "malformed `include directive, line %d, \"%s\"", istack->lineno, istack->path );
     print_output( user_msg, FATAL );
     BEGIN( ERROR_LINE );
 }

  /* Define directive */
^{WSPACE}?`define{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*{WSPACE} {
     yy_push_state( DEFINE );
     def_start();
 }
^{WSPACE}?`define{WSPACE}[a-zA-Z_][a-zA-Z0-9_]* {
     yy_push_state( DEFINE );
     def_start();
 }
<DEFINE>.* { 
     do_define();
 }
<DEFINE>(\n|"\r\n"|"\n\r") {
     def_finish();
     istack->lineno += 1;
     fputc( '\n', yyout );
     yy_pop_state();
 }

  /* Undefine directive */
^{WSPACE}?`undef{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*{WSPACE}?.* { 
     def_undefine();
 }

  /* ifdef directive */
^{WSPACE}?`ifdef{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*.* {
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
^{WSPACE}?`ifndef{WSPACE}[a-zA-Z_][a-zA-Z0-9_]*.* {
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

  /* nested ifdef directive in unused block */
<IFDEF_FALSE,IFDEF_SUPR>^{WSPACE}?`ifdef{WSPACE}.* { yy_push_state( IFDEF_SUPR ); }

  /* nested ifndef directive in unused block */
<IFDEF_FALSE,IFDEF_SUPR>^{WSPACE}?`ifndef{WSPACE}.* { yy_push_state( IFDEF_SUPR ); }

  /* else directive after used block */
<IFDEF_TRUE>^{WSPACE}?`else.* { BEGIN( IFDEF_FALSE ); }

  /* else directive after unused block */
<IFDEF_FALSE>^{WSPACE}?`else.* { BEGIN( IFDEF_TRUE ); }

  /* else directive in unused block */
<IFDEF_SUPR>^{WSPACE}?`else.* { }
<IFDEF_FALSE,IFDEF_SUPR>.     { }
<IFDEF_FALSE,IFDEF_SUPR>\n    {
     istack->lineno += 1;
     fputc( '\n', yyout );
 }

  /* endif directive */
<IFDEF_FALSE,IFDEF_TRUE,IFDEF_SUPR>^{WSPACE}?`endif.* { yy_pop_state(); }

  /* Timescale directive -- we ignore this information */
^{WSPACE}?`timescale { comment_enter = YY_START;  BEGIN( PPTIMESCALE );  ECHO; }
<PPTIMESCALE>.       {                                                   ECHO; }
<PPTIMESCALE>\n      {
     istack->lineno += 1;
     BEGIN( comment_enter );
     fputc( '\n', yyout );
 }

  /* Miscellaneous directives -- we ignore this information */
^{WSPACE}?`celldefine{WSPACE}?.*             { ECHO; }
^{WSPACE}?`default_nettype{WSPACE}?.*        { ECHO; }
^{WSPACE}?`delay_mode_distributed{WSPACE}?.* { ECHO; }
^{WSPACE}?`delay_mode_unit{WSPACE}?.*        { ECHO; }
^{WSPACE}?`delay_mode_path{WSPACE}?.*        { ECHO; }
^{WSPACE}?`disable_portfaults{WSPACE}?.*     { ECHO; }
^{WSPACE}?`enable_portfaults{WSPACE}?.*      { ECHO; }
^{WSPACE}?`endcelldefine{WSPACE}?.*          { ECHO; }
^{WSPACE}?`endprotect{WSPACE}?.*             { ECHO; }
^{WSPACE}?`nosuppress_faults{WSPACE}?.*      { ECHO; }
^{WSPACE}?`nounconnected_drive{WSPACE}?.*    { ECHO; }
^{WSPACE}?`protect{WSPACE}?.*                { ECHO; }
^{WSPACE}?`resetall{WSPACE}?.*               { ECHO; }
^{WSPACE}?`suppress_faults{WSPACE}?.*        { ECHO; }
^{WSPACE}?`unconnected_drive{WSPACE}?.*      { ECHO; }
^{WSPACE}?`uselib{WSPACE}?.*                 { ECHO; }

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
 \param name  Name of define to lookup in table
 
 \return Pointer to defined value.

 Looks up the specified define identifier in the binary tree lookup table.
 Returns NULL (0), if the define was not found in the lookup tree; otherwise,
 returns a pointer to the defined value.
*/
static tnode* def_lookup( const char* name ) {

  return( tree_find( name, def_table ) );

}

/*!
 \param name  Name of defined value.
 
 \return Returns 1 if define is found in lookup table; otherwise, returns 0.

 Checks lookup table to see if specified define identifier has been
 previously defined.
*/
static int is_defined( const char* name ) {

  return( def_lookup( name ) != NULL );

}

/*!
 When a macro use is discovered in the source, this function is
 used to look up the name and emit the substitution in its
 place. If the name is not found, then the `name string is written
 out instead.
*/

static void def_match() {

  tnode* cur = def_lookup( yytext + 1 );  /* Pointer to define identifier node */
  int    i;                               /* Loop iterator                     */

  if( cur ) {

    for( i=strlen( cur->value ) - 1; i>=0; i-- ) {
      unput( cur->value[i] );
    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "macro %s undefined (and assumed null) at this point, line %d, \"%s\"", yytext, istack->lineno, istack->path );
    print_output( user_msg, WARNING );

  }

}

/*!
 Holds current define identifier string.
*/
static char def_name[256];

/*!
 Extracts define identifier string from define directive match.
*/
static void def_start() {

  sscanf(yytext, "`define %s", def_name);

}

/*!
 \param name     String value of define identifier
 \param value    String value associated with define identifier

 Stores define identifer with its value in a define node and placed in
 the binary search tree for quick lookup later on.
*/
void define_macro( const char* name, const char* value ) {

  tree_add( name, value, TRUE, &def_table );

}

/*!
 \bug This strips trailing line comments out of the definition.
      It's not adequate as the "//" may have been quoted or commented,
      but it will do for now.

 Prepares define value and identifier for storage in the lookup tree and
 then performs the store.
*/
static void do_define() {

  char *cp;        /* Pointer to current token */

  if( (cp = strstr(yytext, "//")) != NULL ) {
    *cp = 0;
  }

  /* Trim trailing white space. */
  cp = yytext + strlen( yytext );

  while( (cp > yytext) && isspace( *cp ) ) {
    cp -= 1;
    *cp = '\0';
  }

  define_macro( def_name, yytext );

  def_name[0] = '\0';

}

/*!
 Completes the define addition phase.
*/
static void def_finish() {

  if( def_name[0] ) {
    define_macro( def_name, "1" );
  }

}

/*!
 Performs an undefine of an undef indentifier by searching for the
 matching define node and removing it from the tree.
*/
static void def_undefine() {

  struct define_t* cur;   /* Pointer to current define node */
  struct define_t* tail;  /* Pointer to last define node    */

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
static void include_filename() {

  assert( standby == 0 );

  standby         = malloc(sizeof(struct include_stack_t));
  standby->path   = strdup(yytext+1);
  standby->path[strlen(standby->path)-1] = 0;
  standby->lineno = 0;

}

/*!
 Performs the file context switch and gets everything ready for parsing
 the new file.
*/
static void do_include() {

  char      path[4096];  /* Full pathname to include file           */
  str_link* curr;        /* Pointer to current include file element */

  if( standby->path[0] == '/') {

    standby->file = fopen(standby->path, "r");

    if( standby->file == 0 ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open `include file \"%s\"", standby->path );
      print_output( user_msg, FATAL );
    }

  } else {

    standby->file = 0;
    curr          = inc_paths_head;

    while( (curr != NULL) && (standby->file == 0) ) {

      snprintf(path, 4096, "%s/%s", curr->str, standby->path);
      standby->file = fopen(path, "r");
      curr = curr->next;

    }

    if( (curr == NULL) && (standby->file == 0) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to find `include file \"%s\" in include path", standby->path );
      print_output( user_msg, FATAL );
      exit( 1 );
    }

  }

  assert( standby->file );

  standby->next     = istack;
  istack->yybs      = YY_CURRENT_BUFFER;
  istack->lineno    = istack->lineno;
  istack            = standby;
  standby           = 0;

  snprintf( user_msg, USER_MSG_LENGTH, "Parsing include file \"%s\"", path );
  print_output( user_msg, NORMAL );

  yy_switch_to_buffer( yy_new_buffer( istack->file, YY_BUF_SIZE ) );

  fprintf( yyout, "\n`line %u \"%s\" 1\n", (istack->lineno + 1), istack->path );
  
}

/*
 * The lexical analyzer calls this function when the current file
 * ends. Here I pop the include stack and resume the previous file. If
 * there is no previous file, then the main input is ended.
 */
static int yywrap() {

  struct include_stack_t* isp = istack;
  str_link*               curr;

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
    snprintf( user_msg, USER_MSG_LENGTH, "Resume parsing %s", istack->path );
    print_output( user_msg, NORMAL );
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
void reset_pplexer( char* filename, FILE* out ) {

  struct include_stack_t* isp = malloc( sizeof( struct include_stack_t ) );

  assert( filename != NULL );

  isp->path   = strdup( filename );
  isp->file   = fopen( isp->path, "r" );
  isp->lineno = 0;
  isp->str    = 0;
  isp->next   = 0;

  if( isp->file == 0 ) {
    perror( isp->path );
    exit( 1 );
  }

  yyout = out;

  yyrestart( isp->file );

  istack = isp;

}

