# Global variables
set fsm_states     [list 0000 0001 0010 0100 1000]
set fsm_hit_states [list 0000 0001 0010]
set fsm_arcs       [list [list 0000 0001] [list 0001 0000] [list 0000 0010] [list 0010 0100] [list 0100 0000]]
set fsm_hit_arcs   [list [list 0000 0001] [list 0001 0000] [list 0000 0010]]
set fsm_in_state   next_state
set fsm_out_state  state
set fsm_bheight    -1
set curr_fsm_ptr   ""

proc fsm_place {height value} {

  place .fsmwin.f.io     -height [expr $height - $value]
  place .fsmwin.f.handle -y [expr $height - $value]
  place .fsmwin.f.t      -height $value

}

proc create_fsm_window {expr_id} {

  global prev_fsm_index next_fsm_index

  # Now create the window and set the grab to this window
  if {[winfo exists .fsmwin] == 0} {

    # Create new window
    toplevel .fsmwin
    wm title .fsmwin "FSM State/State Transition Coverage - Verbose"

    # Create all frames for window
    frame .fsmwin.f -width 500 -height 350
    frame .fsmwin.f.io     -relief raised -borderwidth 1
    frame .fsmwin.f.t      -relief raised -borderwidth 1
    frame .fsmwin.f.handle -relief raised -borderwidth 2 -cursor sb_v_double_arrow

    # Create frame slider
    place .fsmwin.f.io     -relwidth 1 -height 1
    place .fsmwin.f.t      -relwidth 1 -rely 1 -anchor sw -height -1
    place .fsmwin.f.handle -relx 0.8 -anchor e -width 8 -height 8

    bind .fsmwin.f <Configure> {
      set H [winfo height .fsmwin.f]
      set Y0 [winfo rooty .fsmwin.f]
      if {$fsm_bheight == -1} {
        set fsm_bheight [expr .7 * $H]
      }
      fsm_place $H $fsm_bheight
    }

    bind .fsmwin.f.handle <B1-Motion> {
      set fsm_bheight [expr $H - [expr %Y - $Y0]]
      fsm_place $H $fsm_bheight
    }

    # Create input/output expression frame components
    label .fsmwin.f.io.l -anchor w -text "FSM Input/Output State Expressions"
    text .fsmwin.f.io.t -height 4 -width 60 -xscrollcommand ".fsmwin.f.io.hb set" \
                        -yscrollcommand ".fsmwin.f.io.vb set" -wrap none -state disabled
    scrollbar .fsmwin.f.io.hb -orient horizontal -command ".fsmwin.f.io.t xview"
    scrollbar .fsmwin.f.io.vb -command ".fsmwin.f.io.t yview"

    # Pack the input/output expression frame
    grid rowconfigure    .fsmwin.f.io 1 -weight 1
    grid columnconfigure .fsmwin.f.io 0 -weight 1
    grid .fsmwin.f.io.l   -row 0 -column 0 -sticky news
    grid .fsmwin.f.io.t   -row 1 -column 0 -sticky news
    grid .fsmwin.f.io.vb  -row 1 -column 1 -sticky news
    grid .fsmwin.f.io.hb  -row 2 -column 0 -sticky news

    # Create canvas frame components
    label .fsmwin.f.t.l -anchor w -text "FSM State and State Transition Table"
    canvas .fsmwin.f.t.c -relief sunken -borderwidth 2 \
           -xscrollcommand ".fsmwin.f.t.hb set" -yscrollcommand ".fsmwin.f.t.vb set"
    scrollbar .fsmwin.f.t.hb -orient horizontal -command ".fsmwin.f.t.c xview"
    scrollbar .fsmwin.f.t.vb -command ".fsmwin.f.t.c yview"

    # Create general information window
    label .fsmwin.info -anchor w -relief raised -borderwidth 1 -width 60

    # Pack the main frame widgets
    grid rowconfigure    .fsmwin.f.t 1 -weight 1
    grid columnconfigure .fsmwin.f.t 0 -weight 1
    grid .fsmwin.f.t.l  -row 0 -column 0 -sticky news
    grid .fsmwin.f.t.c  -row 1 -column 0 -sticky news
    grid .fsmwin.f.t.hb -row 2 -column 0 -sticky news
    grid .fsmwin.f.t.vb -row 1 -column 1 -sticky news

    # Create the button frame
    frame .fsmwin.bf -relief raised -borderwidth 1
    button .fsmwin.bf.prev -text "<--" -state disabled -command {
      display_fsm $prev_fsm_index
    }
    button .fsmwin.bf.next -text "-->" -state disabled -command {
      display_fsm $next_fsm_index
    }
    button .fsmwin.bf.close -text "Close" -width 10 -command {
      destroy .fsmwin
    }
    button .fsmwin.bf.help -text "Help" -width 10 -command {
      help_show_manual fsm
    }

    # Pack the buttons into the button frame
    pack .fsmwin.bf.prev  -side left  -padx 8 -pady 4
    pack .fsmwin.bf.next  -side left  -padx 8 -pady 4
    pack .fsmwin.bf.help  -side right -padx 8 -pady 4
    pack .fsmwin.bf.close -side right -padx 8 -pady 4

    # Pack frames into window
    pack .fsmwin.f    -fill both -expand yes
    pack .fsmwin.info -fill both
    pack .fsmwin.bf   -fill x

  }

  # Display the current FSM table
  display_fsm_window $expr_id

  # Raise this window to the foreground
  raise .fsmwin

}

