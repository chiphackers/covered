#ifndef __DEVEL_DOC_H__
#define __DEVEL_DOC_H__

/*
 \file     devel_doc.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     6/11/2002
 \brief    Contains development documentation for project
*/

/*!
 \mainpage Covered:  Verilog Code-Coverage Analyzer
 This document is meant to be a living document, that is, it is subject to change and
 updates as the project continues and evolves.  There may be incomplete information in
 certain portions of this documentation that will be filled in with more detail as the
 code is these areas stabilize.
 
<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
 - \ref page_misc
*/
/*!
 \page page_intro Section 1.  Introduction
 \par
 This documentation is specific to the development of the Covered tool.  For usage-specific
 information, please consult the Covered User's Guide which is accessible via tarball
 download or off of the Covered hompage.

 \par
 Welcome to Covered development!  Since you are reading this document, it is assumed that you
 are either on the development team, looking to join the team, or are just interested in
 some of the "under the hood" technical details about how Covered is intended to get the
 job done.  This document will seek to specify the overall project development plan; coding
 methodologies; project communication guidelines; programs/utilities needed for code
 development and documentation generation; the code's "big picture"; the nitty-gritty 
 details for all structures, functions and defines; testing procedure; and some odds and 
 ends information.  This document will serve as a technical reference as well as the 
 "Covered constitution" on programming guidelines for this project.

 \par 
 But first of all, what is the purpose of this project?  Covered is a Verilog code coverage 
 analyzation utility that allows a user to examine the effectiveness of a suite of diagnostics
 testing a design-under-test (DUT).  \b TBD

<HR>

 \par Go To Section...
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
 - \ref page_misc 
*/

/*!
 \page page_project_plan Section 2.  Project Plan
 \par Section 2.1.  Project Goals for Usability
 The goals of the Covered project as it pertains to its users are as follows:

 \par
 -# Have the ability to parse all legal Verilog code as defined by the Verilog 2000 LRM
 -# Generate concise, human-readable summary reports for line, toggle, combinational,
    and FSM state and arc coverage in which a user may quickly discern the amount of
    coverage achieved.
 -# Generate concise, human-readable verbose reports for line, toggle, combinational,
    and FSM state and arc coverage in which a user may easily discern why certain coverage
    results did not achieve 100% coverage.  Verbose reports should contain all information
    necessary for diagnosing the cause for lack of coverage without being excessive in
    the amount of information provided to aid in readability.
 -# Allow the ability to easily merge generated Coverage Description Database (CDD) files
    to allow users to accumulate coverage results for a particular design.
 -# Have the ability to parse designs, generate coverage results, and generate reports via
    command-line calls and through the use of a GUI.
 -# Provide sufficient user documentation to understand how to use the tool and understand
    its output.
 -# Provide users with additional ways to get questions answered and submit bug reports/
    enhancement requests via the following mechanisms:  FAQ, bug reporting facility,
    mailing list, user manual, and Homepage "News" section.

 <HR>

 \par Section 2.2.  Project Goals for Development
 The goals of the Covered project as it pertains to ease in development are as follows:

 \par
 -# Write source code using C language using standard C libraries for cross-platform
    compatibility purposes.
 -# Maintain sufficient amount of in-line documentation to understand purpose of functions,
    structures, defines, variables, etc.
 -# Use autoconf and automake to generate configuration files and Makefiles that will
    be able to compile the source code on any UNIX-based operating system.
 -# Use CVS for project management and file revision purposes, allowing outside developers
    to contribute to source code.
 -# Use the Doxygen utility for generating documentation from source files that is used
    for development reference.
 -# Develop self-checking, self-contained diagnostic regression suite to verify new and
    existing features of code to assure that new releases are backwards compatible to
    older versions of the tool and that new features have been tested adequately prior to
    tool releases to the general public.
 -# Provide sufficient development documentation to allow new and existing developer's to 
    understand how Covered works, the procedures/practices used in the development process 
    and other development-specific information.
 -# Provide developer's with a means of communicating project ideas, status or other
    announcements to other project developers via the following mechanisms:  CVS,
    mailing lists, and bug reporting facility.

 <HR>

 \par Section 2.3.  Project Goals for Distribution
 The goals of the Covered project as it pertains to ease in project releases and distributions
 are as follows:

 \par
 -# Provide source code in tarball format (tar'ed and gzip'ed) which will be accessible via
    the Covered homepage.
 -# Provide any links to RPMs, Debian packages, etc. that others provide for the project.
 -# Generate new releases/information on a timely basis so that users of the tool do not 
    question whether development is still occurring with the project.
 -# Generate stable releases for users.
 -# Generate development releases in CVS for branching and regression purposes.
 -# Verify that user documentation does not become stale but rather is synchronized with
    the current release.
    
<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
 - \ref page_misc 
*/

