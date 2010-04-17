################################################################################################
# Copyright (c) 2006-2010 Trevor Williams                                                      #
#                                                                                              #
# This program is free software; you can redistribute it and/or modify                         #
# it under the terms of the GNU General Public License as published by the Free Software       #
# Foundation; either version 2 of the License, or (at your option) any later version.          #
#                                                                                              #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;    #
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    #
# See the GNU General Public License for more details.                                         #
#                                                                                              #
# You should have received a copy of the GNU General Public License along with this program;   #
# if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. #
################################################################################################

# Performs substitution using a user-specified command
proc regsub-eval {re string cmd} {
  
  subst [regsub $re [string map {\[ \\[ \] \\] \$ \\$ \\ \\\\} $string] "\[$cmd\]"]

}

# Postprocesses a specified Verilog file that contains `line directives from preprocessor
proc postprocess_verilog {fname} {

  global fileContent

  set contents [split $fileContent($fname) \n]
  set linenum 1
  set newContent {}

  foreach line $contents {
    set pattern "`line\\s+(\\d+)\\s+\"$fname\""
    if {[regexp "$pattern" $line -> linenum] == 0} {
      set newContent [linsert $newContent [expr $linenum - 1] $line]
      incr linenum
    }
  }

  # Recreate the new file content
  set fileContent($fname) [join $newContent "\n"]

}

# Main function called to read in the contents of the specified filename into
# the fileContent array.
proc load_verilog {fname pp} {

  global fileContent

  set retval 1

  # Save off current directory
  set cwd [pwd]

  # Set current working directory to the score directory
  if {[catch "cd [tcl_func_get_score_path]" eid]} {

    tk_messageBox -default ok -icon error -message $eid -parent . -type ok
    set retval 0

  } else {

    if {[catch {set fileText $fileContent($fname)}]} {
      if {$pp} {
        set tmpname [tcl_func_preprocess_verilog $fname]
      } else {
        set tmpname $fname
      }
      if {[catch {set fp [open $tmpname "r"]}]} {
        tk_messageBox -message "File $fname Not Found!" -title "No File" -icon error
        set retval 0
      }
      set fileContent($fname) [read $fp]
      close $fp
      if {$pp} {
        file delete -force $tmpname
      }
      postprocess_verilog $fname
    }

    # Return current working directory
    cd $cwd

  }

  return $retval

}

# Creates the include_enter, include_button and include_leave tags for the specified textbox,
# underlining the filenames of all included files.
# - tb     = pathname to textbox
# - inform = pathname to information window
# - cmd    = Name of command to call when an indices is selected by the user -- must take one argument: the name of the selected file
proc handle_verilog_includes {tb inform cmd} {
 
  global curr_cursor curr_info curr_cmd curr_inform

  # Iterate through all found `include directives, creating a tag that will underline all of these
  set start_index      1.0
  set matching_chars   ""
  set include_indices  ""
  set curr_cmd($tb)    $cmd
  set curr_inform($tb) $inform
  
  # Find all include directives and add their indices to the list
  while 1 {
    set start_index [$tb search "`include" $start_index end]
    if {$start_index != ""} {
      set start_index [$tb index "[$tb search -count matching_chars -regexp {\".*\"} $start_index] + 1 chars"]
      set end_index   [$tb index "$start_index + [expr $matching_chars - 2] chars"]
      lappend include_indices $start_index $end_index
      set start_index "$start_index + 1 chars"
    } else {
      break
    }
  }

  # Remove all include tags
  $tb tag delete include_enter include_button include_leave

  if {$include_indices != ""} {

    eval "$tb tag add include_enter  $include_indices"
    eval "$tb tag add include_button $include_indices"
    eval "$tb tag add include_leave  $include_indices"

    # Configure the include_button tag
    $tb tag configure include_button -underline true
    
    # Bind the include_enter tag
    $tb tag bind include_enter <Enter> {
      set curr_cursor [%W cget -cursor]
      set curr_info   [$curr_inform(%W) cget -text]
      %W configure -cursor hand2
      $curr_inform(%W) configure -text "Click left button to view this included file"
    }
    
    # Bind the include_leave tag
    $tb tag bind include_leave <Leave> {
      %W configure -cursor $curr_cursor
      $curr_inform(%W) configure -text $curr_info
    }
    
    # Bind the include_button tag
    $tb tag bind include_button <ButtonPress-1> {
      %W configure -cursor $curr_cursor
      $curr_inform(%W) configure -text $curr_info
      eval $curr_cmd(%W) [tcl_func_get_include_pathname [eval "%W get [%W tag prevrange include_button {current + 1 chars}]"]]
    }
    
  }

}