proc display_fsm_state_exprs {} {

  global fsm_in_state fsm_out_state

  # Allow us to update the text widget
  .fsmwin.f.io.t configure -state normal

  # Clear the textbox
  .fsmwin.f.io.t delete 1.0 end

  # Populate the textbox
  .fsmwin.f.io.t insert end "Input  state expression:  $fsm_in_state\n\n"
  .fsmwin.f.io.t insert end "Output state expression:  $fsm_out_state\n"

  # Disable the textbox
  .fsmwin.f.io.t configure -state disabled

}

proc fsm_gen_vertical_text {t} {

  set new_t ""

  for {set i 0} {$i<[string length $t]} {incr i} {
    if {$i == 0} {
      set new_t [string index $t $i]
    } else {
      set new_t "$new_t\n[string index $t $i]"
    }
  }

  return $new_t

}

proc fsm_calc_xwidth {coord pad} {

  return [expr [expr [lindex $coord 2] - [lindex $coord 0]] + $pad]

}

proc fsm_calc_ywidth {coord pad} {

  return [expr [expr [lindex $coord 3] - [lindex $coord 1]] + $pad]

}

proc fsm_calc_state_fillcolor {idx} {

  global uncov_bgColor cov_bgColor
  global fsm_states fsm_hit_states

  if {[lsearch -exact $fsm_hit_states [lindex $fsm_states $idx]] == -1} {
    set fillcolor $uncov_bgColor
  } else {
    set fillcolor $cov_bgColor
  }

  return $fillcolor

}

proc fsm_calc_arc_fillcolor {in out dfltColor} {

  global uncov_bgColor cov_bgColor
  global fsm_arcs fsm_hit_arcs
  global fsm_states

  set found 0

  # First search the fsm_arcs list to see if the current in/out state is even a valid transition
  set i 0
  while {[expr $i < [llength $fsm_arcs]] && [expr $found == 0]} {

    set arc [lindex $fsm_arcs $i]

    if {[expr [string compare [lindex $arc 0] [lindex $fsm_states $in]] == 0] && \
        [expr [string compare [lindex $arc 1] [lindex $fsm_states $out]] == 0]} {
      set found 1
    }

    incr i

  }

  # If this wasn't even a valid state transition, set the fill color to the specified default color
  if {$found == 0} {

    set fillcolor $dfltColor

  } else {

    # Reset the value of found
    set found 0

    # Otherwise, check to see if it was covered or not
    set i 0
    while {[expr $i < [llength $fsm_hit_arcs]] && [expr $found == 0]} {

      set arc [lindex $fsm_hit_arcs $i]

      if {[expr [string compare [lindex $arc 0] [lindex $fsm_states $in]] == 0] && \
          [expr [string compare [lindex $arc 1] [lindex $fsm_states $out]] == 0]} {
        set found 1
      }

      incr i

    }

    if {$found == 0} {
      set fillcolor $uncov_bgColor
    } else {
      set fillcolor $cov_bgColor
    }

  }

  return $fillcolor

}