/*!
 \page page_code_style Section 3.  Coding Style Guidelines
 \par Section 3.1.  Preamble
 The guidelines to follow when writing code are here to make the entire project look as
 though it has been written by only one developer.  They are intended to keep the code easy
 to read and understand.  Many of the documentation guidelines are in place to keep the
 generated documentation consistent and helpful for development of the project.  These
 guidelines are intended to be as minimal as possible while still keep the project as
 consistent as possible.  Other ideas to improve code readability and usefulness are
 encouraged to be shared with the rest of the project.  By no means are these guidelines 
 meant to make its developers feel restricted in their coding styles!

<HR>

 \par Section 3.2.  Documentation Style Guidelines
 The Covered project uses a combination of standard C comments embedded in the code as well
 as special comments that are parsable by the Doxygen utility.  The Doxygen tool is used to
 generate all of the development documentation for the project in HTML and Latex versions.
 This allows the documentation to be viewable via an HTML browser, Acrobat reader, LaTeX
 viewers (and other related viewers), and embedded in the code itself.  For documentation on
 the usage of Doxygen, please see examples within the Covered project and/or check out the
 available documentation at the Doxygen website:

 \par
 http://www.stack.nl/~dimitri/doxygen/index.html

 \par
 The following are a list of guidelines that should be followed whenever/wherever possible
 in the source code in the area of documentation.
 
 \par
 -# All header files must begin with a Doxygen-style header.  For an example of what these 
    headers look like, please see the file signal.h
 -# All source files must begin with a Doxygen-style source header.  For an example of what
    these headers look like, please see the file signal.c
 -# All files should contain the RCS file revision history information at the bottom
    of the file.  This is accomplished by placing the string \c $Log$
    of the file.  This is accomplished by placing the string \c Revision 1.1  2002/06/21 05:55:05  phase1geo
    of the file.  This is accomplished by placing the string \c Getting some codes ready for writing simulation engine.  We should be set
    of the file.  This is accomplished by placing the string \c now.
    of the file.  This is accomplished by placing the string \c at the bottom of the
    file.
 -# All defines, structures, and global variables should contain a Doxygen-style comment 
    describing its meaning and usage in the code.
 -# Each function declaration in the header file should contain a Doxygen-style brief, one
    line description of the function's use.
 -# Each function definition in the source file should contain a Doxygen-style verbose
    description of the function's parameters, return value (if necessary), and overall
    description.
 -# All internal function variables should be documented using standard C-style comments.
 
 \par
 The most important guideline is to keep the code documentation consistent with other
 documentation found in the project and to keep that documentation up-to-date with the code
 that it is associated with.  Out-of-date documentation is usually worse than no documentation
 at all.

<HR>

 \par Section 3.3.  Coding Style Guidelines
 The following are a list of guidelines that should be followed whenever/wherever possible
 in the source code in the area of source code.

 \par
 -# Avoid using tabs in any of the source files.  Tabs are interpreted differently by all
    kinds of editors.  What looks well-formatted in your editor, may be messy and hard to
    read in someone else's editor.  Please use only spaces for formatting code.
 -# All defines and global structures are defined in the defines.h file.  If you need to
    create any new defines and/or structures for the code, please place these in this file
    in the appropriate places.
 -# For all header files, place an 

 \code
 #ifndef __<uppercase_filename>__
 #define __<uppercase_filename>__
 ...
 #endif
 \endcode

 \par
 around all code in the file.

 \par
 The most important guideline is to keep the code consistent with other code found in the
 project as it will keep the code easy to read and understand for other developer's.

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
 - \ref page_misc 
*/

