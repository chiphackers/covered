# Name:    regress_subs.pl
# Author:  Trevor Williams  <phase1geo(gmail.com)>
# Date:    6/16/2008
# Purpose: Contains shared Perl subroutines for performing regression
#          runs.

####################
# GLOBAL VARIABLES #
####################

# Global variable that can be used to specify Covered's executable pathname
$COVERED = "../../src/covered";

# Specifies the relative pathname to the directory containing CDD files to
# compare against.
$CDD_DIR = "../cdd";

# Specifies the relative pathname to the directory containing report files to
# compare against.
$RPT_DIR = "../rpt";

# Specifies the relative pathname to the directory containing error files to
# compare against
$ERR_DIR = "../err";

# Specifies the name of the file containing regression results in the Verilog
# directory.
$RPT_OUTPUT = "regress.output";

# Specifies the name of the file containing all failed regression runs in the
# Verilog directory.
$FAIL_OUTPUT = "regress.failed";


######################
# GLOBAL SUBROUTINES #
######################

# This subroutine should be used whenever a system command needs to be
# executed from a regression script.  Echoes the given command and then
# executes it.
sub runCommand {
  
  my( $cmd ) = $_[0];
  
  print "$cmd\n";
  system( "$cmd" ) && die;
  
}

# Verifies that specified diagnostic successfully passed all phases of testing.
# Note:    mode 0:  Normally run diagnostic error check (checks report and CDD files)
#          mode 1:  Error diagnostic (checks output against *.err files)
#          mode 2:  Skipped diagnostic (no check but increment PASSED count)
#          mode 3:  Finish run (removes all *.done files if FAILED count == 0)
#          mode 4:  Finish run but leave *.done files
#          mode 5:  Run report file diagnostic error check (for use with LXT/VPI dump verification)
sub checkTest {
 
  my( $test, $rm_cdd, $mode ) = @_;

  my( $passed ) = 0;
  my( $failed ) = 0;
  
  # Open report results file if it currently exists and accumulate info.
  if( open( RPT_RESULTS, "${RPT_OUTPUT}" ) > 0 ) {

    my( $results ) = <RPT_RESULTS>;
    chomp( $results );

    if( $results =~ /^DIAGNOSTICS PASSED: (\d+), FAILED: (\d+)$/ ) {
      $passed = $1;
      $failed = $2;
    } else {
      die "Bad format for regression results file\n";
    }

    close( RPT_RESULTS );

  }

  # If this is the finish mode, remove all *.done files if failed count shows no failures
  if( ($mode == 3) || ($mode == 4) ) {

    system( "echo; cat $RPT_OUTPUT; echo; rm -rf $RPT_OUTPUT" ) && die;

    if( ($failed == 0) && ($mode == 3) ) {
      system( "rm -rf *.done $FAIL_OUTPUT" ) && die;
    }

  } else {

    my( $check1, $check2, $check3, $check4, $check5 );
    
    # If this is not an error test, check the CDD and report files */
    if( ($mode == 0) || ($mode == 5) ) {

      if( $mode == 0 ) {

        # Check CDD file
        if( -e "${test}.cdd" ) {
          $check1 = system( "./cdd_diff ${test}.cdd ${CDD_DIR}/${test}.cdd" );
          if( $check1 == 0 ) {
            if( $rm_cdd > 0 ) {
              system( "rm ${test}.cdd" ) && die;
            }
            if( $rm_cdd > 1 ) {
              system( "rm ${test}a.cdd ${test}b.cdd" ) && die;
            }
            if( $rm_cdd > 2 ) {
              system( "rm ${test}c.cdd" ) && die;
            }
          }
        } else {
          $check1 = 1;
        }

      }

      # Check module report
      $check2 = system( "diff ${test}.rptM ${RPT_DIR}/${test}.rptM" );
      if( $check2 == 0 ) {
        system( "rm ${test}.rptM" ) && die;
      }

      # Check instance report
      $check3 = system( "diff ${test}.rptI ${RPT_DIR}/${test}.rptI" );
      if( $check3 == 0 ) {
        system( "rm ${test}.rptI" ) && die;
      }

      # Check module with line-width report
      $check4 = system( "diff ${test}.rptWM ${RPT_DIR}/${test}.rptWM" );
      if( $check4 == 0 ) {
        system( "rm ${test}.rptWM" ) && die;
      }

      # Check instance with line-width report
      $check5 = system( "diff ${test}.rptWI ${RPT_DIR}/${test}.rptWI" );
      if( $check5 == 0 ) {
        system( "rm ${test}.rptWI" ) && die;
      }

    # If this is an error test, check the error file
    } elsif( $mode == 1 ) {
  
      $check1 = system( "diff ${test}.err ${ERR_DIR}/${test}.err" );
      if( $check1 == 0 ) {
        system( "rm ${test}.err" ) && die;
      }

    }

    open( RPT_RESULTS, ">${RPT_OUTPUT}" ) || die "Can't open ${RPT_OUTPUT} for writing!\n";
    open( RPT_FAILED, ">>${FAIL_OUTPUT}" ) || die "Can't open ${FAIL_OUTPUT} for writing!\n";

    if( (($mode == 0) && (($check1 > 0) || ($check2 > 0) || ($check3 > 0) || ($check4 > 0) || ($check5 > 0))) ||
        (($mode == 1) && ($check1 > 0)) ||
        (($mode == 5) && (($check2 > 0) || ($check3 > 0) || ($check4 > 0) || ($check5 > 0))) ) {
      print "  Checking output results         -- FAILED\n";
      print RPT_FAILED "${test}\n";
      $failed++;
    } else {

      # Check to make sure that a tmp* file does not exist
      my( $tmp ) = `ls | grep ^tmp`;
      chomp( $tmp );
      if( $tmp ne "" ) {
        die "  Temporary file was not removed!\n";
      }

      if( ($mode == 0) || ($mode == 1) || ($mode == 5) ) {
        print "  Checking output results         -- PASSED\n";
      }
      $passed++;

      # Remove VCD file
      system( "rm -f *.vcd" ) && die;
 
      # Create the done file
      system( "touch ${test}.done" ) && die;

    }

    print RPT_RESULTS "DIAGNOSTICS PASSED: ${passed}, FAILED: ${failed}\n";

    close( RPT_RESULTS );
    close( RPT_FAILED  );

  }
 
}