proc display_fsm_table {} {

  global fsm_hit_states fsm_states fsm_hit_arcs fsm_arcs
  global uncov_bgColor cov_bgColor

  # Initialize padding values
  set tpad 10
  set xpad 20
  set ypad 20

  # Calculate the width of a horizontal FSM state value
  .fsmwin.f.t.c create text 1c 1c -text [lindex $fsm_states 1] -anchor nw -tags htext
  set hstate_coords [.fsmwin.f.t.c bbox htext]
  .fsmwin.f.t.c delete htext

  # Calculate the width of a vertical FSM state value
  .fsmwin.f.t.c create text 1c 1c -text [fsm_gen_vertical_text [lindex $fsm_states 1]] -anchor nw -tags vtext
  set vstate_coords [.fsmwin.f.t.c bbox vtext]
  .fsmwin.f.t.c delete vtext

  # Initialize y coordinate
  set y $ypad

  # Calculate the number of the rows/columns
  set width [expr [llength $fsm_states] + 1]

  # Step through each row
  for {set row 0} {$row<$width} {incr row} {

    # Reset X coordinate to 0
    set x $xpad

    # Step through each column
    for {set col 0} {$col<$width} {incr col} {

      # Calculate text, xwidth, ywidth and fillcolor
      if {$row == 0} {
        if {$col == 0} {
          set t ""
          set xwidth [fsm_calc_xwidth $hstate_coords [expr $tpad * 2]]
          set ywidth [fsm_calc_ywidth $vstate_coords [expr $tpad * 2]]
          set fillcolor [.fsmwin.f.t.c cget -bg]
        } else {
          set t [fsm_gen_vertical_text [lindex $fsm_states [expr $col - 1]]]
          set xwidth [fsm_calc_xwidth $vstate_coords [expr $tpad * 2]]
          set ywidth [fsm_calc_ywidth $vstate_coords [expr $tpad * 2]]
          set fillcolor [fsm_calc_state_fillcolor [expr $col - 1]]
        }
      } elseif {$col == 0} {
        set t [lindex $fsm_states [expr $row - 1]]
        set xwidth [fsm_calc_xwidth $hstate_coords [expr $tpad * 2]]
        set ywidth [fsm_calc_ywidth $hstate_coords [expr $tpad * 2]]
        set fillcolor [fsm_calc_state_fillcolor [expr $row - 1]]
      } else {
        set t ""
        set xwidth [fsm_calc_xwidth $vstate_coords [expr $tpad * 2]]
        set ywidth [fsm_calc_ywidth $hstate_coords [expr $tpad * 2]]
        set fillcolor [fsm_calc_arc_fillcolor [expr $row - 1] [expr $col - 1] [.fsmwin.f.t.c cget -bg]]
      }

      # Create square
      .fsmwin.f.t.c create rect $x $y [expr $x + $xwidth] [expr $y + $ywidth] \
                              -outline black -fill $fillcolor -tags rect

      # Create text
      if {$t != ""} {
        .fsmwin.f.t.c create text [expr $x + $tpad] [expr $y + $tpad] -text $t -anchor nw -tags text
      }

      # If we are in row 0, column 0, draw the special output
      if {[expr $row == 0] && [expr $col == 0]} {
        .fsmwin.f.t.c create line $x $y [expr $x + $xwidth] [expr $y + $ywidth] -tags line
        .fsmwin.f.t.c create text [expr [expr $xwidth * 0.25] + $xpad] [expr [expr $ywidth * 0.75] + $ypad] \
                                -text "IN"  -anchor center -tags text
        .fsmwin.f.t.c create text [expr [expr $xwidth * 0.75] + $xpad] [expr [expr $ywidth * 0.25] + $ypad] \
                                -text "OUT" -anchor center -tags text
      }

      # Recalculate the x coordinates
      set x [expr $x + $xwidth]

    }

    # Recalculate the y coordinates
    set y [expr $y + $ywidth]

  }

  # Set the canvas scrollregion so that our rows/columns will fit
  .fsmwin.f.t.c configure -scrollregion "0 0 [expr $x + $xwidth] [expr $y + $ywidth]"

}

