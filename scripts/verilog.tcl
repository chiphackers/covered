# Performs substitution using a user-specified command
proc regsub-eval {re string cmd} {
  
  subst [regsub $re [string map {\[ \\[ \] \\] \$ \\$ \\ \\\\} $string] "\[$cmd\]"]

}

# Preprocesses a specified Verilog file, performing 
proc preprocess_verilog {fname args} {

}

# Main function called to read in the contents of the specified filename into
# the fileContent array.
proc load_verilog {fname} {

  global fileContent

  set retval 1

  # Save off current directory
  set cwd [pwd]

  # Set current working directory to the score directory
  cd [tcl_func_get_score_path]

  if {[catch {set fileText $fileContent($fname)}]} {
    if {[catch {set fp [open $fname "r"]}]} {
      tk_messageBox -message "File $fname Not Found!" -title "No File" -icon error
      set retval 0
    }
    set fileText [read $fp]
    set fileContent($fname) $fileText
    close $fp
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
    $tb tag configure include_button -underline true -foreground $cov_fgColor
    
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
