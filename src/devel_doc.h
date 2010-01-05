#ifndef __DEVEL_DOC_H__
#define __DEVEL_DOC_H__

/*
 Copyright (c) 2006-2010 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 \file     devel_doc.h
 \author   Trevor Williams  (phase1geo@gmail.com)
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
 - \ref page_debugging
 - \ref page_misc
*/
/*!
 \page page_intro Section 1.  Introduction
 
 \par
 This documentation is specific to the development of the Covered tool.  For usage-specific
 information, please consult the Covered User's Guide which is accessible via tarball
 download or off of the Covered homepage.

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
 analyzation utility that allows a user to examine the effectiveness of a suite of diagnostics,
 testing a design-under-test (DUT).  The goal of Covered is to allow the user to determine the
 amount of verification "done-ness" by examining several metrics:  line, toggle, memory,
 combinational logic, FSM state/state transition, and assertion coverage.  Each of these metrics
 are useful for finding logic that is currently unexercised, bits that are not toggled on/off,
 memory locations that have not been written/read, untested logical conditions, untraveled state
 machine states and/or state transitions, and unhit OVL cover assertions.  Covered is
 not intended to inform the user if the logic works correctly, however.
 
 \par
 The key to answering these questions about coverage is done in two ways.  First, a concise
 summary report is generated to indicate percentages of cases for each metric that were hit
 (tested) or missed (untested).  This quick reference can give the user a sense of how far
 along a testsuite is and also how useful (in terms of added coverage) a diagnostic is to the
 testsuite.  Second, a verbose/detailed report is generated to show the user exactly what logic
 was not tested and potentially why it was not tested.  This information is important to help
 the diagnostic writer understand how to test the design better.
 
 \par
 By using the information contained in the summary and verbose reports, a design tester can
 feel more confident about the effectiveness of a testsuite (as it relates to the amount of
 code that it tests) and they can be guided to areas of the logic that still require testing
 in a more focused fashion.

<HR>

 \par Go To Section...
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
 - \ref page_debugging
 - \ref page_misc 
*/

/*!
 \page page_project_plan Section 2.  Project Plan
 
 \par Section 2.1.  Project Goals for Usability
 
 \par
 The goals of the Covered project as it pertains to its users are as follows:

 \par
 -# Have the ability to parse all legal Verilog, Verilog-2001 and SystemVerilog code as
    defined by their individual LRMs.
 -# Generate concise, human-readable summary reports for line, toggle, memory, combinational,
    FSM state and arc, and assertion coverage in which a user may quickly discern the amount of
    coverage achieved.
 -# Generate concise, human-readable verbose reports for line, toggle, memory, combinational,
    FSM state and arc, and assertion coverage in which a user may easily discern why certain coverage
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
 
 \par
 The goals of the Covered project as it pertains to ease in development are as follows:

 \par
 -# Write source code using C language using standard C libraries for cross-platform
    compatibility purposes.
 -# Maintain sufficient amount of in-line documentation to understand purpose of functions,
    structures, defines, variables, etc.
 -# Use autoconf and automake to generate configuration files and Makefiles that will
    be able to compile the source code on any *NIX-like operating system.
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
 
 \par
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
 - \ref page_debugging
 - \ref page_misc 
*/

/*!
 \page page_code_style Section 3.  Coding Style Guidelines
 
 \par Section 3.1.  Preamble
 
 \par
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
 
 \par
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
 <ol>
   <li> All header files must begin with a Doxygen-style header.  For an example of what these 
     headers look like, please see the file vsignal.h
   <li> All source files must begin with a Doxygen-style source header.  For an example of what
     these headers look like, please see the file vsignal.c
   <li> All files should contain the RCS file revision history information at the bottom
     of the file by using the Log keyword.
   <li> All defines, structures, and global variables should contain a Doxygen-style comment 
     describing its meaning and usage in the code.
   <li> Each function declaration in the header file should contain a Doxygen-style brief, one
     line description of the function's use.
   <li> Each function definition in the source file should contain a Doxygen-style verbose
     description of the function's parameters, return value (if necessary), and overall
     description.
   <li> All internal function variables should be documented using standard C-style comments.
 </ol>
 
 \par
 The most important guideline is to keep the code documentation consistent with other
 documentation found in the project and to keep that documentation up-to-date with the code
 that it is associated with.  Out-of-date documentation is usually worse than no documentation
 at all.

<HR>

 \par Section 3.3.  Coding Style Guidelines
 
 \par
 The following are a list of guidelines that should be followed whenever/wherever possible
 in the source code in the area of source code.

 \par
 <ol>
   <li> Avoid using tabs in any of the source files.  Tabs are interpreted differently by all
        kinds of editors.  What looks well-formatted in your editor, may be messy and hard to
        read in someone else's editor.  Please use only spaces for formatting code.
   <li> Make sure that all files contain the GPL header information (see any source file to
        get a copy of this license agreement).
   <li> All defines and global structures are defined in the defines.h file.  If you need to
        create any new defines and/or structures for the code, please place these in this file
        in the appropriate places.
   <li> For all header files, place an
 </ol>

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
 - \ref page_debugging
 - \ref page_misc 
*/

