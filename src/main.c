/*!
 \file     main.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/26/2001
*/
/*!
 \mainpage Covered:  Verilog Code-Coverage Analyzer
 - \ref page_intro
 - \ref page_howitworks
 - \ref page_news
 - \ref page_todo
 - \ref page_download
 - \ref page_bugs
*/
/*!
 \page page_intro Introduction
 \par What is a code coverage tool used for?
 Covered is a Verilog code coverage analysis tool that can be useful for determining how
 well a diagnostic test suite is covering the design under test.  Typically in the design
 verification work flow, a design verification engineer will develop a self-checking test
 suite to verify design elements/functions specified by a design's specification document.
 When the test suite contains all of the tests required by the design specification, the
 test writer may be asking him/herself, "How much logic in the design is actually being
 exercised?", "Does my test suite cover all of the logic under test?", and "Am I done writing
 tests for the logic?".  When the design verification gets to this point, it is often
 useful to get some metrics for determining logic coverage.  This is where a code coverage
 utility, such as Covered, is very useful.

 \par
 The metrics obtained by using a code coverage analysis tool can be very useful for determining
 the following about a design and the test suite testing that design:
 -# Completeness of the test suite in terms of logic coverage.
 -# Unexercised logic in the design (useful in helping to determine what types of tests need
    to be added to the test suite).
 -# Corner cases in design that are untestable.

 \par
 It is very important to note that any code coverage tool is only useful in indicating how
 much logic is being covered by a test suite.  It does not indicate that the covered logic
 works appropriately.  This, of course, can only be verified by the diagnostics themselves.
 Additionally, it is possible that two or more diagnostics can achieve the same coverage
 and yet be functionally testing different features in the design.  Since the coverage
 metrics are not improved in this case, one may conclude that the second test is unneccessary.
 This may or may not be true depending on what is being tested, it is always up to the
 test writer to determine the necessity of the diagnostic.  Using the code coverage tool
 results as the sole means of making this determination is not recommended.  Use common
 sense in these areas.

 \par What does Covered do?
 Covered is a tool that uses your design files along with specialized VCD dump files to
 analyze the code coverage of the design.  The code coverage information is stored in a special
 database file that can be retrieved and "merged" with new coverage information to create a
 summed coverage total for several tests.  After a database file has been create, the user
 may generate reports that summarize the coverage information.

 \par Covered Metrics
 Covered currently generates four types of code coverage metrics.  Each of these metrics is
 described in detail in the following subsections:

 \par Line Coverage Description
 Line coverage simply answers the question, "Was this line of code executed during simulation?"
 Covered will display the number of logical lines of code that exist in a particular file with
 the number of logical lines that were executed during the simulation along with a percentage
 indicating the percentage of lines executed.  If verbose mode is selected for a report, Covered
 will display the lines of logic that were not executed during the simulation run.

 \par
 For a design to pass full coverage, it is recommended that the line coverage for all modules in
 a design receive 100% coverage.  If a line of logic is not executed during simulation, the
 design has not been fully exercised.  Line coverage is useful for determining holes in the test
 suite.

 \par Toggle Coverage Description
 Toggle coverage answers the question, "Did this bit of this wire/register change from a value
 of zero (0) to one (1) and back from one (1) to zero (0) during simulation?".  A bit is said
 to be fully covered when it toggles back and forth at least once.  This metric does not
 indicate to the user that every value of a multi-bit vector was seen.  For example, if we have
 a two bit vector called "foo", toggle coverage will not tell you that the value of foo was set
 to the values of 0, 1, 2 and 3.  However, it will tell you that all bits in that vector were
 toggled back and forth.

 \par
 For a design to pass full coverage, it is recommended that the toggle coverage for all modules
 in a design received 100% coverage.  If a bit is never changes value, it is usually an indication
 that a mode is not being exercised in the design or a datapath has a stuck at issue.

 \par Combinational Logic Coverage Description
 Combinational logic coverage answers the question, "What values did an expression (or 
 subexpression) evaluate to (or not evaluate to) during the course of the simulation?"  This 
 type of coverage is extremely useful in determining logical combinations of signals that were 
 not tried during simulation, exposing potential holes in verification.

 \par
 For a design to pass full coverage, it is recommended that the combinational logic coverage for 
 all modules be 80% or higher.  If the expression coverage for an expression is not 100%, it is
 recommended that the verification engineer closely examine this missed cases to determine if
 more testing is required.  Sometimes certain combinations of signals are unachievable due to
 design constraints, keeping the expression coverage from ever reaching a value of 100% but still
 can be considered full covered.

 \par Finite State Machine Coverage Description
 Finite state machine coverage is particular coverage numbers for state machines within the design.
 There are two types of coverage detail for FSMs that Covered can handle:
 -# State coverage:  answers the question "Were all states of an FSM hit during simulation?"
 -# State transition coverage:  answers the question "Did the FSM transition between all states
 (that are achievable) in simulation?"

 \par
 For a design to pass full coverage, it is recommended that the FSM coverage for all finite state
 machines in the design to receive 100% coverage for the state coverage and 100% for all achievable
 state transitions.  Since Covered will not determine which state transitions are achievable, it is
 up to the verification engineer to examine the executed state transitions to determine if 100%
 of possible transitions occurred.  

*/
/*!
 \page page_howitworks How does Covered work?
 To begin extracting coverage numbers for a design, the design must first be simulated with VCD
 style dumping turned on for the modules undergoing coverage analysis.  Once a VCD file has
 been created, the design must be parsed by the Covered utility, and a database file must be
 created, storing the parsed information.  This is achieved through the use of Covered's score
 command.  Once an initial database file has been generated, Covered will read in the specified 
 VCD file along with the initial database file and update the database file to contain the coverage
 results extracted from the dumpfile.  This step also occurs during Covered's score command phase.
 Once the database file contains scoring information, several things can occur with that database
 file.

 \par
 First, a user may create coverage reports from the scored database file.  Several types of reports
 can be generated from a given scored database file.  The following types of reports can be generated
 (this is not intended to be a complete list):
 -# Summarized reports:  only coverage percentages are reported for modules/module instances.  These
 types of reports are typically generated first to get a high-level view of the coverage amount.
 -# Verbose reports:  detailed information on certain coverage results are displayed, including
 Verilog code snippets and detailed analysis reports on logical values.
 -# Module instance reports:  look at summary/verbose reports for each individual module instance
 in the analyzed design.
 -# Module reports:  look at summary/verbose reports for a module (combines all analyzed module instances
 of the specified module).

 \par
 Second, a user may merge coverage databases for the same design (but from different simulation runs).
 This is necessary since one test will usually not test an entire design.  Typically, a test suite 
 (containing more than one diagnostic) is used to test a design.  By merging the coverage results of all
 test runs, we can generate coverage reports that reflect the amount of the design covered by the
 entire test suite (rather than just a single test).  Any database file that was generated from the
 exact same design may be merged; however, merging different designs is prohibited by Covered and will
 produce an error to the user (without affecting any of the specified design files).
 
*/
/*!
 \page page_news Look here for the latest Covered news!

 - 4/12/2002
 \par
 Created a home for the Covered utility on SourceForge.net.  Currently, the generated HTML
 files are only available for viewing.  No downloads are yet available.  There are a few
 features that need to be added to the tool (and verification of those and other parts)
 before the source files and tool become available for public use.  Check out the ToDo page
 for items that are on my todo list before making the tool available online!

 - 11/27/2001
 \par
 Started working on the Covered utility.  After searching long and hard for a free,
 open source code coverage utility and coming up empty, decided to start working on
 one.  This utility will be available on the gEDA website and database.
*/
/*!
 \page page_todo What is on the Covered todo list

 \par
 Items that need to be completed before initial release of this utility/source code.

 -# Work on verbose reporting for all coverage metrics.
 -# Add support for statements in Verilog parser.  Currently, only expressions are handled
 correctly by the parser.  Expressions are not contingent upon anything else besides
 signal changes within the expression to be added the the run-time engine.  Statements
 are required to handle ordering of expressions.
 -# Add combinational logic coverage handling to report structure.  Currently, line and
 toggle coverage are handled in summary form.

 \par
 The following are items that are on the todo list after the above have been completed.
 Some of them are long term features that I want as part of this tool.

 -# Enhance Verilog code parser to allow coverage on more of the language.
 -# Add support to handle Finite State Machines.  The initial available versions of
 Covered will not include FSM coverage metrics.
 -# Provide GUI for generating/parsing reports.  The GUI will allow users to quickly
 see what is covered and what is not covered (without being as messy as a large
 text file).

 \par
 If you have any other things that you would like to see put into these lists, let me
 know at trevorw@charter.net.
*/
/*!
 \page page_download Downloading and Installing Covered

 No version is currently available for download.
*/
/*!
 \page page_bugs Reporting Covered Bugs

 If you encounter any problems with the Covered tool, please send these problems along with
 the version of Covered that you are using to trevorw@charter.net.  Please describe the
 problem as accurately as possible or send an example that uncovers the problem.  I will
 try to look at these as soon as possible.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "defines.h"
#include "score.h"
#include "merge.h"
#include "report.h"

/*!
 Displays usage information about this utility.
*/
void usage() {

  printf( "\n" );
  printf( "Usage:\n" );
  printf( "\n" );
  printf( "  General Covered information:\n" );
  printf( "    covered -v       Display current Covered version\n" );
  printf( "    covered -h       Display this usage information\n" );
  printf( "\n" );
  printf( "  To generate coverage database:\n" );
  printf( "    covered score -t <top-level_module_name> -vcd <dumpfile> [<options>]\n" );
  printf( "\n" );
  printf( "    Options:\n" );
  printf( "      -o <database_filename>  Name of database to write coverage information to.\n" );
  printf( "      -I <directory>          Directory to find included Verilog files\n" );
  printf( "      -f <filename>           Name of file containing additional arguments to parse\n" );
  printf( "      -y <directory>          Directory to find unspecified Verilog files\n" );
  printf( "      -v <filename>           Name of specific Verilog file to score\n" );
  printf( "      -e <module_name>        Name of module to not score\n" );
  printf( "      -q                      Suppresses output to standard output\n" );
  printf( "\n" );
  printf( "      +libext+.<extension>(+.<extension>)+\n" );
  printf( "                              Extensions of Verilog files to allow in scoring\n" );
  printf( "\n" );
  printf( "    Note:\n" );
  printf( "      The top-level module specifies the module to begin scoring.  All\n" );
  printf( "      modules beneath this module in the hierarchy will also be scored\n" );
  printf( "      unless these modules are explicitly stated to not be scored using\n" );
  printf( "      the -e flag.\n" );
  printf( "\n" );
  printf( "  To merge a database with an existing database:\n" );
  printf( "    covered merge [<options>] <existing_database> <database_to_merge>\n" );
  printf( "\n" );
  printf( "    Options:\n" );
  printf( "      -o <filename>           File to output new database to.  If this argument is not\n" );
  printf( "                              specified, the <existing_database> is used as the output\n" );
  printf( "                              database name.\n" );
  printf( "\n" );
  printf( "  To generate a coverage report:\n" );
  printf( "    covered report [<options>] <database_file>\n" );
  printf( "\n" );
  printf( "    Options:\n" );
  printf( "      -m [l][t][c][f]         Type(s) of metrics to report.  Default is ltc.\n" );
  printf( "      -v                      Provide verbose information in report.  Default is summarize.\n" );
  printf( "      -i                      Provides coverage information for instances instead of module.\n" );
  printf( "      -o <filename>           File to output report information to.  Default is standard output.\n" );
  printf( "\n" );

}

