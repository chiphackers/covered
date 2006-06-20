set cov_rb      line
set last_cov_rb line

proc cov_create {f} {

  global cov_rb file_name start_line end_line last_cov_rb

  # Create frame for the radio buttons
  frame $f.m -relief raised -borderwidth 1

  radiobutton $f.m.line -variable cov_rb -value line -text "Line" -command { 
    if {$file_name != ""} {
      set text_x [.bot.right.txt xview]
      set text_y [.bot.right.txt yview]
      if {$last_cov_rb != $cov_rb} {
        set last_cov_rb $cov_rb
        highlight_listbox
        process_funit_line_cov
        update_all_windows
      } else {
        display_line_cov
      }
      .bot.right.txt xview moveto [lindex $text_x 0]
      .bot.right.txt yview moveto [lindex $text_y 0]
    } 
  }
  radiobutton $f.m.tog  -variable cov_rb -value toggle -text "Toggle" -command {
    if {$file_name != ""} {
      set text_x [.bot.right.txt xview]
      set text_y [.bot.right.txt yview]
      if {$last_cov_rb != $cov_rb} {
        set last_cov_rb $cov_rb
        highlight_listbox
        process_funit_toggle_cov
        update_all_windows
      } else {
        display_toggle_cov
      }
      .bot.right.txt xview moveto [lindex $text_x 0]
      .bot.right.txt yview moveto [lindex $text_y 0]
    }
  }
  radiobutton $f.m.comb -variable cov_rb -value comb -text "Logic" -command {
    if {$file_name != ""} {
      set text_x [.bot.right.txt xview]
      set text_y [.bot.right.txt yview]
      if {$last_cov_rb != $cov_rb} {
        set last_cov_rb $cov_rb
        highlight_listbox
        process_funit_comb_cov
        update_all_windows
      } else {
        display_comb_cov
      }
      .bot.right.txt xview moveto [lindex $text_x 0]
      .bot.right.txt yview moveto [lindex $text_y 0]
    }
  }
  radiobutton $f.m.fsm  -variable cov_rb -value fsm -text "FSM" -command {
    if {$file_name != ""} {
      set text_x [.bot.right.txt xview]
      set text_y [.bot.right.txt yview]
      if {$last_cov_rb != $cov_rb} {
        set last_cov_rb $cov_rb
        highlight_listbox
        process_funit_fsm_cov
        update_all_windows
      } else {
        display_fsm_cov
      }
      .bot.right.txt xview moveto [lindex $text_x 0]
      .bot.right.txt yview moveto [lindex $text_y 0]
    }
  }
  radiobutton $f.m.assert -variable cov_rb -value assert -text "Assert" -command {
    if {$file_name != ""} {
      set text_x [.bot.right.txt xview]
      set text_y [.bot.right.txt yview]
      if {$last_cov_rb != $cov_rb} {
        set last_cov_rb $cov_rb
        highlight_listbox
        process_funit_assert_cov
        update_all_windows
      } else {
        display_assert_cov
      }
      .bot.right.txt xview moveto [lindex $text_x 0]
      .bot.right.txt yview moveto [lindex $text_y 0]
    }
  }

  # Cause line coverage to be the default
  $f.m.line select

  # Pack radiobuttons
  pack $f.m.line   -side left
  pack $f.m.tog    -side left
  pack $f.m.comb   -side left
  pack $f.m.fsm    -side left
  pack $f.m.assert -side left

  # Create summary frame and widgets
  frame $f.s -relief raised -borderwidth 1
  label $f.s.l -text "Summary Information:"
  label $f.s.ht -width 45 -anchor w

  # Pack the summary frame
  pack $f.s.l  -side left
  pack $f.s.ht -side left -fill both

  # Pack the coverage box frame
  pack $f.m -side left -fill both
  pack $f.s -side left -fill both -expand yes

  # Pack the metric selection and summary frames into the current window
  pack $f -side top -fill both

}

proc cov_display_summary {hit total} {

  global cov_rb

  # Create summary information text
  if {$cov_rb == "line"} {
    set info "$hit out of $total lines executed"
  } elseif {$cov_rb == "toggle"} {
    set info "$hit out of $total signals fully toggled"
  } elseif {$cov_rb == "comb"} {
    set info "$hit out of $total logical combinations hit"
  } elseif {$cov_rb == "fsm"} {
    if {$total == -1} {
      set info "$hit FSM state transitions hit"
    } else {
      set info "$hit out of $total FSM state transitions hit"
    }
  } elseif {$cov_rb == "assert"} {
    set info "$hit out of $total assertion coverage points hit"
  } else {
  }

  # Display text to GUI
  .covbox.s.ht configure -text $info

}

proc cov_clear_summary {} {

  .covbox.s.ht configure -text ""

}
