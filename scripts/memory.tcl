set sig_name        ""
set curr_memory_ptr ""

proc display_memory {curr_index} {

  global prev_memory_index next_memory_index curr_memory_ptr

  # Get range of current signal
  set curr_range [.bot.right.txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Calculate the current signal string
  set curr_signal [string trim [lindex [split [.bot.right.txt get [lindex $curr_range 0] [lindex $curr_range 1]] "\["] 0]]

  # Make sure that the selected signal is visible in the text box and is shown as selected
  set_pointer curr_memory_ptr [lindex [split [lindex $curr_range 0] .] 0]
  goto_uncov [lindex $curr_range 0]

  # Get range of previous signal
  set prev_memory_index [lindex [.bot.right.txt tag prevrange uncov_button [lindex $curr_index 0]] 0]

  # Get range of next signal
  set next_memory_index [lindex [.bot.right.txt tag nextrange uncov_button [lindex $curr_range 1]] 0]

  # Now create the memory window
  create_memory_window $curr_signal

}

proc create_memory_window {signal} {

  global memory_udim memory_pdim memory_array
  global sig_name prev_memory_index next_memory_index
  global curr_funit_name curr_funit_type
  global curr_memory_ptr

  set sig_name       $signal

  # Now create the window and set the grab to this window
  if {[winfo exists .memwin] == 0} {

    # Create new window
    toplevel .memwin
    wm title .memwin "Memory Coverage - Verbose"

    # Create upper frame
    frame .memwin.f

    # Add addressable memory elements selector widgets
    frame .memwin.f.fae -relief raised -borderwidth 1
    label .memwin.f.fae.l -anchor w -text "Memory elements"
    listbox .memwin.f.fae.lb -height 10 -width 20 -yscrollcommand ".memwin.f.fae.vb set"
    scrollbar .memwin.f.fae.vb -command ".memwin.f.fae.lb"

    # Pack the addressable memory elements frame
    grid rowconfigure    .memwin.f.fae 1 -weight 1
    grid columnconfigure .memwin.f.fae 0 -weight 1
    grid columnconfigure .memwin.f.fae 1 -weight 0
    grid .memwin.f.fae.l  -row 0 -column 0 -sticky new
    grid .memwin.f.fae.lb -row 1 -column 0 -sticky news
    grid .memwin.f.fae.vb -row 1 -column 1 -sticky ns

    # Add memory toggle widgets
    frame .memwin.f.ft -relief raised -borderwidth 1
    label .memwin.f.ft.l -anchor w -text "Element Coverage"
    label .memwin.f.ft.l_01 -anchor w -text "Toggle 0->1"
    label .memwin.f.ft.l_10 -anchor w -text "Toggle 1->0"
    text  .memwin.f.ft.t -height 2 -width 40 -xscrollcommand ".memwin.f.ft.hb set" -wrap none -spacing1 2 -spacing3 3
    scrollbar .memwin.f.ft.hb -orient horizontal -command ".memwin.f.ft.t xview"

    # Create bottom information bar
    label .memwin.f.ft.info -anchor e

    # Add memory write/read widgets
    checkbutton .memwin.f.ft.cbw -anchor w -text "Written" -variable memory_curr_wr -state disabled
    checkbutton .memwin.f.ft.cbr -anchor w -text "Read"    -variable memory_curr_rd -state disabled

    # Create exclude checkbutton
    checkbutton .memwin.f.ft.excl -text "Exclude" -variable memory_excluded -command {
      tcl_func_set_memory_exclude $curr_funit_name $curr_funit_type $sig_name $memory_excluded
      set text_x [.bot.right.txt xview]
      set text_y [.bot.right.txt yview]
      process_funit_memory_cov
      .bot.right.txt xview moveto [lindex $text_x 0]
      .bot.right.txt yview moveto [lindex $text_y 0]
      update_summary
      enable_cdd_save
      set_pointer curr_memory_ptr $curr_memory_ptr
    }

    # Pack the toggle/write/read widgets
    grid rowconfigure    .memwin.f.ft 6 -weight 1
    grid columnconfigure .memwin.f.ft 0 -weight 1
    grid .memwin.f.ft.l    -row 0 -column 0 -sticky new
    grid .memwin.f.ft.t    -row 1 -rowspan 2 -column 0 -columnspan 2 -sticky new
    grid .memwin.f.ft.l_01 -row 1 -column 2 -sticky new
    grid .memwin.f.ft.l_10 -row 2 -column 2 -sticky new
    grid .memwin.f.ft.hb   -row 3 -column 0 -columnspan 2 -sticky new
    grid .memwin.f.ft.info -row 4 -column 0 -columnspan 2 -sticky new
    grid .memwin.f.ft.cbw  -row 5 -column 0 -sticky new
    grid .memwin.f.ft.cbr  -row 5 -column 1 -sticky new
    grid .memwin.f.ft.excl -row 6 -column 0 -sticky news

    # Create bottom button bar
    frame .memwin.bf -relief raised -borderwidth 1
    button .memwin.bf.close -text "Close" -width 10 -command {
      rm_pointer curr_memory_ptr
      destroy .memwin
    }
    button .memwin.bf.help -text "Help" -width 10 -command {
      help_show_manual memory
    }
    button .memwin.bf.prev -text "<--" -command {
      display_memory $prev_memory_index
    }
    button .memwin.bf.next -text "-->" -command {
      display_memory $next_memory_index
    }

    # Pack the buttons into the button frame
    pack .memwin.bf.prev  -side left  -padx 8 -pady 4
    pack .memwin.bf.next  -side left  -padx 8 -pady 4
    pack .memwin.bf.help  -side right -padx 8 -pady 4
    pack .memwin.bf.close -side right -padx 8 -pady 4

    # Pack the upper frames
    pack .memwin.f.fae -side left -fill both
    pack .memwin.f.ft  -side left -fill both -expand yes

    pack .memwin.f  -fill both -expand yes
    pack .memwin.bf -fill x

  }

  # Disable next or previous buttons if valid
  if {$prev_memory_index == ""} {
    .memwin.bf.prev configure -state disabled
  } else {
    .memwin.bf.prev configure -state normal
  }
  if {$next_memory_index == ""} {
    .memwin.bf.next configure -state disabled
  } else {
    .memwin.bf.next configure -state normal
  }

  # Get verbose memory information
  set memory_udim  0
  set memory_pdim  0
  set memory_array 0
  tcl_func_get_memory_coverage $curr_funit_name $curr_funit_type $signal

  # Allow us to clear out text box
  .memwin.f.ft.t configure -state normal

  # Remove any tags associated with the memory text box
  .memwin.f.ft.t tag delete mem_motion

  # Clear the text-box before any insertion is being made
  .memwin.f.ft.t delete 1.0 end

  # Write memory 0->1 and 1->0 information in text box
  # .memwin.f.ft.t insert end "$memory01_verbose\n$memory10_verbose" 

  # Keep user from writing in text boxes
  .memwin.f.ft.t configure -state disabled

  # Create leave bindings for textboxes
  bind .memwin.f.ft.t <Leave> {
    .memwin.f.ft.info configure -text "$sig_name\[$memory_msb:$memory_lsb\]"
  }

  # Add memory tags and bindings
  .memwin.f.ft.t tag add  mem_motion 1.0 {end - 1 chars}
  .memwin.f.ft.t tag bind mem_motion <Motion> {
    set tmp [lindex [split [.memwin.f.ft.t index current] "."] 1]
    set tmp [expr $memory_msb - $tmp]
    .memwin.f.ft.info configure -text "$sig_name\[$tmp\]"
  }

  # Right justify the memory information
  if {[expr [expr $memory_msb - $memory_lsb] + 1] <= 32} { 
    .memwin.f.ft.t tag configure mem_motion -justify right
  } else {
    .memwin.f.ft.t tag configure mem_motion -justify left
    .memwin.f.ft.t xview moveto 1.0
  }

  .memwin.f.ft.info configure -text "$sig_name\[$memory_msb:$memory_lsb\]"

  # Raise this window to the foreground
  raise .memwin

}

# Updates the GUI elements of the memory window when some type of event causes the
# current metric mode to change.
proc update_memory {} {

  global cov_rb prev_memory_index next_memory_index curr_memory_ptr

  # If the combinational window exists, update the GUI
  if {[winfo exists .memwin] == 1} {

    # If the current metric mode is not memory, disable the prev/next buttons
    if {$cov_rb != "memory"} {

      .memwin.bf.prev configure -state disabled
      .memwin.bf.next configure -state disabled

    } else {

      # Restore the pointer if it is set
      set_pointer curr_memory_ptr $curr_memory_ptr

      # Restore the previous/next button enables
      if {$prev_memory_index != ""} {
        .memwin.bf.prev configure -state normal
      } else {
        .memwin.bf.prev configure -state disabled
      }
      if {$next_memory_index != ""} {
        .memwin.bf.next configure -state normal
      } else {
        .memwin.bf.next configure -state disabled
      }

    }

  }

}

# This is called when the current CDD is closed.  It resets all associated variables
# and destroys the window (if it is currently opened).
proc clear_memory {} {

  global curr_memory_ptr

  # Clear the current memory pointer value
  set curr_memory_ptr ""

  # Destroy the window
  destroy .memwin

}