# Finds all comments in the specified textbox and highlights them with the given color
proc verilog_highlight_comments {tb color} {

  set start_index 1.0
  set ilist       ""

  # Handle all single line comments first
  while 1 {
    set start_index [$tb search "//" $start_index end]
    if {$start_index != ""} {
      set end_index [$tb index "[lindex [split $start_index .] 0].end"]
      lappend ilist $start_index $end_index
      set start_index "$end_index + 1 chars"
    } else {
      break
    }
  }

  # Handle all multi-line comments
  set start_index 1.0
  while 1 {
    set start_index [$tb search "/*" $start_index end]
    if {$start_index != ""} {
      set end_index [$tb search "*/" "$start_index + 1 chars" end]
      if {$end_index == ""} {
        set end_index [$tb index end]
      }
      lappend ilist $start_index $end_index
      set start_index "$end_index + 1 chars"
    } else {
      break
    }
  }

  if {$ilist != ""} {

    eval "$tb tag add comment_highlights $ilist"

    # Configure the comment_highlights tag
    $tb tag configure comment_highlights -foreground $color

  }

}

# Finds all preprocessor keywords in the specified textbox and highlights them with the given color
proc verilog_highlight_ppkeywords {tb color} {

  # Create list of all preprocessor directives
  set directives [list "`line" "`define" "`include" "`ifdef" "`ifndef" "`undef" "`else" "`elseif" "`timescale" "`endif"]
  set ilist      ""

  foreach directive $directives {

    set start_index 1.0
    set len         [string length $directive]
    set match_str   ""
    append match_str $directive {($|[^a-zA-Z0-9_])}

    # Find all preprocessor directives and add their indices to the list
    while 1 {
      set start_index [$tb search -regexp $match_str $start_index end]
      if {$start_index != ""} {
        set end_index [$tb index "$start_index + $len chars"]
        lappend ilist $start_index $end_index
        set start_index "$start_index + 1 chars"
      } else {
        break
      }
    }

  }

  if {$ilist != ""} {

    eval "$tb tag add ppkeyword_highlights $ilist"

    # Configure the ppkeyword_highlights tag
    $tb tag configure ppkeyword_highlights -foreground $color

  }

}