/*!
 \param argc Number of arguments specified in argv parameter list.
 \param argv List of arguments passed to this process from the command-line.
 \return Returns 0 to indicate a successful return; otherwise, returns a non-zero value.

 Main function for the Covered utility.  Parses command-line arguments and calls
 the appropriate functions.
*/
int main( int argc, char** argv ) {

  int retval     = 0;    /* Return value of this utility */

  /* Initialize error suppression value */
  set_output_suppression( FALSE );

  if( argc == 1 ) {

    print_output( "Must specify a command (score, merge, report, -v, or -h)", FATAL );
    retval = -1;

  } else {

    if( strncmp( "-v", argv[1], 2 ) == 0 ) {

      /* Display version of Covered */
      print_output( COVERED_VERSION, NORMAL );

    } else if( strncmp( "-h", argv[1], 2 ) == 0 ) {

      usage();

    } else if( strncmp( "score", argv[1], 5 ) == 0 ) {

      printf( COVERED_HEADER );
      retval = command_score( argc, argv );

    } else if( strncmp( "merge", argv[1], 5 ) == 0 ) {

      printf( COVERED_HEADER );
      retval = command_merge( argc, argv );

    } else if( strncmp( "report", argv[1], 6 ) == 0 ) {

      printf( COVERED_HEADER );
      retval = command_report( argc, argv );

    } else {

      print_output( "Unknown command.  Please see \"covered -h\" for more usage.", FATAL );
      retval = -1;

    }

  }

  return( retval );

}

