# Global variables
set fsm_states     [list 0000 0001 0010 0100 1000]
set fsm_hit_states [list 0000 0001 0010]
set fsm_arcs       [list [list 0000 0001] [list 0001 0000] [list 0000 0010] [list 0010 0100] [list 0100 0000]]
set fsm_hit_arcs   [list [list 0000 0001] [list 0001 0000] [list 0000 0010]]
set fsm_in_state   next_state
set fsm_out_state  state
set fsm_bheight    -1
set curr_fsm_ptr   ""
set fsm_geometry   ""
set fsm_gui_saved  0

proc create_fsm_window {expr_id} {

  global prev_fsm_index next_fsm_index HOME
  global fsm_geometry fsm_gui_saved

  # Now create the window and set the grab to this window
  if {[winfo exists .fsmwin] == 0} {

    set fsm_gui_saved 0

    # Create new window
    toplevel .fsmwin
    wm title .fsmwin "FSM State/State Transition Coverage - Verbose"

    # Create all frames for window
    panedwindow .fsmwin.pw -width 500 -height 350 -sashwidth 4 -sashrelief raised -orient vertical
    frame .fsmwin.pw.io     -relief raised -borderwidth 1
    frame .fsmwin.pw.t      -relief raised -borderwidth 1

    # Add movable frames to panedwindow
    .fsmwin.pw add .fsmwin.pw.io
    .fsmwin.pw add .fsmwin.pw.t

    # Create input/output expression frame components
    label .fsmwin.pw.io.l -anchor w -text "FSM Input/Output State Expressions"
    text .fsmwin.pw.io.t -height 4 -width 60 -xscrollcommand ".fsmwin.pw.io.hb set" \
                        -yscrollcommand ".fsmwin.pw.io.vb set" -wrap none -state disabled
    scrollbar .fsmwin.pw.io.hb -orient horizontal -command ".fsmwin.pw.io.t xview"
    scrollbar .fsmwin.pw.io.vb -command ".fsmwin.pw.io.t yview"

    # Pack the input/output expression frame
    grid rowconfigure    .fsmwin.pw.io 1 -weight 1
    grid columnconfigure .fsmwin.pw.io 0 -weight 1
    grid .fsmwin.pw.io.l   -row 0 -column 0 -sticky news
    grid .fsmwin.pw.io.t   -row 1 -column 0 -sticky news
    grid .fsmwin.pw.io.vb  -row 1 -column 1 -sticky news
    grid .fsmwin.pw.io.hb  -row 2 -column 0 -sticky news

    # Create canvas frame components
    label .fsmwin.pw.t.l -anchor w -text "FSM State and State Transition Table"
    canvas .fsmwin.pw.t.c -relief sunken -borderwidth 2 \
           -xscrollcommand ".fsmwin.pw.t.hb set" -yscrollcommand ".fsmwin.pw.t.vb set"
    scrollbar .fsmwin.pw.t.hb -orient horizontal -command ".fsmwin.pw.t.c xview"
    scrollbar .fsmwin.pw.t.vb -command ".fsmwin.pw.t.c yview"

    # Create general information window
    label .fsmwin.info -anchor w -relief raised -borderwidth 1 -width 60

    # Pack the main frame widgets
    grid rowconfigure    .fsmwin.pw.t 1 -weight 1
    grid columnconfigure .fsmwin.pw.t 0 -weight 1
    grid .fsmwin.pw.t.l  -row 0 -column 0 -sticky news
    grid .fsmwin.pw.t.c  -row 1 -column 0 -sticky news
    grid .fsmwin.pw.t.hb -row 2 -column 0 -sticky news
    grid .fsmwin.pw.t.vb -row 1 -column 1 -sticky news

    # Create the button frame
    frame .fsmwin.bf -relief raised -borderwidth 1
    button .fsmwin.bf.prev -image [image create photo -file [file join $HOME scripts left_arrow.gif]] -relief flat -state disabled -command {
      display_fsm $prev_fsm_index
    }
    set_balloon .fsmwin.bf.prev "Click to view the previous uncovered FSM in this window"
    button .fsmwin.bf.next -image [image create photo -file [file join $HOME scripts right_arrow.gif]] -relief flat -state disabled -command {
      display_fsm $next_fsm_index
    }
    set_balloon .fsmwin.bf.prev "Click to view the next uncovered FSM in this window"
    button .fsmwin.bf.close -text "Close" -width 10 -command {
      destroy .fsmwin
    }
    help_button .fsmwin.bf.help chapter.gui.fsm

    # Pack the buttons into the button frame
    pack .fsmwin.bf.prev  -side left
    pack .fsmwin.bf.next  -side left
    pack .fsmwin.bf.help  -side right -padx 8 -pady 4
    pack .fsmwin.bf.close -side right -padx 8 -pady 4

    # Pack frames into window
    pack .fsmwin.pw   -fill both -expand yes
    pack .fsmwin.info -fill both
    pack .fsmwin.bf   -fill x

    # Set window geometry, if specified
    if {$fsm_geometry != ""} {
      wm geometry .fsmwin $fsm_geometry
    }

    # Handle the destruction of this window
    wm protocol .fsmwin WM_DELETE_WINDOW {
      save_fsm_gui_elements 0
      destroy .fsmwin
    }
    bind .fsmwin <Destroy> {
      save_fsm_gui_elements 0
    }

  }

  # Display the current FSM table
  display_fsm_window $expr_id

  # Raise this window to the foreground
  raise .fsmwin

}

