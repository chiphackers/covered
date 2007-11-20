/*!
 \file     regress.cc
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     1/31/2006
 \brief    Regression utility for running Covered diagnostics and regression suite.
 \note     For usage information, type 'regress -h'.
*/

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <vector>

/* Defines */
#define VERSION    "0.1"

class regress {

  private:

    int            run_with_iv;
    int            run_with_vcs;
    int            run_with_vcd;
    int            run_with_lxt;
    vector<string> diag_list;

    void usage() {
      printf( "Usage:  regress [options]\n" );
      printf( "\n" );
      printf( "  Options:\n" );
      printf( "    -iv             Runs all Icarus Verilog diagnostics (does not need to be specified unless -vcs option is specified)\n" );
      printf( "    -vcs            Runs all VCS diagnostics\n" );
      printf( "    -vcd            Runs all diagnostics with VCD dumping (does not need to be specified unless -lxt option is specified)\n" );
      printf( "    -lxt            Runs all diagnostics with LXT dumping (only valid for Icarus Verilog runs)\n" );
      printf( "    -d <name>       Runs the diagnostic called <name> (default is to run all diagnostics)\n" );
      printf( "\n" );
      printf( "    -v              Displays the version of this regression utility\n" );
      printf( "    -h              Displays this help information\n" );
      exit( 1 );
    }

    void parse_args( int argc, char** argv ) {
      int   i = 0;    /* Loop iterator */
      char* argvptr;  /* Current argument */
      while( i < argc ) {
        argvptr = argv[i];
        if( strncmp( "-iv", argvptr, 3 ) == 0 ) {
          run_with_iv++;
        } else if( strncmp( "-vcs", argvptr, 4 ) == 0 ) {
          run_with_vcs++;
        } else if( strncmp( "-vcd", argvptr, 4 ) == 0 ) {
          run_with_vcd++;
        } else if( strncmp( "-lxt", argvptr, 4 ) == 0 ) {
          run_with_lxt++;
        } else if( strncmp( "-d", argvptr, 2 ) == 0 ) {
          i++;
          if( (i >= argc) || (argvptr[i][0] == '-') ) {
            printf( "ERROR:  The diagnostic was not specified for the -d option\n" );
            exit( 1 );
          }
          diag_list[0] = argvptr[i];
        } else if( strncmp( "-v", argvptr, 2 ) == 0 ) {
          printf( "%s\n", VERSION );
        } else {
          usage();
        }
        i++;
      }
    }

    void create_diag_list() {
      DIR*           dir_handle;  /* Pointer to opened directory */
      struct dirent* dirp;        /* Pointer to current directory entry */
      char*          ptr;         /* Pointer to current character in filename */
      int            tmpchars;    /* Number of characters needed to store full pathname for file */
      char*          tmpfile;     /* Temporary string holder for full pathname of file */
      if( (dir_handle = opendir( "." )) == NULL ) {
        printf( "Unable to read current directory", dir );
        exit( 1 );
      } else {
        while( (dirp = readdir( dir_handle )) != NULL ) {
          ptr = dirp->d_name + strlen( dirp->d_name ) - 1;
          /* Work backwards until a dot is encountered */
          while( (ptr >= dirp->d_name) && (*ptr != '.') ) {
            ptr--;
          }
          if( *ptr == '.' ) {
            ptr++;
            if( strcmp( "v", ptr ) == 0 ) {
              diag_list[diag_list.size()] = dirp->d_name;
            }
          }
        }
        closedir( dir_handle );
      }
    }

    void run_diag( int index ) {

      int run_cnt

      /* Parse the diagnostic run instructions */
      read_diag_options( index );

      while( diag_to_run ) {
        if( run_with_iv > 0 ) {

    }

  public:

    regress( int argc, char** argv ) {
 
      /* Initialize variables */
      run_with_iv  = 1;
      run_with_vcs = 0;
      run_with_vcd = 1;
      run_with_lxt = 0;

      /* Parse command-line arguments */
      parse_args( argc, argv );

      /* Gather the list of diagnostics to run */
      if( diag_list.size() == 0 ) {
        create_diag_list();
      }

      for( int i=0; i<diag_list.size(); i++ ) {
        run_diag( i );
      }

    }

    ~regress() {}

    
        
};

main( int argc, char** argv ) {

  regress tester( argc, argv );

}
