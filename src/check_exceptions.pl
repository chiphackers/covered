#! /usr/bin/perl

# Name:         check_exceptions
# Author:       Trevor Williams  <phase1geo(gmail.com)>
# Date:         3/4/2008
# Description:  Checks to make sure that all thrown exceptions are being properly handled.

# Read all .c files and extract needed information.
opendir( CDIR, "." ) || die "Can't open current directory for reading: $!\n";

while( $file = readdir( CDIR ) ) {
  chomp( $file );
  if( $file =~ /^(\w+.c)$/ ) {
    $file = $1;
    open( IFILE, $file ) || die "Can't open ${file} for reading: $!\n";
    # print "Parsing file ${file}...\n";
    $lnum        = 1;
    $scope_depth = 0;
    $in_ccomment = 0;
    while( $line = <IFILE> ) {
      chomp( $line );
      if( $line =~ /^\s*\/\// ) {
        next;
      } else {
        if( ($in_ccomment == 0) && ($line =~ /\/\*(\s+|$)/) ) {
          $in_ccomment = 1;
          # print "Starting normal comment block (file: ${file}, line: ${lnum})\n";
        }
        if( ($in_ccomment == 1) && ($line =~ /\*\//) ) {
          $in_ccomment = 0;
          # print "Ending normal comment block (file: ${file}, line: ${lnum})\n";
        }
      }
      if( $in_ccomment == 0 ) {
        if( ($scope_depth == 0) && ($line =~ /\/\*\!/) ) {
          $in_comment = 1;
          # print "Starting Doxygen comment block (file: ${file}, line: ${lnum})\n";
          %thrown_funcs = ();
        } elsif( ($in_comment == 1) && ($line =~ /\\throws\s+anonymous\s+(.*)\s*$/) ) {
          %thrown_funcs = split( join( " 1 ", split( $1 ) ) );
        } elsif( ($in_comment == 1) && ($line =~ /\*\//) ) {
          $in_comment = 0;
          # print "Ending Doxygen comment block (file: ${file}, line: ${lnum})\n";
        }
        if( $in_comment == 0 ) {
          if( ($scope_depth == 0) && ($line =~ /([a-z0-9_]+)\(/) ) {
            $func_name = $1;
            if( $func_name ne "PROFILE" ) {
              # print "  FOUND FUNCTION ${func_name} (line: ${lnum})\n";
              $funcs->{$func_name}{CTHROWS} = $thrown_funcs;
              print "  FOUND FUNCTION ${func_name} (line: ${lnum}) thrown_funcs: " . $funcs->{$func_name}{CTHROWS} . ".\n";
              $funcs->{$func_name}{FILE}    = $file;
              $funcs->{$func_name}{LINE}    = $lnum;
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
              if( ($called_fn ne "if")   &&
                  ($called_fn ne "while")  &&
                  ($called_fn ne "for")    &&
                  ($called_fn ne "switch") &&
                  ($called_fn ne "return") ) {
                $funcs->{$func_name}{CALLED}{$called_fn}{$lnum} = $thrown_funcs{$called_fn};
                # print "    FOUND CALLED FUNCTION ${called_fn} (line: ${lnum})\n";
              }
            } elsif( $line =~ /Throw/ ) {
              $funcs->{$func_name}{THROWS}{$lnum} = 1;
            } elsif( $line =~ /Try\s*\{/ ) {
              $funcs->{$func_name}{TRY}{$lnum} = 1;
            } elsif( $line =~ /Catch_anonymous/ ) {
              $funcs->{$func_name}{CATCH}{$lnum} = 1;
            }
          }
        }
      }
      $lnum++;
    }
    close( IFILE );
  }
}

closedir( CDIR );

# Now perform exception handling recursive searches to make sure that all functions are handled properly.
print "The following functions are missing a \\throws command in their function documentation:\n";
foreach $func (keys %$funcs) {
  &check_function( $func, "" );
}

sub is_throw_within_try_catch {

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

  my( $func, $called_func ) = @_;
  my( @throws, $throw );
  my( $fn, $lnum );
  my( $called_fn_lines, $called_fn_line );

  # print "Checking function ${func} in " . $funcs->{$func}{FILE} . " on line " . $funcs->{$func}{LINE} . " ...\n";

  # First, check to see if the function explicitly throws an exception
  $throws = $funcs->{$func}{THROWS};
  if( (keys %$throws) > 0 ) {

    # print "  Found to throw an exception...\n";

    # Check to see if this throws lands between a Try and Catch_anonymous call.
    foreach $throw (keys %$throws) {

      if( &is_throw_within_try_catch( $func, $throw ) == 0 ) {

        # print "    Throw is not caught in the same function (line: ${throw})\n";

        # If this function does not have the \throws command in its documentation, indicate that here.
        if( $funcs->{$func}{CTHROWS} == 0 ) {

          print "    ${func}   (Line throwing exception " . $funcs->{$func}{FILE} . ":${throw})\n";
          $funcs->{$func}{CTHROWS} = 1;

          # Then search for all of the functions that call this function and set their throws accordingly.
          foreach $fn (keys %$funcs) {
            $called_fn_lines = $funcs->{$fn}{CALLED}{$func};
            foreach $called_fn_line (keys %$called_fn_lines) {
              if( $lnum != 0 ) {
                $funcs->{$fn}{THROWS}{$lnum} = 2;
                &check_function( $fn );
              }
            }
          }

        }

      } else {

        # print "    Throw is caught in the same function (line: ${throw})\n";

      }

    }

  }

}
