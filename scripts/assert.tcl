set curr_assert_ptr ""

proc assert_yset {args} {

  eval [linsert $args 0 .assertwin.f.vb set]
  assert_yview moveto [lindex [.assertwin.f.vb get] 0]

}

proc assert_yview {args} {

  eval [linsert $args 0 .assertwin.f.td yview]
  eval [linsert $args 0 .assertwin.f.tc yview]

}

proc display_assert {curr_index} {

  global prev_assert_index next_assert_index curr_assert_ptr curr_assert_inst

  # Get range of current instance
  set curr_range [.bot.right.txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Calculate the current instance string
  set curr_assert_inst [string trim [lindex [split [.bot.right.txt get [lindex $curr_range 0] [lindex $curr_range 1]] "\["] 0]]

  # Make sure that the selected instance is visible in the text box and is shown as selected
  set_pointer curr_assert_ptr [lindex [split [lindex $curr_range 0] .] 0]
  goto_uncov [lindex $curr_range 0]

  # Get range of previous instance
  set prev_assert_index [lindex [.bot.right.txt tag prevrange uncov_button [lindex $curr_range 0]] 0]

  # Get range of next instance
  set next_assert_index [lindex [.bot.right.txt tag nextrange uncov_button [lindex $curr_range 1]] 0]

  # Now create the assertion window
  create_assert_window $curr_assert_inst

}

proc create_assert_window {inst} {

  global prev_assert_index next_assert_index
  global curr_funit_name curr_funit_type
  global curr_assert_ptr assert_cov_points assert_cov_mod
  global uncov_bgColor uncov_fgColor
  global cov_bgColor cov_fgColor

  # Now create the window and set the grab to this window
  if {[winfo exists .assertwin] == 0} {

    # Create new window
    toplevel .assertwin
    wm title .assertwin "Assertion Coverage - Verbose"

    frame .assertwin.f -relief raised -borderwidth 1

    # Add toggle information
    label .assertwin.f.ld -anchor w -text "Coverage point description"
    label .assertwin.f.lc -anchor w -text "# of hits"
    text  .assertwin.f.td -height 5 -width 40 -borderwidth 0 -xscrollcommand ".assertwin.f.hb set" \
          -yscrollcommand assert_yset -wrap none -spacing1 2 -spacing3 3
    text  .assertwin.f.tc -height 5 -width 10 -borderwidth 0 -yscrollcommand assert_yset -wrap none -spacing1 2 -spacing3 3
    scrollbar .assertwin.f.hb -orient horizontal -command ".assertwin.f.td xview"
    scrollbar .assertwin.f.vb -command assert_yview
    button .assertwin.f.b -text "Show Code" -command {
      display_assertion_module $assert_cov_mod
    }

    # Create bottom information bar
    label .assertwin.f.info -anchor w

    # Create bottom button bar
    frame .assertwin.bf -relief raised -borderwidth 1
    button .assertwin.bf.close -text "Close" -width 10 -command {
      rm_pointer curr_assert_ptr
      destroy .assertwin
      destroy .amodwin
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
    grid .assertwin.f.ld   -row 0 -column 0 -sticky nsew
    grid .assertwin.f.lc   -row 0 -column 1 -sticky nsew
    grid .assertwin.f.td   -row 1 -column 0 -sticky nsew
    grid .assertwin.f.tc   -row 1 -column 1 -sticky nsew
    grid .assertwin.f.vb   -row 1 -column 2 -sticky nsew
    grid .assertwin.f.hb   -row 2 -column 0 -sticky nsew
    grid .assertwin.f.b    -row 2 -column 1 -sticky nsew
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
  set assert_cov_mod    ""
  set assert_cov_points ""
  tcl_func_get_assert_coverage $curr_funit_name $curr_funit_type $inst

  # Allow us to clear out text boxes
  .assertwin.f.td configure -state normal
  .assertwin.f.tc configure -state normal

  # Clear the text boxes before any insertion is being made
  .assertwin.f.td delete 1.0 end
  .assertwin.f.tc delete 1.0 end

  # Delete any existing tags
  .assertwin.f.td tag delete td_uncov_colorMap td_cov_colorMap
  .assertwin.f.tc tag delete tc_uncov_colorMap tc_cov_colorMap tc_excl_colorMap

  # Create covered/uncovered/underline tags
  .assertwin.f.td tag configure td_uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
  .assertwin.f.td tag configure td_cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor
  .assertwin.f.tc tag configure tc_uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
  .assertwin.f.tc tag configure tc_cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor
  .assertwin.f.tc tag configure tc_uncov_uline    -underline true

  # Write assertion coverage point information in text boxes
  foreach cov_point $assert_cov_points {
    if {[lindex $cov_point 1] == 0} {
      if {[lindex $cov_point 3] == 1} {
        .assertwin.f.td insert end "[lindex $cov_point 0]\n" td_cov_colorMap
        .assertwin.f.tc insert end "E\n" {tc_cov_colorMap tc_uncov_uline}
      } else {
        .assertwin.f.td insert end "[lindex $cov_point 0]\n" td_uncov_colorMap
        .assertwin.f.tc insert end "[lindex $cov_point 1]\n" {tc_uncov_colorMap tc_uncov_uline}
      }
    } else {
      .assertwin.f.td insert end "[lindex $cov_point 0]\n" td_cov_colorMap
      .assertwin.f.tc insert end "[lindex $cov_point 1]\n" tc_cov_colorMap
    }
  }

  # Bind uncovered coverage points
  .assertwin.f.tc tag bind tc_uncov_uline <Enter> {
    set curr_cursor [.assertwin.f.tc cget -cursor]
    set curr_info   [.assertwin.f.info cget -text]
    .assertwin.f.tc   configure -cursor hand2
    .assertwin.f.info configure -text "Click the left button to exclude/include coverage point"
  }
  .assertwin.f.tc tag bind tc_uncov_uline <Leave> {
    .assertwin.f.tc   configure -cursor $curr_cursor
    .assertwin.f.info configure -text $curr_info
  }
  .assertwin.f.tc tag bind tc_uncov_uline <Button-1> {
    set curr_index [expr [lindex [split [.assertwin.f.tc index current] .] 0] - 1]
    set curr_exp   [lindex [lindex $assert_cov_points $curr_index] 2]
    if {[lindex [lindex $assert_cov_points $curr_index] 3] == 0} {
      set curr_excl 1
    } else {
      set curr_excl 0
    }
    tcl_func_set_assert_exclude $curr_funit_name $curr_funit_type $curr_assert_inst $curr_exp $curr_excl
    set text_x [.bot.right.txt xview]
    set text_y [.bot.right.txt yview]
    process_funit_assert_cov
    .bot.right.txt xview moveto [lindex $text_x 0]
    .bot.right.txt yview moveto [lindex $text_y 0]
    update_summary
    enable_cdd_save
    set_pointer curr_assert_ptr $curr_assert_ptr
    create_assert_window $curr_assert_inst
  }

  # Keep user from writing in text boxes
  .assertwin.f.td configure -state disabled
  .assertwin.f.tc configure -state disabled
  
  # Update informational bar
  .assertwin.f.info configure -text "Assertion module: $assert_cov_mod"

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

proc clear_assert {} {

  global curr_assert_ptr

  # Reset the variables
  set curr_assert_ptr ""

  # Destroy the window
  destroy .assertwin

}

proc display_assertion_module {mod} {

  global open_files file_name

  # Store original filename and working directory
  set tmp_name $file_name

  # Get filename
  tcl_func_get_filename $mod 0

  if {[winfo exists .amodwin] == 0} {

    set open_files ""

    # Create the window viewer
    toplevel .amodwin
    wm title .amodwin "Assertion Module - $mod"

    # Create text viewer windows
    frame .amodwin.f -relief raised -borderwidth 1
    text .amodwin.f.t -height 30 -width 80 -xscrollcommand ".amodwin.f.hb set" -yscrollcommand ".amodwin.f.vb set" -state disabled
    scrollbar .amodwin.f.hb -orient horizontal -command ".amodwin.f.t xview"
    scrollbar .amodwin.f.vb -command ".amodwin.f.t yview"
    label .amodwin.f.info -anchor w

    # Layout the text viewer windows
    grid rowconfigure    .amodwin.f 0 -weight 1
    grid columnconfigure .amodwin.f 0 -weight 1
    grid .amodwin.f.t    -row 0 -column 0 -sticky news
    grid .amodwin.f.vb   -row 0 -column 1 -sticky news
    grid .amodwin.f.hb   -row 1 -column 0 -sticky news
    grid .amodwin.f.info -row 2 -column 0 -columnspan 2 -sticky we

    # Create the button bar
    frame .amodwin.bf -relief raised -borderwidth 1
    button .amodwin.bf.back -text "Back" -width 10 -command {
      set fname [lindex $open_files 1]
      set open_files [lreplace $open_files 0 1]
      populate_assertion_text $fname
    } -state disabled
    button .amodwin.bf.close -text "Close" -width 10 -command {
      destroy .amodwin
    }
    button .amodwin.bf.help -text "Help" -width 10 -command {
      help_show_manual src_view
    }

    # Pack the button bar
    pack .amodwin.bf.back  -side left  -padx 8 -pady 4
    pack .amodwin.bf.help  -side right -padx 8 -pady 4
    pack .amodwin.bf.close -side right -padx 8 -pady 4

    # Pack the frames
    pack .amodwin.f -fill both -expand yes
    pack .amodwin.bf -fill x
    
  }

  # Write the module information
  populate_assertion_text $file_name

  # Restore global file_name
  set file_name $tmp_name

  # Raise this window to the foreground
  raise .amodwin

}

proc populate_assertion_text {fname} {

  global fileContent open_files

  # Add the specified file name to the open_files array
  set open_files [linsert $open_files 0 $fname]

  # Set the state of the back button accordingly
  if {[llength $open_files] < 2} {
    .amodwin.bf.back configure -state disabled
  } else {
    .amodwin.bf.back configure -state normal
  }

  # Read in the specified filename
  load_verilog $fname 0

  # Allow us to write to the text box
  .amodwin.f.t configure -state normal

  # Clear the text-box before any insertion is being made
  .amodwin.f.t delete 1.0 end

  set contents [split $fileContent($fname) \n]

  # Populate text box with file contents
  foreach phrase $contents {
    .amodwin.f.t insert end [append phrase "\n"]
  }

  # Perform highlighting
  verilog_highlight .amodwin.f.t

  # Take care of any included files
  handle_verilog_includes .amodwin.f.t .amodwin.f.info populate_assertion_text

  # Set state back to disabled for read-only access
  .amodwin.f.t configure -state disabled

}
