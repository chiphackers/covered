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
#include "parser.h"
#include "util.h"
#include "link.h"
#include "vector.h"
#include "parser_misc.h"
#include "keywords.h"

#define yylval VLlval
#define YYDEBUG 1

extern YYLTYPE yylloc;

extern str_link* inc_paths_head;

extern int  ignore_mode;
extern char user_msg[USER_MSG_LENGTH];


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

static str_link* filelist_head = 0;
static str_link* filelist_curr = 0;
static int last_keyword        = 0;

extern str_link* modlist_head;
extern str_link* modlist_tail;

static struct include_stack_t* istack  = 0;
static struct include_stack_t* standby = 0;

%}

%option stack

%x LCOMMENT
%x CCOMMENT
%x CSTRING
%x DEFINE
%x INCLUDE
%x ERROR_LINE
%x IGNORE_MODULE

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

  /* Single-line comment -- we ignore this information */
"//".*        { comment_enter = YY_START;  BEGIN( LCOMMENT );    }
<LCOMMENT>.   { yymore();                                        }
<LCOMMENT>\n  { yylloc.first_line += 1;  BEGIN( comment_enter ); }

  /* Multi-line comment */
"/*"           { comment_enter = YY_START;  BEGIN( CCOMMENT ); }
<CCOMMENT>.    { yymore();                                     }
<CCOMMENT>\n   { yylloc.first_line += 1;  yymore();            }
<CCOMMENT>"*/" { BEGIN( comment_enter );                       }

  /* Multi-character symbols */
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
<IGNORE_MODULE>endmodule { BEGIN( 0 ); 
			   return I_endmodule; }
<IGNORE_MODULE>\n        { yylloc.first_line += 1;  
			   yymore();           }
<IGNORE_MODULE>.         { yymore();           }

  /* Keyword and signal identifiers */
