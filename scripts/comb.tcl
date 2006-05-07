set comb_ul_ip         0
set comb_curr_uline_id 0
set comb_curr_exp_id   0
set comb_bheight       -1
set curr_comb_ptr      "" 

proc K {x y} {
  set x
}

proc comb_calc_indexes {uline line first} {

  global comb_uline_indexes comb_ul_ip

  set i 0

  if {[expr $first == 0] && [expr $comb_ul_ip == 1]} {
    set i          [string first "-" $uline 0]
    set comb_ul_ip 0
  } else {
    set i [string first "|" $uline 0]
  }

  while {$i != -1} {
    if {$first == 1} {
      set first              0
      set comb_uline_indexes "$line.$i"
      set comb_ul_ip         1
    } else {
      if {$comb_ul_ip == 1} {
        lappend comb_uline_indexes "$line.[expr $i + 1]"
        set comb_ul_ip 0
      } else {
        lappend comb_uline_indexes "$line.$i"
        set comb_ul_ip 1
      }
    }
    set i [string first "|" $uline [expr $i + 1]]
  }

  if {[expr $i == -1] && [expr $comb_ul_ip == 1]} {
    lappend comb_uline_indexes "$line.end"
  }
    
}

proc display_comb_info {} {

  global comb_code comb_uline_groups comb_ulines
  global comb_uline_indexes comb_first_uline
  global comb_gen_ulines comb_curr_uline_id
  global comb_curr_cursor comb_curr_info
  global uncov_fgColor

  set curr_uline 0

  # Remove any tags associated with the toggle text box
  .combwin.f.top.t tag delete comb_enter
  .combwin.f.top.t tag delete comb_leave
  .combwin.f.top.t tag delete comb_bp1
  .combwin.f.top.t tag delete comb_bp3

  # Allow us to clear out textbox and repopulate
  .combwin.f.top.t configure -state normal

  # Clear the textbox before any insertion is being made
  .combwin.f.top.t delete 1.0 end

  # Get length of comb_code list
  set code_len [llength $comb_code]

  # Generate underlines
  generate_underlines

  # Write code and underlines
  set curr_line 1
  set first     1
  for {set j 0} {$j<$code_len} {incr j} {
    .combwin.f.top.t insert end "[lindex $comb_code $j]\n"
    incr curr_line
    .combwin.f.top.t insert end "[lindex $comb_gen_ulines $j]\n"
    comb_calc_indexes [lindex $comb_gen_ulines $j] $curr_line $first
    incr curr_line
    set first 0
    .combwin.f.top.t insert end "\n"
    incr curr_line
  }

  # Perform syntax highlighting
  verilog_highlight .combwin.f.top.t

  # Keep user from writing in text box
  .combwin.f.top.t configure -state disabled

  # Add expression tags and bindings
  if {[llength $comb_uline_indexes] > 0} {
    eval ".combwin.f.top.t tag add comb_enter $comb_uline_indexes"
    eval ".combwin.f.top.t tag add comb_leave $comb_uline_indexes"
    eval ".combwin.f.top.t tag add comb_bp1 $comb_uline_indexes"
    eval ".combwin.f.top.t tag add comb_bp3 $comb_uline_indexes"
    .combwin.f.top.t tag configure comb_bp1 -foreground $uncov_fgColor
    .combwin.f.top.t tag bind comb_enter <Enter> {
      set comb_curr_cursor [.combwin.f.top.t cget -cursor]
      .combwin.f.top.t configure -cursor hand2
      .combwin.info configure -text "Click left button for coverage info / Click right button to see parent expression"
    }
    .combwin.f.top.t tag bind comb_leave <Leave> {
      .combwin.f.top.t configure -cursor $comb_curr_cursor
      .combwin.info configure -text $comb_curr_info
    }
    .combwin.f.top.t tag bind comb_bp1 <ButtonPress-1> {
      set selected_range [.combwin.f.top.t tag prevrange comb_bp1 {current + 1 chars}]
      set redraw [move_display_down [get_expr_index_from_range $selected_range 1]]
      if {$redraw == 1} {
        set text_x [.combwin.f.top.t xview]
        set text_y [.combwin.f.top.t yview]
        display_comb_info
        .combwin.f.top.t xview moveto [lindex $text_x 0]
        .combwin.f.top.t yview moveto [lindex $text_y 0]
      } 
    }
    .combwin.f.top.t tag bind comb_bp3 <ButtonPress-3> {
      set selected_range [.combwin.f.top.t tag prevrange comb_bp3 {current + 1 chars}]
      set redraw [move_display_up [get_expr_index_from_range $selected_range 0]]
      if {$redraw == 1} {
        set text_x [.combwin.f.top.t xview]
        set text_y [.combwin.f.top.t yview]
        display_comb_info
        .combwin.f.top.t xview moveto [lindex $text_x 0]
        .combwin.f.top.t yview moveto [lindex $text_y 0]
      } 
    }
  }

}