# Finds all preprocessor keywords in the specified textbox and highlights them with the given color
proc verilog_highlight_keywords {tb color} {

  global curr_block

  # Create list of all language keywords
  set v1995_keywords [list always and assign begin buf bufif0 bufif1 case casex casez cmos deassign default defparam disable edge else end \
                           endcase endfunction endmodule endprimitive endspecify endtable endtask event for force forever fork function \
                           highz0 highz1 if initial inout input integer join large localparam macromodule medium module nand negedge nmos \
                           nor not notif0 notif1 or output parameter pmos posedge primitive pull0 pull1 pulldown pullup rcmos real realtime \
                           reg release repeat rnmos rpmos rtran rtranif0 rtranif1 scalered signed small specify specparam strong0 strong1 \
                           supply0 supply1 table task time tran tranif0 tranif1 tri tri0 tri1 triand trior trireg vectored wait wand weak0 \
                           weak1 while wire wor xnor xor]
  set v2001_keywords [list automatic cell config design endconfig endgenerate generate genvar instance liblist library localparam \
                           noshowcancelled pulsestyle_onevent pulsestyle_ondetect showcancelled use]
  set sv_keywords    [list always_comb always_ff always_latch assert bool bit break byte char continue cover do endprogram endproperty \
                           endsequence enum final int logic longint packed priority program property return sequence shortint struct \
                           typedef union unique unsigned void]
  set ilist    ""

  # Create full list based on user-specified generation
  set generation [tcl_func_get_generation [tcl_func_get_funit_name $curr_block]]
  if {$generation == 3} {
    set keywords [concat $v1995_keywords $v2001_keywords $sv_keywords]
  } elseif {$generation == 2} {
    set keywords [concat $v1995_keywords $v2001_keywords]
  } else {
    set keywords $v1995_keywords
  }

  foreach keyword $keywords {

    set start_index 1.0
    set len         [string length $keyword]
    set match_str   ""
    append match_str {(^|[^a-zA-Z0-9_`])} $keyword {($|[^a-zA-Z0-9_])}

    # Find all preprocessor directives and add their indices to the list
    while 1 {
      set start_index [$tb search -count matching_chars -regexp $match_str $start_index end]
      if {$start_index != ""} {
        if {[$tb get $start_index] != [string index $keyword 0]} {
          set start_index [$tb index "$start_index + 1 chars"]
        }
        set end_index   [$tb index "$start_index + $len chars"]
        lappend ilist $start_index $end_index
        set start_index "$start_index + 1 chars"
      } else {
        break
      }
    }

  }

  if {$ilist != ""} {

    eval "$tb tag add keyword_highlights $ilist"

    # Configure the keyword_highlights tag
    $tb tag configure keyword_highlights -foreground $color

  }

}

# Finds all defined and constant values in the specified textbox and highlights them with the given color
proc verilog_highlight_values {tb color} {

  set ilist ""

  # Highlight all numeric values
  set start_index 1.0
  while 1 {
    set start_index [$tb search -count matching_chars -regexp {(^|[^a-zA-Z0-9_])[0-9]+($|[^a-zA-Z0-9_])} $start_index end]
    if {$start_index != ""} {
      set start_index [$tb index "$start_index + 1 chars"]
      set end_index   [$tb index "$start_index + [expr $matching_chars - 2] chars"]
      lappend ilist $start_index $end_index
      set start_index "$end_index + 1 chars"
    } else {
      break
    }
  }

  set start_index 1.0
  while 1 {
    set start_index [$tb search -count matching_chars -regexp {[0-9_]*[ \t]*'[sS]?[dDbBoOhH][0-9_a-fA-F]+} $start_index end]
    if {$start_index != ""} {
      set end_index [$tb index "$start_index + $matching_chars chars"]
      lappend ilist $start_index $end_index
      set start_index "$end_index + 1 chars"
    } else {
      break
    }
  }

  # Highlight all defined values
  set start_index 1.0
  while 1 {
    set start_index [$tb search -count matching_chars -regexp {`[a-zA-Z0-9_]+} $start_index end]
    if {$start_index != ""} {
      set end_index [$tb index "$start_index + $matching_chars chars"]
      lappend ilist $start_index $end_index
      set start_index "$end_index + 1 chars"
    } else {
      break
    }
  }

  if {$ilist != ""} {

    eval "$tb tag add value_highlights $ilist"

    # Configure the value_highlights tag
    $tb tag configure value_highlights -foreground $color

  }

}

# Finds all string values in the specified textbox and highlights them with the given color
proc verilog_highlight_strings {tb color} {

  set start_index 1.0
  set ilist       ""

  while 1 {
    set start_index [$tb search \" $start_index end]
    if {$start_index != ""} {
      set end_index [$tb search \" "$start_index + 1 chars" end]
      if {$end_index == ""} {
        set end_index end
      } else {
        set end_index [$tb index "$end_index + 1 chars"]
      }
      lappend ilist $start_index $end_index
      set start_index "$end_index + 1 chars"
    } else {
      break
    }
  }

  if {$ilist != ""} {

    eval "$tb tag add string_highlights $ilist"

    # Configure the comment_highlights tag
    $tb tag configure string_highlights -foreground $color

  }

}

# Highlights all symbols in the specified textbox with the given color
proc verilog_highlight_symbols {tb color} {

  set start_index 1.0
  set ilist       ""

  while 1 {
    set start_index [$tb search -regexp {[\}\{;:\[\],()#=.@&!?<>%|^~+*/-]} $start_index end]
    if {$start_index != ""} {
      set end_index [$tb index "$start_index + 1 chars"]
      lappend ilist $start_index $end_index
      set start_index $end_index
    } else {
      break
    }
  }

  if {$ilist != ""} {

    eval "$tb tag add symbol_highlights $ilist"

    # Configure the symbol_highlights tag
    $tb tag configure symbol_highlights -foreground $color

  }

}

# Main function to highlight all Verilog syntax for the given textbox
proc verilog_highlight {tb} {
  
  global preferences

  if {$preferences(vlog_hl_mode) == "on"} {

    # Highlight all keywords
    verilog_highlight_keywords $tb $preferences(vlog_hl_keyword_color)

    # Highlight all values
    verilog_highlight_values $tb $preferences(vlog_hl_value_color)

    # Highlight all preprocessor keywords
    verilog_highlight_ppkeywords $tb $preferences(vlog_hl_ppkeyword_color)

    # Highlight all symbols
    verilog_highlight_symbols $tb $preferences(vlog_hl_symbol_color)

    # Highlight all string
    verilog_highlight_strings $tb $preferences(vlog_hl_string_color)

    # Highlight all comments
    verilog_highlight_comments $tb $preferences(vlog_hl_comment_color)

  } else {

    # Delete any current syntax highlighting tags
    $tb tag delete ppkeyword_highlights
    $tb tag delete keyword_highlights
    $tb tag delete comment_highlights
    $tb tag delete string_highlights
    $tb tag delete value_highlights
    $tb tag delete symbol_highlights

  }

}