proc display_fsm_window {expr_id} {

  global file_name curr_funit_name curr_funit_type
  global fsm_curr_info
  global fsm_states fsm_hit_states
  global fsm_arcs fsm_hit_arcs
  global fsm_in_state fsm_out_state

  # Get information from design for this expression ID - TBD
  set fsm_states     ""
  set fsm_hit_states ""
  set fsm_arcs       ""
  set fsm_hit_arcs   ""
  set fsm_in_state   ""
  set fsm_out_state  ""
  tcl_func_get_fsm_coverage $curr_funit_name $curr_funit_type $expr_id

  # Display the state expressions
  display_fsm_state_exprs

  # Display the state transition table
  display_fsm_table

  # Display the information in the information bar
  set fsm_curr_info "Filename: $file_name, module: $curr_funit_name"
  .fsmwin.info configure -text $fsm_curr_info

}

# Main function to call that will create the FSM verbose viewer (if needed) and populate it
# with the currently selected uncovered FSM.
proc display_fsm {curr_index} {

  global prev_fsm_index next_fsm_index curr_fsm_ptr
  global uncovered_fsms start_line

  # Calculate expression ID
  set all_ranges [.bot.right.txt tag ranges uncov_button]
  set my_range   [.bot.right.txt tag prevrange uncov_button "$curr_index + 1 chars"]
  set index [expr [lsearch -exact $all_ranges [lindex $my_range 0]] / 2]
  set expr_id [lindex [lindex $uncovered_fsms $index] 2]

  # Get range of current signal
  set curr_range [.bot.right.txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Make sure that the selected signal is visible in the text box and is shown as selected
  set_pointer curr_fsm_ptr [lindex [split [lindex $my_range 0] .] 0]
  goto_uncov [lindex $my_range 0]

  # Get range of previous signal
  set prev_fsm_index [lindex [.bot.right.txt tag prevrange uncov_button [lindex $curr_index 0]] 0]

  # Get range of next signal
  set next_fsm_index [lindex [.bot.right.txt tag nextrange uncov_button [lindex $curr_range 1]] 0]

  # Now create the FSM window
  create_fsm_window $expr_id

}

# Updates the GUI elements of the FSM window when some type of event causes the
# current metric mode to change.
proc update_fsm {} {

  global cov_rb prev_fsm_index next_fsm_index curr_fsm_ptr

  # If the FSM window exists, update the GUI
  if {[winfo exists .fsmwin] == 1} {

    # If the current metric mode is not FSM, disable the prev/next buttons
    if {$cov_rb != "fsm"} {

      .fsmwin.bf.prev configure -state disabled
      .fsmwin.bf.next configure -state disabled

    } else {

      # Restore curr_pointer if it has been set
      set_pointer curr_fsm_ptr $curr_fsm_ptr

      # Restore previous/next button enables
      if {$prev_fsm_index != ""} {
        .fsmwin.bf.prev configure -state normal
      } else {
        .fsmwin.bf.prev configure -state disabled
      }
      if {$next_fsm_index != ""} {
        .fsmwin.bf.next configure -state normal
      } else {
        .fsmwin.bf.next configure -state disabled
      }

    }

  }

}

proc clear_fsm {} {

  global curr_fsm_ptr

  # Reset the variables
  set curr_fsm_ptr ""
  set fsm_bheight  -1

  # Destroy the window
  destroy .fsmwin

}