proc display_comb_coverage {ulid} {

  global curr_funit_name curr_funit_type comb_expr_cov comb_curr_exp_id

  # Allow us to clear out text box and repopulate
  .combwin.f.bot.t configure -state normal

  # Clear the text-box before any insertion is being made
  .combwin.f.bot.t delete 1.0 end

  # Get combinational coverage information
  set comb_expr_cov ""
  tcl_func_get_comb_coverage $curr_funit_name $curr_funit_type $comb_curr_exp_id $ulid

  # Display coverage information
  .combwin.f.bot.t insert end "\n\n"
  foreach line $comb_expr_cov {
    .combwin.f.bot.t insert end "$line\n"
  }

  .combwin.f.bot.t configure -state disabled

}

proc get_expr_index_from_range {selected_range get_uline_id} {

  global comb_uline_expr comb_curr_uline_id
  global curr_funit_name curr_funit_type

  # Get range information
  set line       [lindex [split [lindex $selected_range 0] .] 0]
  set start_char [lindex [split [lindex $selected_range 0] .] 1]
  set end_char   [expr [lindex [split [lindex $selected_range 1] .] 1] - 1]

  # Get ID from string
  set id [regexp -inline -- {\d+} [.combwin.f.top.t get [lindex $selected_range 0] [lindex $selected_range 1]]]

  if {$id != -1} {

    if {$get_uline_id == 1} {

      # Get current expression underline ID for lookup purposes
      set comb_curr_uline_id $id

      # Display information in textbox
      display_comb_coverage $comb_curr_uline_id

    }

  }

  return $id
 
}

proc move_display_down {parent_index} {

  global comb_uline_expr

  set redraw 0

  # Get children list from parent expression
  if {[info exists comb_uline_expr($parent_index,children)]} {

    set children $comb_uline_expr($parent_index,children)

    # Iterate through all children, setting the display variable to 1
    foreach child $children {
      set comb_uline_expr($child,displayed) 1
    }

    if {[llength $children] > 0} {

      # Set parent display variable to 0
      set comb_uline_expr($parent_index,displayed) 0
      set redraw 1

    }

  }

  return $redraw

}

proc zero_display_children {parent_index} {

  global comb_uline_expr

  if {[info exists comb_uline_expr($parent_index,children)]} {

    foreach child $comb_uline_expr($parent_index,children) {

      # Clear child
      set comb_uline_expr($child,displayed) 0

      # Clear grandchild
      zero_display_children $child
    
    }

  }

}

proc move_display_up {child_index} {

  global comb_uline_expr

  set redraw 0

  # Get parent expression from child
  set parent $comb_uline_expr($child_index,parent)

  if {$parent != -1} {

    # Set parent display to 1
    set comb_uline_expr($parent,displayed) 1

    # Zero out display value for all children expressions
    zero_display_children $parent

    set redraw 1

  }

  return $redraw

}

proc comb_find_parent_expr {start_char end_char expr_line uline} {

  global comb_uline_expr

  set parent -1

  # Get the number of expressions in this expression line
  while {[expr [llength [set expr_list [array get comb_uline_expr "*,$expr_line,$uline,output"]]] > 0] && [expr $parent == -1]} {
    set i 0
    while {[expr $i < [llength $expr_list]] && [expr $parent == -1]} {
      set other_start [lindex [lindex $expr_list [expr $i + 1]] 0]
      set other_end   [lindex [lindex $expr_list [expr $i + 1]] 1]
      if {[expr $start_char >= $other_start] && [expr $end_char <= $other_end]} {
        set parent [lindex [split [lindex $expr_list $i] ,] 0]
      }
      incr i 2
    }
    incr uline
  }

  return $parent

}
 
# Calculates the current line index based on the specified expression line and underline
proc comb_calc_line_index {expr_line uline} {

  global comb_uline_groups

  set line_index 0

  for {set i 0} {$i < $expr_line} {incr i} {
    set line_index [expr $line_index + [lindex $comb_uline_groups $i]]
  }

  set line_index [expr $line_index + $uline]

  return $line_index

}

# Calculates the expression line and underline line from the given line index
proc comb_calc_from_line_index {line_index} {

  global comb_uline_groups

  set expr_line 0
  set uline     0

  for {set i 0} {$i < $line_index} {incr i} {
    if {$uline == [lindex $comb_uline_groups $expr_line]} {
      set uline 0
      incr expr_line 
    }
    incr uline
  }

  return [list $expr_line [incr uline -1]]

}

