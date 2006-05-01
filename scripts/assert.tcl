set curr_assert_ptr ""

proc assert_yset {args} {

  eval [linsert $args 0 .assertwin.f.vb set]
  summary_yview moveto [lindex [.assertwin.f.vb get] 0]

}

proc assert_yview {args} {

  eval [linsert $args 0 .assertwin.f.td yview]
  eval [linsert $args 0 .assertwin.f.tc yview]

}

proc display_assert {curr_index} {

  global prev_assert_index next_assert_index curr_assert_ptr

  # Get range of current instance
  set curr_range [.bot.right.txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Calculate the current instance string
  set curr_inst [string trim [lindex [split [.bot.right.txt get [lindex $curr_range 0] [lindex $curr_range 1]] "\["] 0]]

  # Make sure that the selected instance is visible in the text box and is shown as selected
  set_pointer curr_assert_ptr [lindex [split [lindex $curr_range 0] .] 0]
  goto_uncov [lindex $curr_range 0]

  # Get range of previous instance
  set prev_assert_index [lindex [.bot.right.txt tag prevrange uncov_button [lindex $curr_index 0]] 0]

  # Get range of next instance
  set next_assert_index [lindex [.bot.right.txt tag nextrange uncov_button [lindex $curr_range 1]] 0]

  # Now create the assertion window
  create_assert_window $curr_inst

}

proc create_assert_window {inst} {

  global prev_assert_index next_assert_index
  global curr_funit_name curr_funit_type
  global curr_assert_ptr assert_cov_points
  global uncov_bgColor uncov_fgColor
  global cov_bgColor cov_fgColor

  # Now create the window and set the grab to this window
  if {[winfo exists .assertwin] == 0} {

    # Create new window
    toplevel .assertwin
    wm title .assertwin "Assertion Coverage - Verbose"

    frame .assertwin.f -relief raised -borderwidth 1

    # Add toggle information
    label .assertwin.f.l -anchor w -text "Missed assertion coverage points"
    text  .assertwin.f.td -height 5 -width 40 -xscrollcommand ".assertwin.f.hb set" \
          -yscrollcommand assert_yset -wrap none -spacing1 2 -spacing3 3
    text  .assertwin.f.tc -height 5 -width 10 -yscrollcommand assert_yset -wrap none -spacing1 2 -spacing3 3
    scrollbar .assertwin.f.hb -orient horizontal -command ".assertwin.f.td xview"
    scrollbar .assertwin.f.vb -command assert_yview

    # Create bottom information bar
    label .assertwin.f.info -anchor e

    # Create bottom button bar
    frame .assertwin.bf -relief raised -borderwidth 1
    button .assertwin.bf.close -text "Close" -width 10 -command {
      rm_pointer curr_assert_ptr
      destroy .assertwin
    }
    button .assertwin.bf.help -text "Help" -width 10 -command {
      help_show_manual assert
    }
    button .assertwin.bf.prev -text "<--" -command {
      display_assert $prev_assert_index
    }
    button .assertwin.bf.next -text "-->" -command {
      display_assert $next_assert_index
    }

    # Pack the buttons into the button frame
    pack .assertwin.bf.prev  -side left  -padx 8 -pady 4
    pack .assertwin.bf.next  -side left  -padx 8 -pady 4
    pack .assertwin.bf.help  -side right -padx 8 -pady 4
    pack .assertwin.bf.close -side right -padx 8 -pady 4

    # Pack the widgets into the bottom frame
    grid rowconfigure    .assertwin.f 1 -weight 1
    grid columnconfigure .assertwin.f 0 -weight 1
    grid columnconfigure .assertwin.f 1 -weight 0
    grid .assertwin.f.l    -row 0 -column 0 -sticky nsew
    grid .assertwin.f.td   -row 1 -column 0 -sticky nsew
    grid .assertwin.f.tc   -row 1 -column 1 -sticky nsew
    grid .assertwin.f.vb   -row 1 -column 2 -sticky nsew
    grid .assertwin.f.hb   -row 2 -column 0 -sticky nsew
    grid .assertwin.f.info -row 3 -column 0 -columnspan 3 -sticky new

    pack .assertwin.f  -fill both -expand yes
    pack .assertwin.bf -fill x

  }

  # Disable next or previous buttons if valid
  if {$prev_assert_index == ""} {
    .assertwin.bf.prev configure -state disabled
  } else {
    .assertwin.bf.prev configure -state normal
  }
  if {$next_assert_index == ""} {
    .assertwin.bf.next configure -state disabled
  } else {
    .assertwin.bf.next configure -state normal
  }

  # Get verbose toggle information
  set assert_cov_points ""
  tcl_func_get_assert_coverage $curr_funit_name $curr_funit_type $inst

  # Allow us to clear out text boxes
  .assertwin.f.td configure -state normal
  .assertwin.f.tc configure -state normal

  # Clear the text boxes before any insertion is being made
  .assertwin.f.td delete 1.0 end
  .assertwin.f.tc delete 1.0 end

  # Create covered/uncovered tags
  .assertwin.f.td tag configure td_uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
  .assertwin.f.td tag configure td_cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor
  .assertwin.f.tc tag configure tc_uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
  .assertwin.f.tc tag configure tc_cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor

  # Write assertion coverage point information in text boxes
  foreach cov_point $assert_cov_points {
    if {[lindex $cov_point 1] == 0} {
      .assertwin.f.td insert end "[lindex $cov_point 0]\n" td_uncov_colorMap
      .assertwin.f.tc insert end "[lindex $cov_point 1]\n" tc_uncov_colorMap
    } else {
      .assertwin.f.td insert end "[lindex $cov_point 0]\n" td_cov_colorMap
      .assertwin.f.tc insert end "[lindex $cov_point 1]\n" tc_cov_colorMap
    }
  }

  # Keep user from writing in text boxes
  .assertwin.f.td configure -state disabled
  .assertwin.f.tc configure -state disabled

  # Raise this window to the foreground
  raise .assertwin

}

# Updates the GUI elements of the assertion window when some type of event causes the
# current metric mode to change.
proc update_assert {} {

  global cov_rb prev_assert_index next_assert_index curr_assert_ptr

  # If the combinational window exists, update the GUI
  if {[winfo exists .assertwin] == 1} {

    # If the current metric mode is not assert, disable the prev/next buttons
    if {$cov_rb != "assert"} {

      .assertwin.bf.prev configure -state disabled
      .assertwin.bf.next configure -state disabled

    } else {

      # Restore the pointer if it is set
      set_pointer curr_assert_ptr $curr_assert_ptr

      # Restore the previous/next button enables
      if {$prev_assert_index != ""} {
        .assertwin.bf.prev configure -state normal
      } else {
        .assertwin.bf.prev configure -state disabled
      }
      if {$next_assert_index != ""} {
        .assertwin.bf.next configure -state normal
      } else {
        .assertwin.bf.next configure -state disabled
      }

    }

  }

}