/*!
 \page page_tools Section 4.  Development Tools
 
 \par
 The following is a list and description of what outside tools are used in the development
 of Covered, how they are used within the project, and where to find these tools.

 \par Section 4.1.  Doxygen

 \par
 Doxygen is a command-line tool that takes in a configuration file to specify how to generate
 the appropriate documentation.  The name of Covered's Doxygen configuration file is
 located in the doc directory of Covered called covered.dox.  To generate documentation for
 the source files, enter the following command:

 \par
 \c doxygen \c covered.dox

 \par
 The Covered project uses Doxygen's source file documentation extraction capabilities for
 generating this developer's document.  The output of Doxygen is two directories underneath
 the \c doc/devel directory:  html and latex.  It also places a Makefile in the latex
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

 \par Section 4.2.  CVS
 
 \par
 CVS is used as the file revision and project management tool for Covered.  The CVS server
 is provided by SourceForge ( http://sourceforge.net ).  It was chosen due its ability to
 allow multiple developer's from all over the globe to access and work on this project
 simultaneously.  It was also chosen because of its availability for most development
 platforms.

<HR>

 \par Section 4.3.  Valgrind

 \par
 The valgrind utility is useful for performing lots of various runtime checks.  Most UNIX
 variants come equipped with the valgrind utility.  For those environments that do not
 have this utility available, it can be downloaded and fairly easily built and installed.
 Please see its documentation for information on how to use it.

 \par
 To use valgrind for memory checking (is most useful function, in my opinion), simply run
 any command prefixed with

 \par
 \code
 valgrind --tool=memcheck -v
 \endcode

 \par
 This will run the given Covered command using Valgrind to perform memory checking.  No
 further configuration of Covered is necessary.
 
<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
 - \ref page_debugging
 - \ref page_misc 
*/