# Create a list containing locations of underlined expressions in the comb_ulines list
# Each entry in the list will be organized as follows:
#   displayed level uline_id {{index start_char end_char code_linenum}*} parent_id {children_ids...}
# parent_id == -1 means that this doesn't have a parent
# id is based on index in list
proc organize_underlines {} {

  global comb_ulines comb_uline_groups
  global comb_uline_expr

  set code_len     [llength $comb_uline_groups]
  set uline_ip     0

  # Clear the contents of the underline expression array
  array unset comb_uline_expr

  # Decompose each underline and store it in the comb_uline_expr array
  for {set i 0} {$i < [llength $comb_uline_groups]} {incr i} {
    for {set j [expr [lindex $comb_uline_groups $i] - 1]} {$j >= 0} {incr j -1} {
      set curr_uline [lindex $comb_ulines [comb_calc_line_index $i $j]]
      set vbar [string first "|" $curr_uline 0]
      set hbar [string first "-" $curr_uline 0]
      if {[expr $vbar < $hbar] && [expr $vbar != -1]} {
        set curr_char $vbar
      } else {
        set curr_char $hbar
      }
      while {[expr $curr_char != -1] && [expr $curr_char <= [string length $curr_uline]]} {
        if {$uline_ip == 0} {
          set uline_ip 1
          set start_char $curr_char
          set ulid [regexp -inline -start $curr_char -- {\d+} $curr_uline]
        } else {
          set uline_ip 0
          set comb_uline_expr($ulid,$i,$j,output) [list $start_char $curr_char [string range $curr_uline $start_char $curr_char]]
          set comb_uline_expr($ulid,displayed) 0
          set parent [comb_find_parent_expr $start_char $curr_char $i [expr $j + 1]]
          set comb_uline_expr($ulid,parent) $parent
          if {[expr [info exists comb_uline_expr($parent,children)] == 0] || [expr [lsearch -exact $comb_uline_expr($parent,children) $ulid] == -1]} {
            lappend comb_uline_expr($comb_uline_expr($ulid,parent),children) $ulid
          }
        }
        set curr_char [string first "|" $curr_uline [expr $curr_char + 1]]
      }
      if {$uline_ip == 1} {
        set uline_ip 0
        if {$curr_char == -1} { 
          set curr_char [string last "-" $curr_uline]
        }
        set comb_uline_expr($ulid,$i,$j,output) [list $start_char $curr_char [string range $curr_uline $start_char $curr_char]]
        set comb_uline_expr($ulid,displayed) 0
        set parent [comb_find_parent_expr $start_char $curr_char $i [expr $j + 1]]
        set comb_uline_expr($ulid,parent) $parent
        if {[expr [info exists comb_uline_expr($parent,children)] == 0] || [expr [lsearch -exact $comb_uline_expr($parent,children) $ulid] == -1]} {
          lappend comb_uline_expr($comb_uline_expr($ulid,parent),children) $ulid
        }
      }
    } 
  }

  # Set the display value in the children of the top-level
  move_display_down -1 

}

proc comb_calc_line_index {expr_line uline} {

  global comb_uline_groups

  set line_index 0

  for {set i 0} {$i < $expr_line} {incr i} {
    set line_index [expr $line_index + [lindex $comb_uline_groups $i]]
  }

  set line_index [expr $line_index + $uline]

  return $line_index

}

# Calculates underline strings and stores them in the comb_gen_ulines global array.
proc generate_underlines {} {

  global comb_uline_expr comb_ulines comb_code
  global comb_gen_ulines

  # Clear comb_gen_ulines
  set comb_gen_ulines ""

  # Create blank slate for underlines
  foreach code_line $comb_code {
    lappend comb_gen_ulines [string repeat " " [string length $code_line]]
  }

  set exprs [array get comb_uline_expr *,displayed]

  for {set i 0} {$i < [llength $exprs]} {incr i 2} {
    
    # If this expression is to be displayed, generate the output now
    if {[lindex $exprs [expr $i + 1]] == 1} {

      set id [lindex [split [lindex $exprs $i] ,] 0]

      # Get all line information for the given expression ID
      set lines [array get comb_uline_expr "$id,*,*,output"]

      for {set j 0} {$j < [llength $lines]} {incr j 2} {

        # Extract line information
        set uline_start [lindex [lindex $lines [expr $j + 1]] 0]
        set uline_end   [lindex [lindex $lines [expr $j + 1]] 1]
        set uline_str   [lindex [lindex $lines [expr $j + 1]] 2]
        set code_index  [lindex [split [lindex $lines $j] ,] 1]

        # Replace spaces with underline information
        set uline [lindex $comb_gen_ulines $code_index]
        set uline [string replace $uline $uline_start $uline_end $uline_str]
        set comb_gen_ulines [lreplace [K $comb_gen_ulines [set comb_gen_ulines ""]] $code_index $code_index $uline]
        
      }

    }

  }

}