[a-zA-Z_][a-zA-Z0-9$_]* {
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
       if( last_keyword == K_module ) {
         if( (modlist_head == NULL) || (strcmp( modlist_head->str, yytext ) != 0) ) {
           BEGIN( IGNORE_MODULE );
           rc = IGNORE;
         } else {
           if( filelist_curr != filelist_head ) {
             /* The module has been found, recirculate filelist */
             snprintf( user_msg, USER_MSG_LENGTH, "Parsing file %s", istack->path );
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

  /* Hierarchical signal identifiers */
[a-zA-Z_][a-zA-Z0-9$_]*(\.[a-zA-Z_][a-zA-Z0-9$_]*)+ {
     if( ignore_mode == 0 ) {
       yylval.text = strdup( yytext );
       return HIDENTIFIER;
     } else {
       return UNUSED_HIDENTIFIER;
     }
 }

  /* Signal names starting with a '\' character */
\\[^ \t\b\f\r]+  { 
     if( ignore_mode == 0 ) {
       yylval.text = strdup( yytext + 1 );
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

  /* Port name match -- we ignore this information */
\.{WSPACE}?(([a-zA-Z_][a-zA-Z0-9$_]*)|(\\[^ \t\b\f\r]+)) { 
     if( ignore_mode == 0 ) {
       return PORTNAME; 
     } else {
       return UNUSED_PORTNAME;
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

  /* Include directive */
^{WSPACE}?`include  { yy_push_state( INCLUDE ); }
<INCLUDE>\"[^\"]*\" { include_filename();       }
<INCLUDE>[ \t\b\f]  { ;                         }
<INCLUDE>"//".*     { ;                         }
<INCLUDE>\n         {
     yylloc.first_line += 1;
     yy_pop_state();
     do_include();
 }
<INCLUDE>\r\n       {
     yylloc.first_line += 1;
     yy_pop_state();
     do_include();
 }
<INCLUDE>\n\r       {
     yylloc.first_line += 1;
     yy_pop_state();
     do_include();
 }
<INCLUDE><<EOF>>    {
     yylloc.first_line += 1;
     yy_pop_state();
     do_include();
 }

<INCLUDE>.          {
     snprintf( user_msg, USER_MSG_LENGTH, "malformed `include directive, line %d, \"%s\"", yylloc.first_line, istack->path );
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
     yylloc.first_line += 1;
     yy_pop_state();
 }
<DEFINE><<EOF>> {
     def_finish();
     yylloc.first_line += 1;
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
     yylloc.first_line += 1;
 }

  /* endif directive */
<IFDEF_FALSE,IFDEF_TRUE,IFDEF_SUPR>^{WSPACE}?`endif.* { yy_pop_state(); }

  /* Macro identifier */
`[a-zA-Z_][a-zA-Z0-9_]* { def_match(); }

  /* Handle lines with errors */
<ERROR_LINE>.* { yy_pop_state(); }

  /* Final catchall. something got lost or mishandled. */

. {   fprintf( stderr, "%d: unmatched character (%c)\n", yylloc.first_line, yytext[0] ); }

%%

/*
 Defined macros are kept in this table for convenient lookup. As
 `define directives are matched (and the do_define() function
 called) the tree is built up to match names with values. If a
 define redefines an existing name, the new value it taken.
*/
struct define_t {
  char* name;
  char* value;

  struct define_t* left;
  struct define_t* right;
  struct define_t* up;
};

/*!
 Define table.
*/
static struct define_t* def_table = 0;

/*!
 \param name  Name of define to lookup in table
 \return Pointer to defined value.

 Looks up the specified define identifier in the binary tree lookup table.
 Returns NULL (0), if the define was not found in the lookup tree; otherwise,
 returns a pointer to the defined value.
*/
static struct define_t* def_lookup(const char* name) {

  struct define_t* cur = def_table;  /* Pointer to current define table node */
  int              cmp;              /* String compare value                 */

  if( cur != 0 ) {

    assert( cur->up == 0 );

    while( (cur != 0) && ((cmp = strcmp( name, cur->name )) != 0) ) {
      if( cmp < 0 ) {
        cur = cur->left;
      } else {
        cur = cur->right;
      }
    }

  }

  return( cur );

}

/*!
 \param name  Name of defined value.
 \return Returns 1 if define is found in lookup table; otherwise, returns 0.

 Checks lookup table to see if specified define identifier has been
 previously defined.
*/
static int is_defined( const char* name) {

  return( def_lookup( name ) != 0 );

}

/*!
 When a macro use is discovered in the source, this function is
 used to look up the name and emit the substitution in its
 place. If the name is not found, then the `name string is written
 out instead.
*/

static void def_match() {

  struct define_t* cur = def_lookup( yytext + 1 );  /* Pointer to define identifier node */
  int              i;                               /* Loop iterator                     */

  if( cur ) {

    for( i=strlen( cur->value ) - 1; i>=0; i-- ) {
      unput( cur->value[i] );
    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "macro %s undefined (and assumed null) at this point, line %d, \"%s\"", yytext, yylloc.first_line, istack->path );
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

  struct define_t* def;         /* Pointer to newly created define node               */
  struct define_t* cur;         /* Pointer to current define node in tree             */
  int              placed = 0;  /* Specifies that define node has been placed in tree */
  int              cmp;         /* String compare value                               */
  def = malloc( sizeof( struct define_t ) );

  /* Initialize define node */
  def->name    = strdup( name );
  def->value   = strdup( value );
  def->left    = 0;
  def->right   = 0;
  def->up      = 0;

  /* If this node is first inserted, it is root of tree */
  if( def_table == 0 ) {

    def_table = def;

  } else {

    cur = def_table;

    while( !placed ) {

      cmp = strcmp( def->name, cur->name );

      /* Re-define found */
      if( cmp == 0 ) {

        free_safe( cur->value );
        cur->value = def->value;
        free_safe(def->name);
        free_safe(def);
        placed = 1;

      } else if( cmp < 0 ) {

        if( cur->left == 0 ) {
          cur->left = def;
          def->up   = cur;
          placed    = 1;
        } else {
          cur       = cur->left;
        }

      } else {

        if( cur->right == 0 ) {
          cur->right = def;
          def->up    = cur;
          placed     = 1;
        } else {
          cur        = cur->right;
        }

      }

    }

  }

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

  /* Find undefined identifer string in table */
  cur = def_lookup( def_name );

  /* If node is found, restitch the define tree. */
  if( cur != 0 ) {

    /* If we are the root node in the tree */
    if( cur->up == 0 ) {

      /* If we have no children */
      if( (cur->left == 0) && (cur->right == 0) ) {

        def_table = 0;

      } else if( cur->left == 0 ) {

        def_table = cur->right;
        if( cur->right ) {
          cur->right->up = 0;
        }

      } else if( cur->right == 0 ) {

        assert( cur->left );
        def_table     = cur->left;
        def_table->up = 0;

      } else {

        tail = cur->left;
        while( tail->right ) {
          tail = tail->right;
        }

        tail->right     = cur->right;
        tail->right->up = tail;
        def_table       = cur->left;
        def_table->up   = 0;

      }

    } else if( cur->left == 0 ) {

      if( cur->up->left == cur ) {

        cur->up->left = cur->right;

      } else {

        assert( cur->up->right == cur );
        cur->up->right = cur->right;

      }

      if( cur->right ) {
        cur->right->up = cur->up;
      }

    } else if( cur->right == 0 ) {

      assert(cur->left);

      if( cur->up->left == cur ) {

        cur->up->left = cur->left;

      } else {

        assert( cur->up->right == cur );
        cur->up->right = cur->left;

      }

      cur->left->up = cur->up;

    } else {

      tail = cur->left;
      assert( cur->left && cur->right );

      while( tail->right ) {
        tail = tail->right;
      }

      tail->right = cur->right;
      tail->right->up = tail;

      if( cur->up->left == cur ) {

        cur->up->left = cur->left;

      } else {

        assert( cur->up->right == cur );
        cur->up->right = cur->left;

      }

      cur->left->up = cur->up;

    }

    free_safe(cur->name);
    free_safe(cur->value);
    free_safe(cur);

  }

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
  istack->lineno    = yylloc.first_line;
  istack            = standby;
  standby           = 0;
  yylloc.first_line = 1;

  snprintf( user_msg, USER_MSG_LENGTH, "Parsing include file \"%s\"", path );
  print_output( user_msg, NORMAL );

  yy_switch_to_buffer( yy_new_buffer( istack->file, YY_BUF_SIZE ) );

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

  /* Delete the current input buffers, and free the cell. */
  yy_delete_buffer( YY_CURRENT_BUFFER );

  if( isp->file ) {
    fclose(isp->file);
    free_safe(isp->path);
  }

  free_safe(isp);

  /* 
   If I am out of include stack, the main input is done.  If we are still searching,
   process in the modlist structure.  If no more modules are needed, stop.  Otherwise,
   reset the filelist_curr pointer to the head of the filelist, search for the file containing
   this needed module and parse that module.
  */
  if( istack == 0 ) {

    if( modlist_head == NULL ) {
      return( 1 );
    }

    /* Problem here is that istack seems to be gone from memory */
    istack = (struct include_stack_t*)malloc( sizeof( struct include_stack_t ) );
    curr   = filelist_curr;

    if( (curr = get_next_vfile( curr, modlist_head->str )) == NULL ) {
      print_output( "Unable to find the following modules:", FATAL );
      curr = modlist_head;
      while( curr != NULL ) {
        fprintf( stderr, "        %s\n", curr->str );
        curr = curr->next;
      }
      exit( 1 );
    }

    istack->path   = strdup( curr->str );
    istack->next   = 0;
    istack->lineno = 0;
    istack->file   = fopen( istack->path, "r" );
    
    if( istack->file == 0 ) {
      perror( istack->path );
      exit( 1 );
    }

    yyrestart( istack->file );

    yylloc.first_line = 1;
    yylloc.text       = istack->path;

    filelist_curr = curr->next;

    return( 0 );

  }

  /* Otherwise, resume the input buffer that is the new stack top. */
  snprintf( user_msg, USER_MSG_LENGTH, "Resume parsing %s", istack->path );
  print_output( user_msg, NORMAL );
  yy_switch_to_buffer( istack->yybs );
  yylloc.first_line = istack->lineno;

  return 0;

}

/*
 * This function initializes the whole process. The first file is
 * opened, and the lexor is initialized. The include stack is cleared
 * and ready to go.
 */
void reset_lexer( FILE* out, str_link* file_list_head ) {

  struct include_stack_t* isp  = malloc( sizeof( struct include_stack_t ) );
  str_link*               curr = file_list_head;            

  if( (curr = get_next_vfile( curr, modlist_head->str )) == NULL ) {
    print_output( "No verilog files specified", FATAL );
    exit( 1 );
  }

  isp->path   = strdup( curr->str );
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

  yylloc.text       = isp->path;
  yylloc.first_line = 1;

  istack        = isp;
  filelist_head = file_list_head;
  filelist_curr = file_list_head->next;

}

