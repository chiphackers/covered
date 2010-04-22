################################################################################################
# Copyright (c) 2006-2010 Trevor Williams                                                      #
#                                                                                              #
# This program is free software; you can redistribute it and/or modify                         #
# it under the terms of the GNU General Public License as published by the Free Software       #
# Foundation; either version 2 of the License, or (at your option) any later version.          #
#                                                                                              #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;    #
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    #
# See the GNU General Public License for more details.                                         #
#                                                                                              #
# You should have received a copy of the GNU General Public License along with this program;   #
# if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. #
################################################################################################

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
set fsm_excluded   0
set fsm_reason     ""

proc fsm_find_arc_index {row col} {

  global fsm_arcs fsm_in_states fsm_out_states

  set out_state [lindex $fsm_out_states $col]
  set in_state  [lindex $fsm_in_states  $row]

  # Find the FSM state transition for this
  for {set i 0} {$i < [llength $fsm_arcs]} {incr i} {
    set arc [lindex $fsm_arcs $i]
    if {$in_state == [lindex $arc 0] && $out_state == [lindex $arc 1]} {
       break
    }
  }

  # Return found index value or -1 if not found
  if {$i < [llength $fsm_arcs]} {
    return $i
  } else {
    return -1
  }

}

proc fsm_arc_show_tooltip {w row col} {

  global fsm_arcs fsm_in_states fsm_out_states
  global preferences

  # Get arc index
  set arc_index [fsm_find_arc_index $row [expr $col - 1]]

  # Display balloon
  if {$arc_index != -1} {
    set arc [lindex $fsm_arcs $arc_index]
    if {[lindex $arc 2] == 1} {
      if {[lindex $arc 3] != ""} {
        balloon::show $w "State transition: [lindex $arc 0]->[lindex $arc 1]\n\nExclude Reason: [lindex $arc 3]\n\nClick to include this state
transition" \
          $preferences(cov_bgColor) $preferences(cov_fgColor)
      } else {
        balloon::show $w "State transition: [lindex $arc 0]->[lindex $arc 1]\n\nClick to include this state transition"
      }
    } elseif {[$w cellcget $row,$col -text] != ""} {
      balloon::show $w "State transition: [lindex $arc 0]->[lindex $arc 1]\n\nClick to exclude this state transition"
    } else {
      balloon::show $w "State transition: [lindex $arc 0]->[lindex $arc 1]"
    }
  }

}

proc fsm_arc_hide_tooltip {w} {

  balloon::hide $w

}

proc fsm_tablelist_selected {} {

  global fsm_arcs fsm_reason fsm_exclude
  global preferences metric_src
  global curr_block curr_fsm_expr_id curr_fsm_ptr

  set selected_cell [split [.fsmwin.pw.t.tl curcellselection] ,]
  set arc_index     [fsm_find_arc_index [lindex $selected_cell 0] [expr [lindex $selected_cell 1] - 1]]

  # If the arc index exists, perform exclusion
  if {$arc_index != -1} {
    set arc [lindex $fsm_arcs $arc_index]
    if {[.fsmwin.pw.t.tl cellcget [.fsmwin.pw.t.tl curcellselection] -text] != ""} {
      fsm_arc_hide_tooltip .fsmwin.pw.t.tl
      set fsm_reason   ""
      set fsm_excluded [expr ! [lindex $arc 2]]
      if {$preferences(exclude_reasons_enabled) == 1 && $fsm_excluded == 1} {
        set fsm_reason [get_exclude_reason .fsmwin]
      }
      set arc      [lreplace $arc 2 3 $fsm_excluded $fsm_reason]
      set fsm_arcs [lreplace $fsm_arcs $arc_index $arc_index $arc]
      tcl_func_set_fsm_exclude $curr_block $curr_fsm_expr_id [lindex $arc 0] [lindex $arc 1] $fsm_excluded $fsm_reason
      display_fsm_table
      fsm_arc_show_tooltip .fsmwin.pw.t.tl [lindex $selected_cell 0] [lindex $selected_cell 1]
      set text_x [$metric_src(fsm).txt xview]
      set text_y [$metric_src(fsm).txt yview]
      process_fsm_cov
      $metric_src(fsm).txt xview moveto [lindex $text_x 0]
      $metric_src(fsm).txt yview moveto [lindex $text_y 0]
      populate_treeview
      enable_cdd_save
      set_pointer curr_fsm_ptr $curr_fsm_ptr fsm
    }
  }

  # Clear out the selection
  .fsmwin.pw.t.tl selection clear [.fsmwin.pw.t.tl curselection]

}