/*!
 \page page_big_picture Section 5.  Project "Big Picture"
 
 \par Section 5.1.  Covered Building Blocks
 
 \par
 To help understand the basic big picture of how Covered works "under the hood", it is
 important to understand some of the basic building blocks of Covered and their relationship
 to each other.
 
 \par Section 5.1.1.  Vectors
 
 \par
 A vector is the structure that is required to store all coverage metrics and current states
 of a particular value.  It is synonymous with a wire or register in a simulator and is the
 most basic building block used by Covered.  A vector is comprised of three main pieces of
 information:  width, lsb, and value.  The width and lsb values are used to calculate the
 boundaries of memory in the vector and allow vectors to contain information for one or more
 bits of information for a vector.
 
 \par
 The value member is an allocated array of 8-bit unsigned values large enough to store the
 amount of information as specified by the width.  Each each 8-bit value (otherwise referred
 to as a nibble within Covered) can store all of the information for a single Verilog bit.
 Each bit can contain 4-state information (two bits used to store a bit value).  The following
 two-bit values are used to represent the following simulation states:
 
 \par
 <ul>
   <li> 00 = 0
   <li> 01 = 1
   <li> 10 = x
   <li> 11 = z
 </ul>
 
 \par
 Each nibble in the value array is split up into several fields.  Please refer to the description
 of a \ref nibble for more information on the bit breakout of this element.  For more information
 on the vector structure and their usage, please refer to vector.c.
 
 \par Section 5.1.2.  Signals
 
 \par
 Vectors are nameless data holders; therefore, to properly represent a Verilog data type
 the vsignal structure was created.  A vsignal contains a name, a pointer to a vector, and a
 list of expression pointers.  The list of expression pointers is used to quickly find all
 expressions in which the vsignal is a part of.  When the value of a vsignal changes, all
 expressions in which the vsignal is a part of needs to be re-evaluated during the simulation
 phase.
 
 \par
 The list of vsignals in a given module instance is passed to the toggle report generator
 since all toggle coverage information is contained in the vsignals (i.e., toggle information is
 not contained in the expression or statement structures).  For more information on the
 vsignal structure, please refer to vsignal.c.
 
 \par Section 5.1.3.  Expressions
 
 \par
 Expressions represent unary or binary expressions within the verilog code.  Expressions are
 organized in a binary tree structure with a pointer to the parent expression and two pointers
 to the expression's child expressions.  An expression also contains a pointer to a vector
 (which stores the expression's coverage information and current value), a pointer to a vsignal
 (if the expression is a signal type), an opcode, and a 32-bit control element called the supplemental
 field (see \ref control for bit-breakout of the supplemental field).  The expression's
 state/descriptor bits are stored in the supplemental field (for more information on
 the supplemental field bit breakout, please refer to expr.c).  Expressions are used to calculate
 line, combinational logic and FSM coverage.
 
 \par Section 5.1.4.  Statements
 Statements are used by Covered for simulation only.  They do not contain any coverage information
 but instead are used to organize the order that expression trees are simulated.  A statement
 contains three main pieces of information, a pointer to the root of an expression tree (the
 parent pointer of the expression tree points to the statement structure), a pointer to the
 statement that should be executed if the statement's root expression evaluates to true (non-zero
 value), and a pointer to the statement that should be executed if the statement's root expression
 evalutes to false (zero value).  For more information regarding statements, please refer to
 statement.c.
 
 \par Section 5.1.5.  Parameters
 
 \par
 Though the parameter is a VCD dumpable value (indeed there is a parameter type for $var identifier),
 some simulators do not dump this information to the VCD.  Therefore, Covered manages the calculation
 of parameter values and handles parameter overriding properly.  The handling of parameters is probably
 the most complicated code implemented in Covered.  As such, more detail can be found in the param.c
 source file regarding parameters.
 
 \par Section 5.1.6.  Functional Units
 
 \par
 Functional units are the glue that holds all of the information for a particular Verilog scope, including
 filename, scope name, list of vsignals, list of parameters, list of expressions, list
 of statements, and Coverage summary statistic structures.  A functional unit and all structures within it
 are autonomous from all other functional units in that coverage metrics can be gathered independently
 from all other functional units.  Functional units are organized into a globally accessible list but
 a functional unit in the list has no relation to other functional units in the list.  Functional units
 are handled in func_unit.c.
 
 \par Section 5.1.7.  Instances
 
 \par
 Instances of functional units are structures that contain the instance name of the module instance and a
 pointer to the functional unit that represents that instance.  Instances are organized into a tree structure
 that resembles the Verilog hierarchy of the DUT.  The root of this globally accessible instance
 tree is called instance_root.  Instances are described in more detail below and are handled in
 the instance.c source file.
 
 <HR>
 
 \par Section 5.2.  Covered Functional Block Descriptions
 
 \par
 The following diagram illustrates the various core functions of Covered and how they
 are integrated into the tool.

 \image html  big_picture.png "Figure 1.  Data Flow Diagram"
 \image latex big_picture.eps "Figure 1.  Data Flow Diagram"

 \par
 The following subsections describes each of these functions/nodes in greater detail.

 \par Section 5.2.1.  Verilog Parser
 
 \par
 The Verilog parser used by Covered consists of a Flex lexical analyzer lexer.l and 
 Bison parser parser.y .  Both the lexer and parser where used from the Icarus Verilog
 project which can be accessed at:

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
 This testsuite is available for download at:

 \par
 http://ivtest.sourceforge.net

 \par
 Using the Verilog directory and file pathnames specified on the command-line, Covered
 generates a list of files to search for Verilog module names.  The first module name that
 Covered attempts to find is the top-level module targetted for coverage.  This module name
 is also specified on the command-line with the \c -t option.  The lexer reads in the file
 and finds the name specified after the Verilog keyword \c module.  If this module matches
 the top-level module name, the contents of the module are parsed.  If the module name does
 not match the top-level module name, the lexer takes note of the found module and the filename
 in which it found the module and skips the body of the module until it encounters the
 \c endmodule keyword.  If there are any more modules specified in the given file, these are
 parsed in the same fashion.  If the end of the file has been reached and no module has been
 found that is needed, the Verilog file is placed at the end of the file queue and the next
 file at the head of the queue is read in by the lexer.

 \par
 If the top-level module contains module instantiations that also need to be tested for
 coverage, these module names are placed at the tail of the needed module queue.  When the
 needed module queue is empty, all modules have been found and parsed, and the parsing phase
 of the procedure is considered complete and successful.

 \par
 When the parser finds a match for one of its rules, an action is taken.  This action is
 typically one of the following:

 \par
 <ol>
   <li> Create a new structure for data storage.
   <li> Store a structure into one of the lists or trees for later retrieval.
   <li> Manipulate a structure based on some information parsed.
   <li> Display an error message due to finding code that is incorrect language structure.
 </ol>

 \par
 The Verilog parser only sends information to and gets information from the database
 manager.  When parsing is complete, the database manager contains all of the information
 from the Verilog design which is stored in special containers which are organized by
 a group of trees and lists.

<HR>

 \par Section 5.2.2.  Database Manager
 
 \par
 The primary code of the database manager can be found in db.c though the database management
 is distributed among several files.  The database manager, as seen in the above diagram, is
 at the center of activity within the tool.  All Verilog and VCD file information is stored in
 the database manager and all CDD output and report output is generated from it.  The primary
 role of the database manager is to take the information from the Verilog, CDD and VCD parsers and
 populate two main global structures, an instance tree and a module list.
 
 \par
 The instance tree root is pointed to by the global variable instance_root.  The file instance.c
 contains the functions that are used to add to, search, remove from and destroy that instance
 tree.  The instance tree is composed of mod_inst elements which are constructed to match the
 Verilog hierarchy of the DUT with the top-level DUT module at the top of the instance_root tree.
 Each module instance element contains an instance name along with a pointer to a module element.
 During the parsing phase, several module instance elements may point to the same module element.
 After the parsing phase is completed an intermediate CDD file is generated in which each module
 instance is output in its entirety.  Thus when the CDD is reread for scoring, merging or reporting
 each module instance is allocated its own module element (this is necessary to avoid simulation
 errors and to allow instance-based reports to be properly generated).
 
 \par
 The module list is maintained by two pointers:  mod_head (points to head element of list) and
 mod_tail (points to the tail element of list).  New modules are always added to the tail of the
 list.  Each module element in the list holds the name of the module, the file the module was
 taken from, and a set of lists containing all of the module's signals, expressions, statements,
 and parameters.  All of the coverage information is stored in the signal and expression lists.
 For more detailed information on each of these types, see their corresponding code file in
 the detailed information section (vsignal = vsignal.c; expression = expr.c; statement = statement.c;
 parameter = param.c).  The module list is not necessary as far as keeping track of this module
 information (since the module instances point to these structures).  Rather the list is maintained
 because information retrieval is sometimes much quicker than searching the module instance tree.
 
 \par
 After the database manager has built these two structures (by getting information from the Verilog
 parser or the CDD file parser), other operations can be performed on these structures or information
 can be retrieved from them.
 
 \par
 Each of Covered's commands (score, merge, report) contains a series of phases for moving data.
 The following subsections describe the database manager's role in each of these phases.
 
 \par Section 5.2.2.1.  Score Command Phases
 
 \par
 The score command is the initial Covered command that turns Verilog and VCD input into a populated
 CDD database file.  The score command contains five phases as described below.
 
 \par
 <ol>
   <li> Parsing Phase
     <ul>
       <li> Verilog files are read in by Covered and its information stored into the instance tree
            and module list structures.
     </ul>
   <li> CDD Generation Phase
     <ul>
       <li> Instance tree structure is initially written as an unpopulated CDD file.
     </ul>
   <li> CDD Load Phase
     <ul>
       <li> Unpopulated CDD file read and information restored into instance tree and module list
            structures with each module instance receiving its own module element.
     </ul>
   <li> VCD Load and Simulation Phase
     <ul>
       <li> VCD file read and coverage design resimulated based on VCD contents.  During simulation
            coverage information is compiled and stored into proper vsignal and expression structures.
     </ul>
   <li> CDD Final Output Phase
     <ul>
       <li> Instance tree structure is rewritten as a populated CDD file.
     </ul>
 </ol>
 
 \par
 After all five phases of the score command have been completed, the resulting CDD file is ready for
 merging or reporting.  All phases of the score command can be found in the score.c and parse.c
 source files.
 
 \par
 It is important to note that after the first two phases have been completed,
 the resulting CDD file, though it doesn't contain any coverage information, contains all of the
 design information necessary for simulation.  Therefore, if multiple VCD files are needed to be scored,
 phases 1 and 2 can be performed once, the unpopulated files can be manually copied by the user and
 renamed, and phases 3, 4 and 5 can be run once for each VCD file.  This saves the time of having to
 perform phases 1 and 2 for each VCD simulation run.
 
 \par Section 5.2.2.2.  Merge Command Phases
 
 \par
 The merge command is useful for combining the coverage information from two populated CDD files into
 one populated CDD file.  The resulting CDD file is the union of the two merged CDD files.  The
 merge command contains only three phases as described below.
 
 \par
 <ol>
   <li> CDD Load Phase
     <ul>
       <li> Reads in first CDD file and stores it into instance tree and module list structures.
     </ul>
   <li> CDD Merge Phase
     <ul>
       <li> Reads in second CDD file, merging its contents into the existing instance tree and
            module list structures.  All structures now contain merged data.
     </ul>
   <li> CDD Final Output Phase
     <ul>
       <li> Outputs contents of instance tree structure to CDD file.
     </ul>
 </ol>
 
 \par
 After all three phases have been completed, the resulting CDD file is a union of the two
 input CDD files but remains in the exact same format as the CDD file read in by phase 1 of
 the merge.  All phases of the merge command can be found in the merge.c source file.
 
 \par Section 5.2.2.3.  Report Command Phases
 
 \par
 The report command is responsible for converting the cryptic CDD coverage file into human
 readable output to describe summary and/or verbose output.  The report command is composed
 of three phases as described below.
 
 \par
 <ol>
   <li> CDD Load Phase
     <ul>
       <li> Input CDD file is loaded into instance tree and module list structures.
     </ul>
   <li> Summary Statistical Gathering Phase
     <ul>
       <li> Summary statistics are calculated and stored for each metric.
     </ul>
   <li> Report Output Phase
     <ul>
       <li> Summary, Detail and/or Verbose report is output to standard output or
            specified file.
     </ul>
 </ol>
 
 \par
 The report command is the only command whose output is not a CDD file.  The report command
 treats the input CDD file as read-only and does not alter the files contents.  All phases
 of the report command can be found in the report.c source file.

<HR>

 \par Section 5.2.3.  CDD Parser
 
 \par
 The Coverage Description Database file, or CDD as it is referred to in this documentation,
 is a generalized description of a Verilog design that contains coverage-specific information
 as it pertains to that design.  CDD files are in ASCII text format.  The reasons for having
 this file format are three-fold.
 
 \par
 <ol>
   <li> Allow a way to store information about a particular design in a way that is compact and
        concise.  It is understood that a CDD file may exist for an indeterminant amount of time
        so it is important that the file size be as small as possible while still carrying
        enough information to generate useful coverage reports.
   <li> Create a standardized output format that is easy to parse (can be done with the sscanf
        utility in a straight-forward way) not requiring the use and overhead of another lexer 
        and parser.  The standardization of the file format allows several CDDs to be easily
        merged and output in the same format.
   <li> Create a format that is flexible enough to add new constructs as needed to support the
        growing Verilog language while not making it more difficult to parse.
 </ol>

 \par
 The generic output format for the CDD file is as follows:

 \par
 \c \<unique_numeric_ID_for_construct\> \c \<information_about_construct_separated_by_spaces\>

 \par
 If a new construct needs to be added to the tool, one merely needs to select a unique ID
 for that construct and come up with a format for displaying the information for that
 construct so that it is separated by spaces or commas and contains only one ENDLINE character
 at the end of the line.  Blank lines or comments are allowed within the file.  The current
 constructs that are output to a CDD file by Covered are listed below along with their
 unique ID.

 \par
 <ul>
   <li> vsignal (1; vsignal.c) </li>
   <li> expression (2; expr.c) </li>
   <li> functional unit (3; func_unit.c) </li>
   <li> statement (4; statement.c) </li>
   <li> info (5; info.c) </li>
   <li> fsm (6; fsm.c) </li>
   <li> race (7; race.c) </li>
   <li> score args (8; info.c) </li>
   <li> struct/union start (9; struct_union.c) </li>
   <li> struct/union end (10; struct_union.c) </li>
 </ul>

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
 
 \par
 The CDD generator is actually distributed among the various constructs that make up the CDD
 file.  The main \c db_write function located in db.c calls each of the construct's \c db_write
 functions which, in turn, output their information to the CDD file in their own format.
 After each of the stored constructs have written their information to the CDD file, it is
 closed by the \c db_write function.  The end result is a CDD file that is in the same
 format as the CDD file that was read in by the CDD parser.

<HR>

 \par Section 5.2.5.  VCD Parser
 
 \par
 After a design or CDD has been stored internally into the database manager's memory, that
 memory may be merged with the data stored in another CDD, used to generate a report, or
 simulated with the use of the input from a VCD file that was created from the design loaded
 into the database manager's memory.  In the last scenario, the VCD parser is invoked to
 read in the contents of the specified VCD dumpfile.  The VCD parser can read in any
 VCD file that is output according to the VCD format.  The format of VCD can be found at:

 \par
 http://www-ee.eng.hawaii.edu/~msmith/ASICs/HTML/Verilog/LRM/HTML/15/ch15.2.htm#pgfId=250

 \par
 The VCD parser is written using the fscanf and sscanf utilities.  Due to the ambiguity of
 the VCD file format, it was decided to write the parser using these utilities instead of
 the standard flex and bison.  In addition to ease of code writing, the fscanf and sscanf
 readers are more efficient than the alternative.  All VCD file parsing is contained in
 the vcd.c source file.

<HR>

 \par Section 5.2.6.  Coverage Simulation Engine
 
 \par
 Because Covered determines coverage for a simulation without participating in the actual
 Verilog simulation (this is because Covered does not "annotate" the design prior to
 compilation/simulation), a "resimulation" of the original is necessary using the VCD file
 as the means of data input.  The "resimulation" can be performed much quicker than the original
 simulation because many details that the actual Verilog simulator needs to handle and account
 for can be ignored by Covered.  Intermediate calculations are not performed in a given timestep,
 since such "glitches" can generate bad/misleading coverage information, only the last value of
 a given vsignal is used in calculations.  These optimizations/shortcuts can make resimulation
 quick but cannot be entirely eliminated.  Though it is possible to calculate toggle coverage
 using only the VCD file and no simulation, metrics like line, combinational logic and FSM
 coverage require simulation data.
 
 \par
 The simulation engine in Covered is closely tied to the VCD parser.  When the VCD parser is
 parsing the value change portion of the VCD file, value changes are recorded by associating
 a value to the signal specified by the VCD symbol.  The symbol and value are stored in a tree
 structure for quick lookup.  Entries in this table are sorted/searched by symbol string value.
 When a timestep is encountered in the VCD table, the simulation engine is invoked for that
 timestep in which it carries out two operations.
 
 \par Section 5.2.6.1.  Symbol Table Transfer Operation
 
 \par
 The first operation of the simulation engine is to transfer the information stored in
 the symbol/value table to the associated signals.  This operation is known as symbol table
 transfer and is performed by the function symtable_assign located in the symtable.c source
 file.  In this operation, the tree that contains the current timestep symbols/values (timestep_tab)
 is traversed, assigning the stored value to the signal structure that is associated with
 the stored symbol.  When this operation occurs, toggle coverage information is obtained glitch-free
 and all statements containing those signals are flagged as being modified and placed in a special
 queue known as the pre-simulation queue (please see sim.c for more details).  After all entries
 in the timestep_tab symbol tree have been traversed, the entire tree is deallocated from memory,
 ready for the next timestep information.
 
 \par Section 5.2.6.2.  Statement Simulation Operation
 
 \par
 The second operation of the simulation engine is the actual simulation itself.  The source
 code for the simulation engine is located in sim.c.  During the statement simulation operation,
 the statement at the head of the pre-simulation queue is evaluated (combinational and line coverage
 metrics are obtained at this time) and statements within its statement tree (a.k.a., statement block)
 traversed and executed accordingly.  When a statement tree has completed it is removed from the
 pre-simulation queue and the next statement in the pre-simulation queue is executed.  If a statement
 tree hits a delay or wait event, the statement pointer in the pre-simulation queue is updated to point
 to the current statement and the next statement in the pre-simulation queue is simulated.  This
 process continues until all statements in the pre-simulation queue have been simulated.

<HR>

 \par Section 5.2.7.  Report Generator
 
 \par
 The report generator is rooted in the file report.c; however, the actual job of generating a report
 is distributed among five source files:
 
 \par
 <ol>
   <li> line.c    (Line coverage output)
   <li> toggle.c  (Toggle coverage output)
   <li> comb.c    (Combinational logic coverage output)
   <li> codegen.c (Reconstructs Verilog code from expression tree information)
   <li> fsm.c     (Finite State Machine coverage output)
 </ol>
 
 \par
 Once the CDD has been loaded into the instance_root module instance tree, the report generator
 calls each metric's statistical generator.  Each statistical generator traverses through the various
 signal/expression/statement lists gaining summary coverage information for each module/instance.
 This information is stored in the statistic structure associated with each module instance in
 the design.  When outputting occurs, the statistic structure is used to generate the summary
 coverage data.  The source file for handling statistical structures can be found in stat.c.
 
 \par
 The second phase of the report generator is the output of information.  In all cases, summary
 information is output to the report (as mentioned above).  If the user has specified summary
 information only, report outputting is complete once all user specified metrics have been output
 in summary form.  If the user has specified detailed or verbose reports, the report generator
 must generate this information to the report.  Each metric accomplishes this output in different
 ways according to its metric type.  Please refer to the metric report file for more information
 in what is necessary to generate the detailed/verbose report.

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_code_details
 - \ref page_testing
 - \ref page_debugging
 - \ref page_misc 
*/