proc display_fsm_state_exprs {} {

  global fsm_in_state fsm_out_state

  # Allow us to update the text widget
  .fsmwin.pw.io.t configure -state normal

  # Clear the textbox
  .fsmwin.pw.io.t delete 1.0 end

  # Populate the textbox
  .fsmwin.pw.io.t insert end "Input  state expression:  $fsm_in_state\n\n"
  .fsmwin.pw.io.t insert end "Output state expression:  $fsm_out_state\n"

  # Disable the textbox
  .fsmwin.pw.io.t configure -state disabled

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

proc fsm_calc_arc_coverage {in out} {

  global fsm_arcs fsm_hit_arcs fsm_states

  set found    0
  set coverage -1

  # First search the fsm_arcs list to see if the current in/out state is even a valid transition
  set i 0
  while {[expr $i < [llength $fsm_arcs]] && [expr $found == 0]} {

    set arc [lindex $fsm_arcs $i]

    if {[expr [string compare [lindex $arc 0] [lindex $fsm_states $in]] == 0] && \
        [expr [string compare [lindex $arc 1] [lindex $fsm_states $out]] == 0]} {
      if {[lindex $arc 2] == 1} {
        set coverage 3
      }
      set found 1
    }

    incr i

  }

  # If this wasn't even a valid state transition, set the fill color to the specified default color
  if {$found == 0} {

    set coverage 0

  } else {

    # Reset the value of found
    set found 0

    # Otherwise, check to see if it was covered or not
    set i 0
    while {[expr $i < [llength $fsm_hit_arcs]] && [expr $found == 0]} {

      set arc [lindex $fsm_hit_arcs $i]

      if {[expr [string compare [lindex $arc 0] [lindex $fsm_states $in]] == 0]  && \
          [expr [string compare [lindex $arc 1] [lindex $fsm_states $out]] == 0]} {
        set found 1
      }

      incr i

    }

    if {$found == 0} {
      if {$coverage == -1} {
        set coverage 2
      }
    } else {
      set coverage 1
    }

  }

  return $coverage

}

proc fsm_calc_arc_fillcolor {in out dfltColor} {

  global uncov_bgColor cov_bgColor

  set coverage [fsm_calc_arc_coverage $in $out]

  if {$coverage == 0} {
    set fillcolor $dfltColor
  } elseif {$coverage == 2} {
    set fillcolor $uncov_bgColor
  } else {
    set fillcolor $cov_bgColor
  }

  return $fillcolor

}

proc display_fsm_table {} {

  global fsm_hit_states fsm_states fsm_hit_arcs fsm_arcs
  global uncov_bgColor cov_bgColor
  global curr_fsm_expr_id

  # Initialize padding values
  set tpad 10
  set xpad 20
  set ypad 20

  # Calculate the width of a horizontal FSM state value
  .fsmwin.pw.t.c create text 1c 1c -text [lindex $fsm_states 1] -anchor nw -tags htext
  set hstate_coords [.fsmwin.pw.t.c bbox htext]
  .fsmwin.pw.t.c delete htext

  # Calculate the width of a vertical FSM state value
  .fsmwin.pw.t.c create text 1c 1c -text [fsm_gen_vertical_text [lindex $fsm_states 1]] -anchor nw -tags vtext
  set vstate_coords [.fsmwin.pw.t.c bbox vtext]
  .fsmwin.pw.t.c delete vtext

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
          set fillcolor [.fsmwin.pw.t.c cget -bg]
          set tagname "title"
          set uline 0
        } else {
          set t [fsm_gen_vertical_text [lindex $fsm_states [expr $col - 1]]]
          set xwidth [fsm_calc_xwidth $vstate_coords [expr $tpad * 2]]
          set ywidth [fsm_calc_ywidth $vstate_coords [expr $tpad * 2]]
          set fillcolor [fsm_calc_state_fillcolor [expr $col - 1]]
          set tagname "to_state"
          set uline 0
        }
      } elseif {$col == 0} {
        set t [lindex $fsm_states [expr $row - 1]]
        set xwidth [fsm_calc_xwidth $hstate_coords [expr $tpad * 2]]
        set ywidth [fsm_calc_ywidth $hstate_coords [expr $tpad * 2]]
        set fillcolor [fsm_calc_state_fillcolor [expr $row - 1]]
        set tagname "from_state"
        set uline 0
      } else {
        set coverage [fsm_calc_arc_coverage [expr $row - 1] [expr $col - 1]]
        if {$coverage == 2} {
          set tagname "uncov_arc"
          set t       "I"
          set uline   1
        } elseif {$coverage == 3} {
          set tagname "uncov_arc"
          set t       "E"
          set uline   1
        } else {
          set tagname "arc"
          set t       ""
          set uline   0
        }
        set xwidth [fsm_calc_xwidth $vstate_coords [expr $tpad * 2]]
        set ywidth [fsm_calc_ywidth $hstate_coords [expr $tpad * 2]]
        set fillcolor [fsm_calc_arc_fillcolor [expr $row - 1] [expr $col - 1] [.fsmwin.pw.t.c cget -bg]]
      }

      # Create square
      .fsmwin.pw.t.c create rect $x $y [expr $x + $xwidth] [expr $y + $ywidth] \
                                -outline black -fill $fillcolor -tags rect

      # Create text
      if {$t != ""} {
        set tid [.fsmwin.pw.t.c create text [expr $x + $tpad] [expr $y + $tpad] -text $t -anchor nw -tags $tagname]
        if {$uline == 1} {
          set font_str "Courier [font actual [.fsmwin.pw.t.c itemcget $tid -font] -size] underline"
          .fsmwin.pw.t.c itemconfigure $tid -font $font_str
        }
      }

      # Bind each square
      .fsmwin.pw.t.c bind uncov_arc <Enter> {
        set curr_cursor [.fsmwin.pw.t.c cget -cursor]
        set curr_info   [.fsmwin.info cget -text]
        .fsmwin.pw.t.c configure -cursor hand2
        .fsmwin.info configure -text "Click the left button to exclude/include state transition"
      }
      .fsmwin.pw.t.c bind uncov_arc <Leave> {
        .fsmwin.pw.t.c configure -cursor $curr_cursor
        .fsmwin.info configure -text $curr_info
      }
      .fsmwin.pw.t.c bind uncov_arc <Button-1> {
        set coord [.fsmwin.pw.t.c coords [.fsmwin.pw.t.c find withtag current]]
        foreach rid [.fsmwin.pw.t.c find overlapping [lindex $coord 0] [lindex $coord 1] [lindex $coord 0] [lindex $coord 1]] {
          if {[.fsmwin.pw.t.c type $rid] == "rectangle"} {
            break
          }
        }
        set fsl [.fsmwin.pw.t.c find withtag from_state]
        for {set i 0} {$i < [llength $fsl]} {incr i} {
          set fcoord [.fsmwin.pw.t.c coords [lindex $fsl $i]]
          if {[lindex $coord 1] == [lindex $fcoord 1]} {
            set from_st [lindex $fsm_states $i]
            break
          }
        }
        set tsl [.fsmwin.pw.t.c find withtag to_state]
        for {set i 0} {$i < [llength $tsl]} {incr i} {
          set tcoord [.fsmwin.pw.t.c coords [lindex $tsl $i]]
          if {[lindex $coord 0] == [lindex $tcoord 0]} {
            set to_st [lindex $fsm_states $i]
            break
          }
        }
        if {[.fsmwin.pw.t.c itemcget current -text] == "E"} {
          set exclude 0
          .fsmwin.pw.t.c itemconfigure $rid -fill $uncov_bgColor
          .fsmwin.pw.t.c itemconfigure current -text "I"
        } else {
          set exclude 1
          .fsmwin.pw.t.c itemconfigure $rid -fill $cov_bgColor
          .fsmwin.pw.t.c itemconfigure current -text "E"
        }
        tcl_func_set_fsm_exclude $curr_block $curr_fsm_expr_id $from_st $to_st $exclude
        set text_x [.bot.right.txt xview]
        set text_y [.bot.right.txt yview]
        process_fsm_cov
        .bot.right.txt xview moveto [lindex $text_x 0]
        .bot.right.txt yview moveto [lindex $text_y 0]
        populate_listbox
        enable_cdd_save
        set_pointer curr_fsm_ptr $curr_fsm_ptr
      }

      # If we are in row 0, column 0, draw the special output
      if {[expr $row == 0] && [expr $col == 0]} {
        .fsmwin.pw.t.c create line $x $y [expr $x + $xwidth] [expr $y + $ywidth] -tags line
        .fsmwin.pw.t.c create text [expr [expr $xwidth * 0.25] + $xpad] [expr [expr $ywidth * 0.75] + $ypad] \
                                -text "IN"  -anchor center -tags text
        .fsmwin.pw.t.c create text [expr [expr $xwidth * 0.75] + $xpad] [expr [expr $ywidth * 0.25] + $ypad] \
                                -text "OUT" -anchor center -tags text
      }

      # Recalculate the x coordinates
      set x [expr $x + $xwidth]

    }

    # Recalculate the y coordinates
    set y [expr $y + $ywidth]

  }

  # Set the canvas scrollregion so that our rows/columns will fit
  .fsmwin.pw.t.c configure -scrollregion "0 0 [expr $x + $xwidth] [expr $y + $ywidth]"

}