proc create_fsm_window {expr_id} {

  global prev_fsm_index next_fsm_index HOME
  global fsm_geometry fsm_gui_saved

  if {[winfo exists .fsmwin] == 0} {

    set fsm_gui_saved 0

    # Create new window
    toplevel .fsmwin
    wm title .fsmwin "FSM State/State Transition Coverage - Verbose"

    # Create all frames for window
    ttk::panedwindow .fsmwin.pw    -width 500 -height 350 -orient vertical
    ttk::frame       .fsmwin.pw.io -relief raised -borderwidth 1
    ttk::frame       .fsmwin.pw.t  -relief raised -borderwidth 1

    # Add movable frames to panedwindow
    .fsmwin.pw add .fsmwin.pw.io
    .fsmwin.pw add .fsmwin.pw.t

    # Create input/output expression frame components
    ttk::label .fsmwin.pw.io.l -anchor w -text "FSM Input/Output State Expressions"
    text .fsmwin.pw.io.t -height 4 -width 60 -xscrollcommand ".fsmwin.pw.io.hb set" \
                        -yscrollcommand ".fsmwin.pw.io.vb set" -wrap none -state disabled
    ttk::scrollbar .fsmwin.pw.io.hb -orient horizontal -command ".fsmwin.pw.io.t xview"
    ttk::scrollbar .fsmwin.pw.io.vb -command ".fsmwin.pw.io.t yview"

    # Pack the input/output expression frame
    grid rowconfigure    .fsmwin.pw.io 1 -weight 1
    grid columnconfigure .fsmwin.pw.io 0 -weight 1
    grid .fsmwin.pw.io.l   -row 0 -column 0 -sticky news
    grid .fsmwin.pw.io.t   -row 1 -column 0 -sticky news
    grid .fsmwin.pw.io.vb  -row 1 -column 1 -sticky news
    grid .fsmwin.pw.io.hb  -row 2 -column 0 -sticky news

    # Create FSM tablelist frame
    ttk::label .fsmwin.pw.t.l -anchor w -text "FSM State and State Transition Table"
    tablelist::tablelist .fsmwin.pw.t.tl -titlecolumns 1 -xscrollcommand {.fsmwin.pw.t.hb set} -yscrollcommand {.fsmwin.pw.t.vb set} -showseparators 1 -resizablecolumns 0 -selecttype cell -stripebg #EDF3FE -bg white -tooltipaddcommand fsm_arc_show_tooltip -tooltipdelcommand fsm_arc_hide_tooltip
    ttk::scrollbar .fsmwin.pw.t.hb -command {.fsmwin.pw.t.tl xview} -orient horizontal
    ttk::scrollbar .fsmwin.pw.t.vb -command {.fsmwin.pw.t.tl yview} 

    bind .fsmwin.pw.t.tl <<TablelistSelect>> {fsm_tablelist_selected}

    # Pack the tablelist frame
    grid rowconfigure    .fsmwin.pw.t 1 -weight 1
    grid columnconfigure .fsmwin.pw.t 0 -weight 1
    grid .fsmwin.pw.t.l  -row 0 -column 0 -sticky news
    grid .fsmwin.pw.t.tl -row 1 -column 0 -sticky news
    grid .fsmwin.pw.t.vb -row 1 -column 1 -sticky ns
    grid .fsmwin.pw.t.hb -row 2 -column 0 -sticky ew

    # Create the button frame
    ttk::frame .fsmwin.bf -relief raised -borderwidth 1
    ttk::label .fsmwin.bf.prev -image [image create photo -file [file join $HOME scripts left_arrow.gif]] -state disabled
    bind .fsmwin.bf.prev <Button-1> {
      display_fsm $prev_fsm_index
    }
    set_balloon .fsmwin.bf.prev "Click to view the previous uncovered FSM in this window"
    ttk::label .fsmwin.bf.next -image [image create photo -file [file join $HOME scripts right_arrow.gif]] -state disabled
    bind .fsmwin.bf.next <Button-1> {
      display_fsm $next_fsm_index
    }
    set_balloon .fsmwin.bf.prev "Click to view the next uncovered FSM in this window"
    ttk::button .fsmwin.bf.close -text "Close" -width 10 -command {
      destroy .fsmwin
    }
    help_button .fsmwin.bf.help chapter.gui.fsm

    # Pack the buttons into the button frame
    pack .fsmwin.bf.prev  -side left
    pack .fsmwin.bf.next  -side left
    pack .fsmwin.bf.help  -side right -padx 4 -pady 4
    pack .fsmwin.bf.close -side right -padx 4 -pady 4

    # Create general information window
    ttk::label .fsmwin.info -anchor w -relief raised -width 60

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

proc display_fsm_window {expr_id} {

  global curr_block
  global fsm_curr_info
  global fsm_in_states fsm_out_states fsm_in_hit_states fsm_out_hit_states
  global fsm_arcs fsm_hit_arcs
  global fsm_in_state fsm_out_state
  global curr_fsm_expr_id

  # Get information from design for this expression ID
  set fsm_in_states      ""
  set fsm_out_states     ""
  set fsm_in_hit_states  ""
  set fsm_out_hit_states ""
  set fsm_arcs           ""
  set fsm_hit_arcs       ""
  set fsm_in_state       ""
  set fsm_out_state      ""
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

proc display_fsm_table {} {

  global fsm_in_states fsm_in_hit_states fsm_out_states fsm_out_hit_states fsm_arcs fsm_hit_arcs
  global preferences

  # Delete all columns if this tablelist has been previously populated
  .fsmwin.pw.t.tl delete 0 end
  if {[.fsmwin.pw.t.tl columncount] > 0} {
    for {set i 1} {$i < [.fsmwin.pw.t.tl columncount]} {incr i} {
      .fsmwin.pw.t.tl columnconfigure $i -labelbg $preferences(uncov_fgColor)
    }
  }

  # Create the table
  set out_states {0 "IN \\ OUT"}
  foreach out_state $fsm_out_states {
    lappend out_states 0 [fsm_gen_vertical_text $out_state]
  }
  .fsmwin.pw.t.tl configure -columns $out_states

  # Add in the input FSM states
  foreach in_state $fsm_in_states {
    .fsmwin.pw.t.tl insert end [join $in_state [lrepeat [llength $fsm_out_states] {}]]
  }

  # Now add the total arcs
  foreach arc $fsm_arcs {
    set row [lsearch $fsm_in_states [lindex $arc 0]]
    set col [expr [lsearch $fsm_out_states [lindex $arc 1]] + 1]
    if {[lindex $arc 2] == 1} {
      .fsmwin.pw.t.tl cellconfigure $row,$col -bg $preferences(cov_bgColor) -fg $preferences(cov_fgColor) -text "E"
    } else {
      .fsmwin.pw.t.tl cellconfigure $row,$col -bg $preferences(uncov_bgColor) -fg $preferences(uncov_fgColor) -text "I"
    }
  }

  # Add the hit arcs
  foreach arc $fsm_hit_arcs {
    set row [lsearch $fsm_in_states [lindex $arc 0]]
    set col [expr [lsearch $fsm_out_states [lindex $arc 1]] + 1]
    .fsmwin.pw.t.tl cellconfigure $row,$col -bg $preferences(cov_bgColor) -fg $preferences(cov_fgColor) -text ""
  }

  # Finally, color the input/output states accordingly
  for {set col 1} {$col < [expr [llength $fsm_out_states] + 1]} {incr col} {
    for {set row 0} {$row < [llength $fsm_in_states]} {incr row} {
      if {[.fsmwin.pw.t.tl cellcget $row,$col -bg] == $preferences(cov_bgColor)} {
        .fsmwin.pw.t.tl columnconfigure $col -labelbg $preferences(cov_bgColor) -labelfg $preferences(cov_fgColor)
        .fsmwin.pw.t.tl cellconfigure $row,0 -bg $preferences(cov_bgColor) -fg $preferences(cov_fgColor) \
                                             -selectbackground $preferences(cov_bgColor) -selectforeground $preferences(cov_fgColor)
      } elseif {[.fsmwin.pw.t.tl cellcget $row,$col -bg] == $preferences(uncov_bgColor)} {
        if {[.fsmwin.pw.t.tl columncget $col -labelbg] != $preferences(cov_bgColor)} {
          .fsmwin.pw.t.tl columnconfigure $col -labelbg $preferences(uncov_bgColor) -labelfg $preferences(uncov_fgColor)
        }
        if {[.fsmwin.pw.t.tl cellcget $row,0 -bg] != $preferences(cov_bgColor)} {
          .fsmwin.pw.t.tl cellconfigure $row,0 -bg $preferences(uncov_bgColor) -fg $preferences(uncov_fgColor) \
                                               -selectbackground $preferences(uncov_bgColor) -selectforeground $preferences(uncov_fgColor)
        }
      } else {
        .fsmwin.pw.t.tl cellconfigure $row,$col -bg white -fg white
      }
    }
  }

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

# Main function to call that will create the FSM verbose viewer (if needed) and populate it
# with the currently selected uncovered FSM.
proc display_fsm {curr_index} {

  global prev_fsm_index next_fsm_index curr_fsm_ptr
  global uncovered_fsms start_line metric_src

  # Calculate expression ID
  set all_ranges [$metric_src(fsm).txt tag ranges uncov_button]
  set my_range   [$metric_src(fsm).txt tag prevrange uncov_button "$curr_index + 1 chars"]
  set index [expr [lsearch -exact $all_ranges [lindex $my_range 0]] / 2]
  set expr_id [lindex [lindex $uncovered_fsms $index] 2]

  # Get range of current signal
  set curr_range [$metric_src(fsm).txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Make sure that the selected signal is visible in the text box and is shown as selected
  set_pointer curr_fsm_ptr [lindex [split [lindex $my_range 0] .] 0] fsm
  goto_uncov [lindex $my_range 0] fsm

  # Get range of previous signal
  set prev_fsm_index [lindex [$metric_src(fsm).txt tag prevrange uncov_button [lindex $curr_index 0]] 0]

  # Get range of next signal
  set next_fsm_index [lindex [$metric_src(fsm).txt tag nextrange uncov_button [lindex $curr_range 1]] 0]

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
      set_pointer curr_fsm_ptr $curr_fsm_ptr fsm

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