/*!
 \page page_code_details Section 6.  Coverage Development Reference
 
 \par Section 6.1.  Extracted Documentation
 
 \par
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
 - \ref page_debugging
 - \ref page_misc
*/

/*!
 \page page_testing Section 7.  Test and Checkout Procedure
 
 \par Section 7.1.  Testing Methodology
 
 \par
 Testing the Covered tool for general "goodness", which is required for release, is 
 accomplished with a suite of C and Verilog diagnostics.  These diagnostics are located in the
 "diag" directory within the main Covered directory.  These suite of tests are run 
 in a regression manner; that is, each diagnostic is self-checking and run in serial order.
 The C regression is run from the "c" directory while the Verilog diagnostic regression is run
 from the "regress" directory.  The C regression test is used to test out specific functions
 within the code that might otherwise not be adequately tested/testable in the full system.  The
 Verilog diagnostic suite is used to verify that Covered works correctly as a whole.  The
 following subsections describe the testing methodology used by both test suites.
 
 \par Section 7.1.1.  C Testing Methodology
 
 \par
 The C regression test suite consists of single file C code that includes the necessary header
 files from the "src" directory and specifies the necessary source files from the "src"
 directory in the linking phase.  Each C diagnostic must contain a "main()" routine, and it must
 print the keyword "PASSED" if the diagnostic was considered successful or print the keyword
 "FAILED" if the diagnostic was not considered successful.  This output must be sent to standard
 output as well as the regression output file "regress.log".  After all diagnostics are run, the
 output file is grep'ed for the keyword "PASSED".  The number of diagnostics finishing the PASS
 message are compared against the total number of diagnostics.  The results of which are output
 to standard output.
 
 \par Section 7.1.2.  Verilog Testing Methodology
 
 \par
 The Verilog regression suite consists of five directories:  regress, verilog, cdd, err, and rpt.  The
 regress directory is the directory where all Verilog regressions are run from.  In this directory
 is the main Makefile and supporting *.cfg files.  Each *.cfg file is named after its corresponding
 Verilog diagnostic file and in it contains all of the options to be passed to Covered on the score
 command-line.  The *.cfg file is passed to Covered using the score command's '-f' option.
 
 \par
 The verilog directory contains all of the Verilog diagnostic files, Verilog include files and Verilog
 library files necessary to run regression.  Additionally, two extra files "Makefile" (Makefile in
 charge of running the Verilog diagnostics and verifying their output) and "check_test" (Perl script
 used to perform verification of test passing) exist to handle diagnostic running and output checking.
 All output from a regression run is also placed into this directory.
 
 \par
 The cdd directory contains a generated/scored CDD file for each Verilog diagnostic.  The CDD files
 in this directory are CDD files determined to be good by the test writer.  When a diagnostic is run
 in which the output is deemed to be good, the *.cdd file from the diagnostic run is copied to this
 directory.  All CDD files in this directory are used to compare the final CDD output from a diagnostic
 run to determine if the generated CDD file is correct.  Compares are performed via the "diff" Unix
 command.

 \par
 The err directory contains error output generated from running Covered in such a way that produces
 an error.  It is used as a means of testing Covered's error detection capabilities in a regression
 environment.
 
 \par
 The rpt directory contains a generated module (*.rptM) and instance (*.rptI) report for each Verilog
 diagnostic.  Like the cdd directory, the rpt directory contains generated reports that were deemed to
 be correct by the diagnostic writer.  When diagnostic is run in which the output is deemed to be good,
 the *.rptI and *.rptM files from the diagnostic run are copied to this directory.  All report files in
 this directory are used to compare the final report outputs from a diagnostic run to determine if
 the generated report files are correct.  Compares are performed via the "diff" Unix command.
 
 \par
 When a Verilog regression is run (from the "regress" directory), all diagnostics are run through
 the score and report commands (only a handful of diagnostics are run through the merge command also)
 in the "verilog" directory.  The generated CDD, module report and instance report files are placed
 into the "verilog" directory.  When these output files are generated for a diagnostic, the check_test
 script is run for that diagnostic to compare the new outputs with the known good outputs.  If an output
 file is not found to differ from the golden version, the newly generated output file is removed from
 the verilog directory.  If an output file is found to differ from the golden version, the output file
 is not removed from the verilog directory (this makes it easier to identify which diagnostics failed
 after regression).  If all output files match, the keyword "PASSED" is sent to standard output and the
 passed counter in the "regress.output" file is incremented by one.  If at least one output file does
 not match, the keyword "FAILED" is sent to standard output and the failed counter in the "regress.output"
 is incremented by one.  If a diagnostic is considered a failure, regression continues to run until all
 diagnostics have been tested.  After all diagnostics have been run, the contents of the "regress.output"
 file is output to standard output to indicate the number of passing and failing diangostics.

<HR>

 \par Section 7.2.  Testing Directories
 
 \par
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

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_debugging
 - \ref page_misc
*/