/*!
 \page page_tools Section 4.  Development Tools
 \par
 The following is a list and description of what outside tools are used in the development
 of Covered, how they are used within the project, and where to find these tools.

 \par Section 4.1.  Doxygen
 Doxygen is a command-line tool that takes in a configuration file to specify how to generate
 the appropriate documentation.  The name of Covered's Doxygen configuration file is
 located in the root directory of Covered called covered.dox.  To generate documentation for
 the source files, enter the following command:

 \par
 \c doxygen \c covered.dox

 \par
 The Covered project uses Doxygen's source file documentation extraction capabilities for
 generating this developer's document.  The output of Doxygen is two directories underneath
 the \c covered/doc directory:  html and latex.  It also places a Makefile in the latex
 directory for creating PDF versions of the documentation.

 \par
 The input files for Doxygen are all *.h and *.c files located in the \c covered/src
 directory.  Because Doxygen is unable to understand/parse Flex and Bison files, the *.l
 and *.y files are omitted from documentation generation.  Placing Doxygen-style comments
 in these files will not result in developer documentation apart from the source file itself.

 \par
 For a complete description on how to use Doxygen and where to download it from, check out
 the Doxygen homepage at:

 \par
 http://www.stack.nl/~dimitri/doxygen/index.html

<HR>

 \par Section 4.2.  ManStyle
 The ManStyle project is basically an HTML document generator with an easy to use GUI 
 interface.  It was used in Covered to create the user's manual and is mentioned mostly
 for credit sake.  The \c covered/manstyle directory contains the source files that
 ManStyle creates when you use the GUI.  If you are interested in updating the user
 manual and would like to use the ManStyle utility for doing so, the main project file to
 be opened, when performing an "open" procedure in ManStyle, is "user".  Opening this file
 will load the entire user's manual for the project.  Note that the user's manual is an
 HTML document only and, as such, may be edited with any editor.

 \par
 You can download ManStyle from the following website:

 \par
 http://manstyle.sourceforge.net/index.php3

<HR>

 \par Section 4.3.  CVS
 CVS is used as the file revision and project management tool for Covered.  The CVS server
 is provided by SourceForge ( http://sourceforge.net ).  It was chosen due its ability to
 allow multiple developer's from all over the globe to access and work on this project
 simultaneously.  It was also chosen because of its availability for most development
 platforms.

<HR>

 \par Section 4.4.  MPatrol
 Like most good C codes, Covered performs a lot heap memory allocations and deallocations,
 using lots of pointers to keep track of memory locations.  As such, it is possible that
 during development and debugging that a memory leak or memory allocation/deallocation error
 will occur (go figure).  To aid in dynamic memory allocation/deallocation debugging, the
 Covered project uses a library called "MPatrol" which contains all of the dynamic memory
 functions available for most C codes but are special in that they can track memory
 allocations, memory deallocations, and related function failures and display this in
 meaningful output files.  This utility has saved a lot of time and frustration so far in
 the project development and will probably serve a great deal more use in the future.

 \par
 Since MPatrol is a library, it may be linked with the executable (overriding the standard
 functions) or may not be linked, allowing the use of the standard library.  By default,
 MPatrol is not linked with the executable as MPatrol does add some overhead to the overall
 runtime of Covered.  This is the mode that we want to release to Covered user's.  However,
 during development, it may be useful, if not necessary, to link in the MPatrol library.
 This is simply achieved by calling the \c ./configure script in the following way:

 \par
 \c ./configure \c -with-mpatrol

 \par
 This will create the appropriate Makefiles to include the MPatrol library into all
 compiles of the project.  Of course, this means that if you want to try compiling without
 MPatrol (after turning it on), you will have to call \c ./configure again and recompile
 the project.

 \par
 The MPatrol library and associated documentation can be downloaded from the following
 website:

 \par
 http://www.cbmamiga.demon.co.uk/mpatrol/

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
 - \ref page_misc 
*/

