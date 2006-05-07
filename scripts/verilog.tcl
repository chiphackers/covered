# Highlight color values (this will need to be configurable via the preferences window
set vlog_hl_mode            on
set vlog_hl_ppkeyword_color ForestGreen
set vlog_hl_keyword_color   purple
set vlog_hl_comment_color   blue
set vlog_hl_value_color     red
set vlog_hl_string_color    red
set vlog_hl_symbol_color    coral

# Performs substitution using a user-specified command
proc regsub-eval {re string cmd} {
  
  subst [regsub $re [string map {\[ \\[ \] \\] \$ \\$ \\ \\\\} $string] "\[$cmd\]"]

}

# Preprocesses a specified Verilog file, performing 
proc preprocess_verilog {txt} {

  puts $txt

  return $txt

}

# Main function called to read in the contents of the specified filename into
# the fileContent array.
proc load_verilog {fname pp} {

  global fileContent

  set retval 1

  # Save off current directory
  set cwd [pwd]

  # Set current working directory to the score directory
  cd [tcl_func_get_score_path]

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
  }

  # Return current working directory
  cd $cwd

  return $retval

}

# Creates the include_enter, include_button and include_leave tags for the specified textbox,
# underlining the filenames of all included files.
# - tb     = pathname to textbox
# - inform = pathname to information window
# - cmd    = Name of command to call when an indices is selected by the user -- must take one argument: the name of the selected file
proc handle_verilog_includes {tb inform cmd} {
  
  global cov_fgColor
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

  # Create list of all preprocessor directives
  set keywords [list always and assign begin buf bufif0 bufif1 case casex casez cmos deassign default defparam disable edge else end endcase \
                     endfunction endmodule endprimitive endspecify endtable endtask event for force forever fork function highz0 highz1 if \
                     initial inout input integer join large localparam macromodule medium module nand negedge nmos nor not notif0 notif1 or \
                     output parameter pmos posedge primitive pull0 pull1 pulldown pullup rcmos real realtime reg release repeat rnmos rpmos rtran \
                     rtranif0 rtranif1 scalered signed small specify specparam strong0 strong1 supply0 supply1 table task time tran tranif0 tranif1 \
                     tri tri0 tri1 triand trior trireg vectored wait wand weak0 weak1 while wire wor xnor xor]
  set ilist    ""

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
  
  global vlog_hl_mode
  global vlog_hl_keyword_color vlog_hl_comment_color vlog_hl_ppkeyword_color
  global vlog_hl_value_color   vlog_hl_string_color  vlog_hl_symbol_color

  if {$vlog_hl_mode == on} {

    # Highlight all keywords
    verilog_highlight_keywords $tb $vlog_hl_keyword_color

    # Highlight all values
    verilog_highlight_values $tb $vlog_hl_value_color

    # Highlight all preprocessor keywords
    verilog_highlight_ppkeywords $tb $vlog_hl_ppkeyword_color

    # Highlight all symbols
    verilog_highlight_symbols $tb $vlog_hl_symbol_color

    # Highlight all string
    verilog_highlight_strings $tb $vlog_hl_string_color

    # Highlight all comments
    verilog_highlight_comments $tb $vlog_hl_comment_color

  }

}