/*!
 \page page_debugging Section 8.  Debugging
 
 \par Section 8.1.  Debugging Utilities
 
 \par
 When a bug is found using Covered, it is often useful for a developer to understand
 what utilities are available for debugging the problem at hand.  Besides using some
 standard debugger, Covered comes with several built-in debugging facilities useful for
 narrowing in on the code that is causing the problem.  They are the following:
 
 \par
 <ol>
   <li> Global Covered option -D </li>
   <li> Internal code assertions </li>
   <li> Command-Line Interface (CLI) interactive layer </li>
   <li> Internal source code profiler </li>
 </ol>
 
 \par
 The following subsections will describe what these facilities are and how they can
 be used or added to.
 
 \par Section 8.1.  Built-in Command Debugging Utility (-D option)
 
 \par
 Covered comes with a global command option (a global command is a command that can be used
 with any command) called '-D'.  When this option is specified for any command, interal
 information is output to standard output during the command run.  The information output is
 meant to help find the area of code which is causing the problem (in the case of a segfault
 or some other error which causes Covered to exit immediately) and to help understand the values
 that are being provided to the functions that are output the debug functionality.
 
 \par
 The merge and report commands currently do not emit much debugging information; however, the
 score command contains a great deal of debugging information.  Most of the debugging information
 comes from the database manager functions.  Each function in the db.c source file contains a
 single debug output statement, specifying the name of the function being executed as the values
 of the parameters passed to that function.  If any new functions are added to db.c, it is recommended
 that these functions output debug information.  Additionally, the expression_operate function in
 expr.c contains a debug output statement that is useful for tracking what Covered is doing during
 the simulation phase of the score command.  If key information is missing in any other functions,
 it is recommended that that information be displayed in debug output.
 
 \par
 To display debug information, the file that you are working with should contain the following
 code.
 
 \par
 \code
 #include <stdio.h>
 #include "util.h"
 
 extern char user_msg[USER_MSG_LENGTH];
 \endcode
 
 \par
 Once this code has been added to source file, add the debugging information using the snprintf
 function along with the \ref print_output function specified in util.c.  The following example
 specifies how to output some debug information:
 
 \par
 \code
 void foobar( char* name ) {
   
   snprintf( user_msg, USER_MSG_LENGTH, "In function foobar, name: %s", name );
   print_output( user_msg, DEBUG );
   
   // ...
   
 }
 \endcode
 
 \par
 Note that it is not necessary (or recommended) to specify a newline character after the user_msg
 string as the print_output function will take care of adding this character.
 
 \par Section 8.2.  Internal Assertions
 
 \par
 The second debugging facility that is used by Covered are C assertions provided by the assert.h
 library.  Assertions are placed in the code to make sure that Covered never attempts to access
 memory that it should not be accessing (to avoid segmentation fault messages whenever possible)
 and to verify that things are in the proper state when performing some type of function.  The
 benefit of creating an assertion is that a problem can be detected at the source (speeding up
 debugging time) and a core dumpfile can be created when Covered is about to do something bad.
 The core file can be used by a debugger to see where in the code was executing when the problem
 occurred.
 
 \par
 To use an internal assertion, make sure that the file you want to add the assertion to contains
 the following include.
 
 \par
 \code
 #include <assert.h>
 \endcode
 
 \par
 Once this header file has been included, simply use its assert function to verify a condition
 that evaluates to TRUE or FALSE.  The overhead for assertions is minimal so please don't be
 shy about putting them in wherever and whenver appropriate.
 
 <HR>

 \par Section 8.3.  Command-Line Interface

 \par
 Covered comes equipped with its own command-line interface which allows the user to interactively
 run Covered's simulator and view internal signal, expression, thread and queue information.  This
 CLI is called in the simulator (sim.c) each time a thread is executed.  This allows the user to
 'step' one thread simulation at a time, go to the 'next' timestep, or simply 'run' the simulation
 without user interaction.  The CLI provides a prompt for the user to enter a command to execute.

 \par
 For more information on how to use the CLI for debugging purposes, see the User Guide or simply
 type 'help' at the CLI command-line.  Note that this capability only exists when Covered is built
 with the --enable-debug option to the configuration script.

 <HR>

 \par Section 8.4.  Internal Source Code Profiler

 \par
 Though gcc and gprof work well as a generic source code profiling toolset, it was determined that
 it would be useful for Covered to have its own built-in profiler for the purposes of getting more
 Covered-specific information that developers might find interesting and/or useful.

 \par
 The usage of a built-in profiler does, however, require the developers of Covered to add special
 calls within the source code to allow the timers to accurately record time spent in each function.
 Some of this work is done manually and some of it is done through the use of a Perl script which
 is located in the /b src directory called /b gen_prof.pl.  The following paragraphs describe
 how to properly add this information so that the profiler can generate profiling statistics about
 this function.

 \par
 When a new function is added that the user wants to make profiling information available for, a
 macro called "PROFILE" should be called with the name of the function (capitalized) specified as the argument
 for the macro.  Likewise, if the function is meant to be timed, the PROFILE_END macro should be
 specified as the last line prior the function returning.  Please note that only one PROFILE_END can be
 specified per function, so these types of functions can only have one return() call at the very end of
 the function.  The following example shows how this is accomplished:

 \par
 \code
 bool foobar() { PROFILE(FOOBAR);
   bool retval = TRUE;
   // Do some stuff.
   PROFILE_END;
   return (retval);
 }
 \endcode

 \par
 Once these profiling commands have been added to all needed functions, the code developer must
 run the \b gen_prof.pl script in the \b src directory to add these new functions to a mapping
 structure that is used by the profiling source code.  If this script is not run, the new functions
 will not compile.  There are no arguments to this script.  It will parse all .c files looking for
 all PROFILE(...) calls, take the string specified within the call and append this information to
 a globally accessible structure located in the gen_prof.h and gen_prof.c files.  Note that these
 two files are completely generated so any hand-modifications to these files will be lost when
 the gen_prof.pl script is run again.  Please make any necessary modifications to this script
 instead.

 \par
 Since the gen_prof.pl script only parses .c files, any profiling macros specified in a .y or .l
 file will not be parsed until the code is first generated into .c files.

 \par
 Also note that certain functions should not be profiled!  All profiling code should not be profiled.
 Also the timer functions in the util.c file should not be profiled (this would cause Covered to go
 into an infinite loop if this were to occur).

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
 \page page_misc Section 9.  Odds and Ends Information

 \par Section 9.1.  Development Team
 
 \par
 - Trevor Williams  (phase1geo@gmail.com)

<HR>

 \par Go To Section...
 - \ref page_intro
 - \ref page_project_plan
 - \ref page_code_style
 - \ref page_tools
 - \ref page_big_picture
 - \ref page_code_details
 - \ref page_testing
 - \ref page_debugging
*/

#endif