/*!
 \page page_big_picture Section 5.  Project "Big Picture"
 \par Section 5.1.  Covered Big Picture
 The following diagram illustrates the various core functions of Covered and how they
 are integrated into the tool.

 \image html  big_picture.png "Figure 1.  Data Flow Diagram"
 \image latex big_picture.eps "Figure 1.  Data Flow Diagram"

<HR>

 \par Section 5.2.  Functional Block Descriptions
 The following subsections describes each of these functions/nodes in greater detail.

 \par Section 5.2.1.  Verilog Parser
 The Verilog parser used by Covered consists of a Flex lexical analyzer lexer.l and 
 Bison parser parser.y .  Both the lexer and parser where used from the Icarus Verilog
 project which can be accessed at 

 \par
 http://icarus.com/eda/verilog/index.html

 \par
 Though most of the structure of both of these files maintain their original appearance from
 the Icarus project, most of the internal rule code has been removed and re-implemented to
 suite Covered's needs from the parser.  The parser and lexer work together as most language
 parsers do with the lexer reading in tokens of information from the input files and passing
 these tokens for the parser to match with pre-existing language rules.  The reason for taking
 both the lexer and parser from the Icarus project is that the Icarus project is well-used
 by the gEDA community for Verilog simulation and passes in regression the IV testsuite.
 This testsuite is part of the Verilog testsuite for Covered and is available for download at

 \par
 http://ivtest.sourceforge.net

 \par
 Using the Verilog directory and file pathnames specified on the command-line, Covered
 generates a list of files to search for Verilog module names.  The first module name that
 Covered attempts to find is the top-level module targetted for coverage.  This module name
 is also specified on the command-line with the \c -t option.  The lexer reads in the file
 and finds the name specified after the Verilog keyword \c module.  If this module matches
 the top-level module name, the contents of the module are parsed.  If the module name does
 not match the top-level module name, the lexer skips the body of the module until it 
 encounters the \c endmodule keyword.  If there are any more modules specified in the given
 file, these are parsed in the same fashion.  If the end of the file has been reached and
 no module has been found that is needed, the Verilog file is placed at the end of the file 
 queue and the next file at the head of the queue is read in by the lexer.

 \par
 If the top-level module contains module instantiations that also need to be tested for
 coverage, these module names are placed at the tail of the needed module queue.  When the
 needed module queue is empty, all modules have been found and parsed, and the parsing phase
 of the procedure is considered complete and successful.

 \par
 When the parser finds a match for one of its rules, an action is taken.  This action is
 typically one of the following:

 \par
 -# Create a new structure for data storage.
 -# Store a structure into one of the lists or trees for later retrieval.
 -# Manipulate a structure based on some information parsed.
 -# Display an error message due to finding code that is incorrect language structure.

 \par
 The Verilog parser only sends information to and gets information from the database
 manager.  When parsing is complete, the database manager contains all of the information
 from the Verilog design which is stored in special containers which are organized by
 a group of trees and lists.

<HR>

 \par Section 5.2.2.  Database Manager
 \b TBD

<HR>

 \par Section 5.2.3.  CDD Parser
 The Coverage Description Database file, or CDD as it is referred to in this documentation,
 is a generalized description of a Verilog design that contains coverage-specific information
 as it pertains to that design.  CDD files are in ASCII text format.  The reasons for having
 this file format are three-fold.
 
 \par
 -# Allow a way to store information about a particular design in a way that is compact and
    concise.  It is understood that a CDD file may exist for an indeterminant amount of time
    so it is important that the file size be as small as possible while still carrying
    enough information to generate useful coverage reports.
 -# Create a standardized output format that is easy to parse (can be done with the sscanf
    utility in a straight-forward way) not requiring the use and overhead of another lexer 
    and parser.  The standardization of the file format allows several CDDs to be easily
    merged and output in the same format.
 -# Create a format that is flexible enough to add new constructs as needed to support the
    growing Verilog language while not making it more difficult to parse.

 \par
 The generic output format for the CDD file is as follows:

 \par
 \c <unique_numeric_ID_for_construct> \c <information_about_construct_separated_by_spaces>

 \par
 If a new construct needs to be added to the tool, one merely needs to select a unique ID
 for that construct and come up with a format for displaying the information for that
 construct so that it is separated by spaces or commas and contains only one ENDLINE character
 at the end of the line.  Blank lines or comments are allowed within the file.  The current
 constructs that are output to a CDD file by Covered are listed below along with their
 unique ID.

 \par
 - signal (1)
 - expression (2)
 - module (3)
 - statement (4)

 \par
 The information format for each construct is listed with the description of the construct
 in the \ref page_code_details section.

 \par
 Each line of the CDD file is read into memory by the CDD parser and the first value of
 the line (the unique construct ID) is used to call the \c db_read function of the
 associated construct.  The construct then takes the initiative of decoding the rest of
 the line and storing its contents into the same set of lists and trees that the Verilog
 parser stores its information into.  The end result of parsing in the CDD file is the same
 as parsing in a design by the Verilog parser.  Once into memory, the information can be
 merged, simulated, or computed on by the report generator.

<HR>

 \par Section 5.2.4.  CDD Generator
 The CDD generator is actually distributed among the various constructs that make up the CDD
 file.  The main \c db_write function located in db.c calls each of the construct's \c db_write
 functions which, in turn, output their information to the CDD file in their own format.
 After each of the stored constructs have written their information to the CDD file, it is
 closed by the \c db_write function.  The end result is a CDD file that is in the same
 format as the CDD file that was read in by the CDD parser.

<HR>

 \par Section 5.2.5.  VCD Parser
 After a design or CDD has been stored internally into the database manager's memory, that
 memory may be merged with the data stored in another CDD, used to generate a report, or
 simulated with the use of the input from a VCD file that was created from the design loaded
 into the database manager's memory.  In the last scenario, the VCD parser is invoked to
 read in the contents of the specified VCD dumpfile.  The VCD parser can read in any
 VCD file that is output according to the VCD format.  The format of VCD can be found at:

 \par
 http://www-ee.eng.hawaii.edu/~msmith/ASICs/HTML/Verilog/LRM/HTML/15/ch15.2.htm#pgfId=250

 \par
 The VCD parser is written using a lexer and parser combination.  The lexer (vcd_lexer.l)
 is compiled using the Flex utility and the parser (vcd_parser.y) is compiled using the 
 Bison utility.

<HR>

 \par Section 5.2.6.  Coverage Simulation Engine
 \b TBD

<HR>

 \par Section 5.2.7.  Report Generator
 \b TBD

<HR>

 \par Section 5.3.  Covered Command Flow
 \b TBD

 \par Section 5.3.1.  Score Command
 \b TBD

<HR>

 \par Section 5.3.2.  Merge Command
 \b TBD

<HR>

 \par Section 5.3.3.  Report Command
 \b TBD

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_code_details
 - \ref page_testing
 - \ref page_misc 
*/