proc display_fsm_window {expr_id} {

  global curr_block
  global fsm_curr_info
  global fsm_states fsm_hit_states
  global fsm_arcs fsm_hit_arcs
  global fsm_in_state fsm_out_state
  global curr_fsm_expr_id

  # Get information from design for this expression ID
  set fsm_states     ""
  set fsm_hit_states ""
  set fsm_arcs       ""
  set fsm_hit_arcs   ""
  set fsm_in_state   ""
  set fsm_out_state  ""
  tcl_func_get_fsm_coverage $curr_block $expr_id

  # Display the state expressions
  display_fsm_state_exprs

  # Initialize current expression ID
  set curr_fsm_expr_id $expr_id

  # Display the state transition table
  display_fsm_table

  # Display the information in the information bar
  set fsm_curr_info "Filename: [tcl_func_get_filename $curr_block], module: [tcl_func_get_funit_name $curr_block]"
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
    if {$cov_rb != "FSM"} {

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

# Saves the GUI elements from the FSM window setup that should be saved
proc save_fsm_gui_elements {main_exit} {

  global fsm_geometry fsm_gui_saved

  if {$fsm_gui_saved == 0} {
    if {$main_exit == 0 || [winfo exists .fsmwin] == 1} {
      set fsm_gui_saved 1
      set fsm_geometry  [winfo geometry .fsmwin]
    }
  }

}

