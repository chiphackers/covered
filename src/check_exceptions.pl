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
        }
        if( ($in_ccomment == 1) && ($line =~ /\*\//) ) {
          $in_ccomment = 0;
        }
      }
      if( $in_ccomment == 0 ) {
        if( ($scope_depth == 0) && ($line =~ /\/\*\!/) ) {
          $in_comment = 1;
          %thrown_funcs = ();
        } elsif( ($in_comment == 1) && ($line =~ /\\throws\s+anonymous\s+(.*)\s*$/) ) {
          %thrown_funcs = split( join( " 1 ", split( $1 ) ) );
        } elsif( ($in_comment == 1) && ($line =~ /\*\//) ) {
          $in_comment = 0;
        }
        if( $in_comment == 0 ) {
          if( ($scope_depth == 0) && ($line =~ /([a-z0-9_]+)\(/) ) {
            $func_name = $1;
            if( $func_name ne "PROFILE" ) {
              $funcs->{$func_name}{FILE} = $file;
              $funcs->{$func_name}{LINE} = $lnum;
              print "Found function ${func_name} (${file}:${lnum})\n";
            }
            if($line =~ / \{/ ) {
              $scope_depth++;
            }
          } else {
            if( $line =~ / \{/ ) {
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
              }
            } elsif( $line =~ /Throw/ ) {
              $funcs->{$func_name}{CALLED}{"Throw"}{$lnum} = $thrown_funcs{"Throw"};
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

            print "Function ${called_fn} (in ${func}) is not within a try..catch block\n";

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