/*!
 \page page_code_details Section 6.  Coverage Development Reference
 \par Section 6.1.  Extracted Documentation
 The following links will take you to the generated documentation for the project.  This
 documentation is always in sync with the current CVS snapshot.

 \par
 - <A HREF="modules.html">Modules</A> - A list of related defines, structures, unions, etc.
 - <A HREF="annotated.html">Structures and Unions</A> - A list of structures/union descriptions.
 - <A HREF="files.html">File List</A> - A list of all source/header file descriptions.
 - <A HREF="functions.html">Function List</A> - A list of all function descriptions.
 - <A HREF="globals.html">File Members</A> - Descriptions of all file members for a given file.
 - <A HREF="bug.html">Bug List</A> - List of known bugs within the code for future fixes.

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_testing
 - \ref page_misc
*/

/*!
 \page page_testing Section 7.  Test and Checkout Procedure
 \par Section 7.1.  Testing Methodology
 Testing the Covered tool for general "goodness", which is required for release, is 
 accomplished with its own suite of C and Verilog diagnostics.  These suite of tests are run 
 in a regression manner; that is, each diagnostic is self-checking and run in serial order.  
 The results of each diagnostic are output to standard output as well as an output file.  After
 all diagnostics are run, the output file is grep'ed for the keyword "PASSED".  The number of
 diagnostics finishing the PASS message are compared against the total number of diagnostics.
 The results of which are output to standard output.

 \par
 To release a new version of the tool for general consumption, the following testing procedures
 are required to occur prior to the release.
 -# New C/Verilog diagnostics are written to test new features of tool.  These diagnostics will
    be self-contained and self-checking, displaying a message of "PASSED" if the diagnostic
    has successfully tested the feature under test or some message displaying the cause of
    failure.  The failure message may not contain the keyword "PASSED" in its description.
 -# These newly written diagnostics are added to the regression suite, the list of which
    is maintained in the Makefile located in the diagnostic directory.
 -# A regression run is run in both the C diagnostic directory as well as the Verilog
    diagnostic directory.
 -# 100% of the diagnostics in the regression suite result in a PASS message for both the C
    and Verilog directories.

<HR>

 \par Section 7.2.  Testing Directories
 The reason for having two directories for regression testing relies on the feature under test.
 Verilog diagnostics are condensed DUTs which only contain the required code for testing a
 particular syntax of the Verilog language to verify that Covered is able to correctly parse
 the code and generate the appropriate coverage results for that feature.  All Verilog
 diagnostics are accompanied by a text file that is used for comparison purposes.  The
 Makefile, after simulating the Verilog file, creating the dumpfile, generating the CDD and
 generating a verbose report based on the CDD will compare the generated report to the text
 file by performing a UNIX "diff" command.  If the results of the "diff" are no differences
 between the two files, the Makefile will assume that the diagnostic has successfully passed
 and output the keyword "PASSED" to the output result file.  If the results of "diff" show
 that there are differences between the two files, the Makefile will assume failure and
 output the keyword "FAILED" to the output result file.

 \par
 C diagnostics exist to test certain functions of Covered rather than the entire tool itself.
 Many times it is impossible/impractical to create Verilog diagnostics that exercise certain
 functions within Covered to completion.  In these cases, it is often easier to write more
 specialized tests that can more quickly manipulate inputs to functions and verify that all
 output values are correct.  Examples of C diagnostics that currently exist in the C 
 regression directory include tests of bitwise operators, mathematical functions, etc. 

 \par
 It is suggested that if functions can be adequately tested at a system level, that it be
 done so using Verilog diagnostics as these will get the most testing out of the entire tool.
 However, if functions are best tested in seclusion, it is suggested that the C testing
 environment be used.

<HR>

 \par Section 7.3.  Verilog Testing Procedure
 \b TBD

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_misc
*/

/*!
 \page page_misc Section 8.  Odds and Ends Information
 \par Section 8.1.  Development Team
 - Trevor Williams  (trevorw@charter.net)

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
*/

/* $Log$
/* Revision 1.1  2002/06/21 05:55:05  phase1geo
/* Getting some codes ready for writing simulation engine.  We should be set
/* now.
/* */

#endif