# This is the main function to call when a combinational logic expression is selected from
# the main window.  Using the currently selected index, this function gets the expression ID of
# the selected expression, sets the current pointer to point to this line and calls the create_comb_window
# function to display the selected expression.
proc display_comb {curr_index} {

  global prev_comb_index next_comb_index curr_comb_ptr
  global uncovered_combs start_line

  # Calculate expression ID and line number
  set all_ranges [.bot.right.txt tag ranges uncov_highlight]
  set my_range   [.bot.right.txt tag prevrange uncov_highlight "$curr_index + 1 chars"]
  set index [expr [lsearch -exact $all_ranges [lindex $my_range 0]] / 2]
  set expr_id [lindex [lindex $uncovered_combs $index] 2]
  set sline [expr [lindex [split [lindex $my_range 0] "."] 0] + $start_line - 1]

  # Get range of current signal
  set curr_range [.bot.right.txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Calculate the current signal string
  set curr_signal [string trim [lindex [split [.bot.right.txt get [lindex $curr_range 0] [lindex $curr_range 1]] "\["] 0]]

  # Make sure that the selected signal is visible in the text box and is shown as selected
  set_pointer curr_comb_ptr [lindex [split [lindex $my_range 0] .] 0]
  goto_uncov [lindex $my_range 0]

  # Get range of previous signal
  set prev_comb_index [lindex [.bot.right.txt tag prevrange uncov_highlight [lindex $curr_index 0]] 0]

  # Get range of next signal
  set next_comb_index [lindex [.bot.right.txt tag nextrange uncov_highlight [lindex $curr_range 1]] 0]

  # Now create the toggle window
  create_comb_window $expr_id $sline

}

# Creates the verbose combinational logic coverage viewer window.  If this window already exists,
# the window is raised to the top of all windows for viewing.  All events in this window are also
# bound here.
proc create_comb_window {expr_id sline} {

  global comb_code comb_uline_groups comb_ulines comb_curr_exp_id
  global curr_funit_name curr_funit_type
  global file_name comb_curr_info
  global prev_comb_index next_comb_index
  global curr_comb_ptr

  # Now create the window and set the grab to this window
  if {[winfo exists .combwin] == 0} {

    # Create new window
    toplevel .combwin
    wm title .combwin "Combinational Logic Coverage - Verbose"

    # Create all frames for the window
    frame .combwin.f -bg grey -width 700 -height 350
    frame .combwin.f.top -relief raised -borderwidth 1
    frame .combwin.f.bot -relief raised -borderwidth 1
    frame .combwin.f.handle -borderwidth 2 -relief raised -cursor sb_v_double_arrow

    # Create frame slider
    place .combwin.f.top -relwidth 1 -height -1
    place .combwin.f.bot -relwidth 1 -rely 1 -anchor sw -height -1
    place .combwin.f.handle -relx 0.8 -anchor e -width 8 -height 8

    bind .combwin.f <Configure> {
      set H [winfo height .combwin.f]
      set Y0 [winfo rooty .combwin.f]
      if {$comb_bheight == -1} {
        set comb_bheight [expr .4 * $H]
      }
      comb_place $H $comb_bheight
    }

    bind .combwin.f.handle <B1-Motion> {
      set comb_bheight [expr $H - [expr %Y - $Y0]]
      comb_place $H $comb_bheight
    }

    # Add expression information
    label .combwin.f.top.l -anchor w -text "Expression:"
    text  .combwin.f.top.t -height 20 -width 100 -xscrollcommand ".combwin.f.top.hb set" -yscrollcommand ".combwin.f.top.vb set" -wrap none
    scrollbar .combwin.f.top.hb -orient horizontal -command ".combwin.f.top.t xview"
    scrollbar .combwin.f.top.vb -orient vertical   -command ".combwin.f.top.t yview"

    # Add expression coverage information
    label .combwin.f.bot.l -anchor w -text "Coverage Information:  ('*' represents a case that was not hit)"
    text  .combwin.f.bot.t -height 10 -width 100 -xscrollcommand ".combwin.f.bot.hb set" -yscrollcommand ".combwin.f.bot.vb set" -wrap none -state disabled
    scrollbar .combwin.f.bot.hb -orient horizontal -command ".combwin.f.bot.t xview"
    scrollbar .combwin.f.bot.vb -orient vertical   -command ".combwin.f.bot.t yview"

    # Create information bar
    label .combwin.info -anchor w -relief raised -borderwidth 1 -width 100

    # Create button frame with close/help buttons at the very bottom of the window
    frame .combwin.bf -relief raised -borderwidth 1
    button .combwin.bf.close -text "Close" -width 10 -command {
      rm_pointer curr_comb_ptr
      destroy .combwin
    }
    button .combwin.bf.help -text "Help" -width 10 -command {
      help_show_manual comb
    }
    button .combwin.bf.prev -text "<--" -command {
      display_comb $prev_comb_index
    }
    button .combwin.bf.next -text "-->" -command {
      display_comb $next_comb_index
    }
    
    # Pack the button widgets into button frame
    pack .combwin.bf.prev  -side left  -padx 8 -pady 4
    pack .combwin.bf.next  -side left  -padx 8 -pady 4
    pack .combwin.bf.help  -side right -padx 8 -pady 4
    pack .combwin.bf.close -side right -padx 8 -pady 4

    # Pack the widgets into the top frame
    grid rowconfigure    .combwin.f.top 1 -weight 1
    grid columnconfigure .combwin.f.top 0 -weight 1
    grid .combwin.f.top.l  -row 0 -column 0 -sticky nsew
    grid .combwin.f.top.t  -row 1 -column 0 -sticky nsew
    grid .combwin.f.top.hb -row 2 -column 0 -sticky ew
    grid .combwin.f.top.vb -row 1 -column 1 -sticky ns

    # Pack the widgets into the bottom frame
    grid rowconfigure    .combwin.f.bot 1 -weight 1
    grid columnconfigure .combwin.f.bot 0 -weight 1
    grid .combwin.f.bot.l  -row 0 -column 0 -sticky nsew
    grid .combwin.f.bot.t  -row 1 -column 0 -sticky nsew
    grid .combwin.f.bot.hb -row 2 -column 0 -sticky ew
    grid .combwin.f.bot.vb -row 1 -column 1 -sticky ns

    # Pack the frame, informational bar, and button frame into the window
    pack .combwin.f    -fill both -expand yes
    pack .combwin.info -fill both
    pack .combwin.bf   -fill both

  } else {

    # Clear out bottom text box
    .combwin.f.bot.t configure -state normal
    .combwin.f.bot.t delete 1.0 end
    .combwin.f.bot.t configure -state disabled

  }

  # Disable next or previous buttons if valid
  if {$prev_comb_index == ""} {
    .combwin.bf.prev configure -state disabled
  } else {
    .combwin.bf.prev configure -state normal
  }
  if {$next_comb_index == ""} {
    .combwin.bf.next configure -state disabled
  } else {
    .combwin.bf.next configure -state normal
  }

  # Get verbose combinational logic information
  set comb_code         "" 
  set comb_uline_groups "" 
  set comb_ulines       ""
  set comb_curr_exp_id  $expr_id
  tcl_func_get_comb_expression $curr_funit_name $curr_funit_type $expr_id

  # Initialize text in information bar
  set comb_curr_info "Filename: $file_name, module: $curr_funit_name, line: $sline"
  .combwin.info configure -text $comb_curr_info

  organize_underlines

  # Write combinational logic with level 0 underline information in text box
  display_comb_info

  # Raise this window to the foreground
  raise .combwin

}

proc comb_place {height value} {

  place .combwin.f.top    -height [expr $height - $value]
  place .combwin.f.handle -y [expr $height - $value]
  place .combwin.f.bot    -height $value

}

# Updates the GUI elements of the combinational logic window when some type of event causes the
# current metric mode to change.
proc update_comb {} {

  global cov_rb prev_comb_index next_comb_index curr_comb_ptr

  # If the combinational window exists, update the GUI
  if {[winfo exists .combwin] == 1} {

    # If the current metric mode is not combinational logic, disable the prev/next buttons
    if {$cov_rb != "comb"} {

      .combwin.bf.prev configure -state disabled
      .combwin.bf.next configure -state disabled

    } else {

      # Restore curr_pointer if it has been set
      set_pointer curr_comb_ptr $curr_comb_ptr

      # Restore previous/next button enables
      if {$prev_comb_index != ""} {
        .combwin.bf.prev configure -state normal
      } else {
        .combwin.bf.prev configure -state disabled
      }
      if {$next_comb_index != ""} {
        .combwin.bf.next configure -state normal
      } else {
        .combwin.bf.next configure -state disabled
      }

    }

  }

}
