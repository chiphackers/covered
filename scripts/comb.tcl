set comb_level 0
set comb_ul_ip 0

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

proc check_level {level} {

  global comb_uline_groups

  # Get length of comb_code list
  set code_len [llength $comb_uline_groups]

  # First, see if this level exceeds the maximum number of levels we have for this expression
  set i 0
  while {[expr $i < $code_len] && [expr [lindex $comb_uline_groups $i] <= $level]} {
    incr i
  }

  # Return 1 if this level exists; otherwise, return 0
  if {$i < $code_len} {
    return 1
  } else {
    return 0
  }
  
}

proc display_comb_info {level} {

  global comb_code comb_uline_groups comb_ulines
  global comb_uline_indexes comb_first_uline
  global comb_curr_cursor
  global uncov_fgColor

  set curr_uline 0

  # Remove any tags associated with the toggle text box
  .combwin.f.t tag delete comb_enter
  .combwin.f.t tag delete comb_leave
  .combwin.f.t tag delete comb_bp1
  .combwin.f.t tag delete comb_bp2

  # Allow us to clear out text box and repopulate
  .combwin.f.t configure -state normal

  # Clear the text-box before any insertion is being made
  .combwin.f.t delete 1.0 end

  # Get length of comb_code list
  set code_len [llength $comb_uline_groups]

  # Write code and underlines
  set curr_line 1
  set first     1
  for {set j 0} {$j<$code_len} {incr j} {
    .combwin.f.t insert end "[lindex $comb_code $j]\n"
    incr curr_line
    set num_ulines [lindex $comb_uline_groups $j]
    if {$level < $num_ulines} {
      set uline [lindex $comb_ulines [expr ($num_ulines - $level - 1) + $curr_uline]]
      .combwin.f.t insert end "$uline\n"
      comb_calc_indexes $uline $curr_line $first
      set first 0
      incr curr_line
    }
    .combwin.f.t insert end "\n"
    incr curr_line
    set curr_uline [expr $curr_uline + $num_ulines]
  }

  # Keep user from writing in text boxes
  .combwin.f.t configure -state disabled

  # Add expression tags and bindings
  if {[llength $comb_uline_indexes] > 0} {
    eval ".combwin.f.t tag add comb_enter $comb_uline_indexes"
    eval ".combwin.f.t tag add comb_leave $comb_uline_indexes"
    eval ".combwin.f.t tag add comb_bp1 $comb_uline_indexes"
    eval ".combwin.f.t tag add comb_bp2 $comb_uline_indexes"
    .combwin.f.t tag configure comb_bp1 -foreground $uncov_fgColor
    .combwin.f.t tag bind comb_enter <Enter> {
      set comb_curr_cursor [.combwin.f.t cget -cursor]
      .combwin.f.t configure -cursor hand2
    }
    .combwin.f.t tag bind comb_leave <Leave> {
      .combwin.f.t configure -cursor $comb_curr_cursor
    }
    .combwin.f.t tag bind comb_bp1 <ButtonPress-1> {
      .combwin.f.t configure -cursor $comb_curr_cursor
      comb_down_level
    }
    .combwin.f.t tag bind comb_bp2 <ButtonPress-2> {
      .combwin.f.t configure -cursor $comb_curr_cursor
      comb_up_level
    }
  }

}

proc comb_up_level {} {

  global comb_level

  if {$comb_level > 0} {
    incr comb_level -1
    display_comb_info $comb_level
  }

}

proc comb_down_level {} {

  global comb_level

  if [check_level [expr $comb_level + 1]] {
    incr comb_level
    display_comb_info $comb_level
  }

}

proc create_comb_window {mod_name expr_id} {

  global comb_code comb_uline_groups comb_ulines
  global comb_level

  # Now create the window and set the grab to this window
  if {[winfo exists .combwin] == 0} {

    # Create new window
    toplevel .combwin
    wm title .combwin "Combinational Logic Coverage - Verbose"

    frame .combwin.f

    # Add toggle information
    label .combwin.f.l0 -anchor w -text "Expression:"
    text  .combwin.f.t -height 20 -width 100 -xscrollcommand ".combwin.f.hb set" -yscrollcommand ".combwin.f.vb set" -wrap none

    scrollbar .combwin.f.hb -orient horizontal -command ".combwin.f.t xview"
    scrollbar .combwin.f.vb -orient vertical   -command ".combwin.f.t yview"

    # Create bottom information bar
    label .combwin.f.info -anchor w

    # Pack the widgets into the bottom frame
    grid rowconfigure    .combwin.f 1 -weight 1
    grid columnconfigure .combwin.f 0 -weight 1
    grid columnconfigure .combwin.f 1 -weight 0
    grid .combwin.f.l0   -row 0 -column 0 -sticky nsew
    grid .combwin.f.t    -row 1 -column 0 -sticky nsew
    grid .combwin.f.hb   -row 2 -column 0 -sticky ew
    grid .combwin.f.vb   -row 1 -column 1 -sticky ns
    grid .combwin.f.info -row 3 -column 0 -sticky ew

    pack .combwin.f -fill both -expand yes -side bottom

  }

  # Get verbose combinational logic information
  set comb_code         "" 
  set comb_uline_groups "" 
  set comb_ulines       ""
  tcl_func_get_comb_coverage $mod_name $expr_id

  # Write combinational logic with level 0 underline information in text box
  display_comb_info 0

}
