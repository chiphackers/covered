#!/usr/bin/perl

# Name:     genprof
# Author:   Trevor Williams  phase1geo at gmail.com
# Date:     12/10/2007
# Usage:    genprof
# Description:
#           Generates genprof.h and genprof.c files that contain defines and structures requires for profiling purposes.

# Initialize all global variables
@funcs       = ();
@open_stack  = ();
$header      =
"/*
 Copyright (c) 2006 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/";

# Create default profile index entry
$funcs[@funcs] = "UNREGISTERED";

# Get all .c files and parse them for PROFILE keywords
opendir(DIR,".") || die "Can't open the current directory for reading: $!\n";
while( $file = readdir(DIR) ) {
  chomp($file);
  if( $file =~ /\S+\.c/ ) {
    open(IFILE,"$file") || die "Can't open $file for reading: $!\n";
    while( <IFILE> ) {
      if( /PROFILE\((\S+)\)/ ) {
        $index = $1;
        chomp($index);
        $funcs[@funcs] = $index;
        push(@open_stack,$#funcs);
      } elsif( /PROFILE_END/ ) {
        $funcs[pop(@open_stack)] .= ":TRUE";
        $next_to_end--;
      }
    }
    close(IFILE);
  }
}
closedir(DIR);

# Output the contents of genprof.h and genprof.c
open(GP_H,">genprof.h") || die "Can't open genprof.h for writing: $!\n";
open(GP_C,">genprof.c") || die "Can't open genprof.c for writing: $!\n";

print GP_H "#ifndef __GENPROF_H__\n#define __GENPROF_H__\n\n$header\n\n";
print GP_H "/*!\n \\file    genprof.h\n \\author  Trevor Williams  (phase1geo\@gmail.com)\n \\date    12/10/2007\n*/\n\n";
print GP_H "#include \"defines.h\"\n\n";
print GP_H "#define NUM_PROFILES " . @funcs . "\n\n#ifdef DEBUG\n";
print GP_C "$header\n\n#include \"genprof.h\"\n\n#ifdef DEBUG\nprofiler profiles[NUM_PROFILES] = {\n";

for( $i=0; $i<@funcs; $i++ ) {
  ($func,$timed) = split( /:/, $funcs[$i] );
  if( $timed eq "" ) {
    $timed = "FALSE";
  }
  print GP_H "#define $func $i\n";
  $func =~ tr/A-Z/a-z/;
  print GP_C "  {\"$func\", NULL, 0, 0, 0, $timed}";
  if (($i+1)<@funcs) {
    print GP_C ",";
  }
  print GP_C "\n";
}

print GP_H "\nextern profiler profiles[NUM_PROFILES];\n#endif\n\nextern unsigned int profile_index;\n\n#endif\n\n";
print GP_C "};\n#endif\n\nunsigned int profile_index = 0;\n\n";

close(GP_H);
close(GP_C);
