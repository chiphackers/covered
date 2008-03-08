#! /usr/bin/perl

# Name:         check_exceptions
# Author:       Trevor Williams  <phase1geo(gmail.com)>
# Date:         3/4/2008
# Description:  Checks to make sure that all thrown exceptions are being properly handled.

# Read all .c files and extract needed information.
opendir( CDIR, "." ) || die "Can't open current directory for reading: $!\n";

while( $file = readdir( CDIR ) ) {
  chomp( $file );
  if( $file =~ /^(\w+).c$/ ) {
    $base  = $1;
    $ext   = "c";
    $wait  = 0;
    if( -f "${base}.l" ) {
      $ext  = "l";
      $wait = 2;
    } elsif( -f "${base}.y" ) {
      next;   # Skip parser files for now
    } elsif( $base eq "vpi" ) {
      next;   # Skip the vpi.c file for now
    }
    $file = "${base}.${ext}";
    system( "cpp -C ${file} ${base}.pp" ) && die "Unable to run preprocessor on ${file}: $!\n";
    open( IFILE, "${base}.pp" ) || die "Can't open ${base}.pp for reading: $!\n";
    $lnum        = 0;
    $scope_depth = 0;
    $parse = 0;
    while( $line = <IFILE> ) {
      $lnum++;
      chomp( $line );
      if( $wait == 0 ) {
        ($line, $lnum) = &preprocess_line( $line, $base, $ext, $lnum );
        if( $parse == 1 ) {
          if( ($scope_depth == 0) && ($line =~ /([a-z0-9_]+)\(/) ) {
            $func_name = $1;
            if( $func_name ne "PROFILE" ) {
              $funcs->{$func_name}{FILE} = $file;
              $funcs->{$func_name}{LINE} = $lnum;
            }
            if($line =~ /\{/ ) {
              $scope_depth++;
            }
          } else {
            if( $line =~ /\{/ ) {
              $scope_depth++;
            }
            if( $line =~ /\}/ ) {
              $scope_depth--;
            }
            if( $line =~ /([a-z0-9_]+)\(/ ) {
              $called_fn = $1;
              if( $called_fn eq "setjmp" ) {
                $funcs->{$func_name}{TRY}{$lnum} = 1;
              } elsif( $called_fn eq "longjmp" ) {
                $funcs->{$func_name}{CALLED}{"Throw"}{$lnum} = $thrown_funcs{"Throw"};
              } elsif( ($called_fn ne "if")   &&
                       ($called_fn ne "while")  &&
                       ($called_fn ne "for")    &&
                       ($called_fn ne "switch") &&
                       ($called_fn ne "return") ) {
                $funcs->{$func_name}{CALLED}{$called_fn}{$lnum} = $thrown_funcs{$called_fn};
              }
            } elsif( $line =~ /else the_exception_context->caught = 1;/ ) {
              $funcs->{$func_name}{CATCH}{$lnum} = 1;
            }
          }
        } else {
          if( ($wait > 0) && ($line =~ /^%%/) ) {
            $wait--;
          }
        }
      }
    }
    close( IFILE );
    system( "rm -f ${base}.pp" ) && die "Unable to remove ${base}.pp: $!\n";
  }
}

closedir( CDIR );

# Now perform exception handling recursive searches to make sure that all functions are handled properly.
foreach $func (keys %$funcs) {
  &check_function( $func );
}

# Now iterate through functions and display.
print "The following functions are missing a \\throws command in their function documentation:\n";
foreach $func (keys %$funcs) {
  $throws_cmd = "\\throws anonymous";
  $display    = 0;
  $called_fns = $funcs->{$func}{CALLED};
  foreach $called_fn (keys %$called_fns) {
    $called_fn_lines = $funcs->{$func}{CALLED}{$called_fn};
    foreach $called_fn_line (keys %$called_fn_lines) {
      if( $funcs->{$func}{CALLED}{$called_fn}{$called_fn_line} > 0 ) {
        $throws_cmd .= " ${called_fn}";
        if( $funcs->{$func}{CALLED}{$called_fn}{$called_fn_line} == 2 ) {
          $display = 1;
        }
      }
    }
  }
  if( $display == 1 ) {
    print "  ${func}  (" . $funcs->{$func}{FILE} . ":" . $funcs->{$func}{LINE} . "):  ${throws_cmd}\n";
  }
}

sub is_within_try_catch {

  my( $func, $throw ) = @_;

  # First, iterate through list of try's
  $tries = $funcs->{$func}{TRY};
  foreach $try (keys %$tries) {

    if( $try < $throw ) {

      $catches = $funcs->{$func}{CATCH};
      foreach $catch (keys %$catches) {

        if( ($catch > $try) && ($throw < $catch) ) {
          return 1;
        }

      }

    }

  }

  return 0;

}

sub check_function {

  my( $func ) = @_;
  my( @throws, $throw );
  my( $called_fn_lines, $called_fn_line );
  my( $fls, $fl );
  my( $fn );

  if( $funcs->{$func}{CHECKED} == 0 ) {

    $funcs->{$func}{CHECKED} = 1;

    $called_fns = $funcs->{$func}{CALLED};
    foreach $called_fn (keys %$called_fns) {

      $called_fn_lines = $funcs->{$func}{CALLED}{$called_fn};
      foreach $called_fn_line (keys %$called_fn_lines) {

        # If this function is known to throw an exception, continue.
        if( ($called_fn eq "Throw") || ($funcs->{$func}{CALLED}{$called_fn}{$called_fn_line} > 0) ) {

          # If we are not within a try..catch, this function will throw an exception
          if( &is_within_try_catch( $func, $called_fn_line ) == 0 ) {

            # Find all functions that call this function and iterate upwards
            foreach $fn (keys %$funcs) {
              $fls = $funcs->{$fn}{CALLED}{$func};
              foreach $fl (keys %$fls) {
                if( $funcs->{$fn}{CALLED}{$func}{$fl} == 0 ) {
                  $funcs->{$fn}{CALLED}{$func}{$fl} = 2;
                }
                &check_function( $fn );
              }
            }

          }

        }

      }

    }

  }

}

# Variables used exclusively by the preprocess_line command (and subcommands)
$in_comment  = 0;
$in_dcomment = 0;
$in_throws   = 0;

sub preprocess_line {

  my( $line, $base, $ext, $lnum ) = @_;
  my( $comment, $clean, $file );

  do {

    $clean = 1;

    # Handle preprocessor stuff
    if( $line =~ /^\s*#/ ) {

      # Handle file line numberings
      if( $line =~ /^\s*#\s+(\d+)\s+\"(\S+)\"/ ) { 
        $lnum = $1 - 1;
        $file = $2;
        if( $file eq "${base}.${ext}" ) {
          $parse = 1;
        } else {
          $parse = 0;
        }
      }
      $line = "";
    }

    # If we are currently not in a comment, check for comment structure.
    if( $in_comment == 0 ) {

      # Handle single line comments (ignore all stuff after the double-slash comment)
      if( $line =~ /(.*)\/\// ) {
        $line  = $1;
        $clean = 0;
      }

      # Handle multi-line comment that ends on the same line
      if( $line =~ /^(.*)\/\*(.*)\*\/(.*)$/ ) {
        $line     = $1 . $3;
        $comment  = $2;
        &check_comment_line( $comment );
        $clean    = 0;
        
      # Otherwise, check to see if this is the start of a multi-line comment
      } elsif( $line =~ /(.*)\/\*(.*)/ ) {
      	$line       = $1;
      	$comment    = $2;
      	&check_comment_line( $comment );
      	$in_comment = 1;
      	$clean      = 0;

      # Otherwise, check to see if this is a string
      } elsif( $line =~ /(.*)\".*\"(.*)/ ) {
        $line  = $1 . $2;
        $clean = 0;

      # Otherwise, check to see if this is a character
      } elsif( $line =~ /(.*)\'.*\'(.*)/ ) {
        $line  = $1 . $2;
        $clean = 0;
      }

    } else {

      # If we see the end-multi-line comment token, return the rest of the line after the token
      if( $line =~ /(.*)\*\/(.*)$/ ) {
        $comment     = $1;
        $line        = $2;
        $in_dcomment = 0;
        $in_throws   = 0;
        &check_comment_line( $comment );
        $in_comment  = 0;
        $clean       = 0;

      # Otherwise, return an empty string
      } else {
        $comment = $line;
        &check_comment_line( $comment );
        $line = "";
      }

    }

  } while( $clean == 0 );

  return ($line, $lnum);

}

sub check_comment_line {

  my( $comment ) = @_;
  my( $list );

  # If this is the first line of a multi-line comment, check for Doxygen comment syntax
  if( $in_comment == 0 ) {

    # If this comment is a Doxygen comment, allow parsing to continue.
    if( $comment =~ /^\!(.*)/ ) {
      $comment      = $1;
      $in_dcomment  = 1;
      %thrown_funcs = ();
    }

  }

  # If the current comment block is a Doxygen comment, parse it.
  if( $in_dcomment == 1 ) {

    # If the current line contains the \throws command, parse it
    if( $comment =~ /\\throws(.*)$/ ) {
      $comment   = $1;
      $in_throws = 1;

    # If this line is a blank line or a new command, stop adding to the thrown_funcs array
    } elsif( $comment =~ /(^\s*$)|(^\s*\\)/ ) {
      $in_throws = 0;
    }

    # If this is the \throws command, parse the thrown functions
    if( $in_throws == 1 ) {

      # If this is the first time we are parsing the \throws command, ignore the next token
      if( $comment =~ /^\s*(anonymous)?\s+(.*)\s*$/ ) {
        $list = $2;
        %thrown_funcs = (%thrown_funcs, split( /\s+/, join( " 1 ", split( /\s+/, $list ) ) ));
      }

    }

  }

}
