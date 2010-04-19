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

set mem_name         ""
set curr_memory_ptr  ""
set mem_curr_entry   ""
set memory_geometry  ""
set memory_gui_saved 0

proc display_memory {curr_index} {

  global prev_memory_index next_memory_index curr_memory_ptr curr_mem_index metric_src

  # Get range of current signal
  set curr_range [$metric_src(memory).txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Calculate the current signal string
  set curr_signal [string trim [lindex [split [$metric_src(memory).txt get [lindex $curr_range 0] [lindex $curr_range 1]] "\["] 0]]

  # Clear global memory index
  set curr_mem_index ""

  # Make sure that the selected signal is visible in the text box and is shown as selected
  set_pointer curr_memory_ptr [lindex [split [lindex $curr_range 0] .] 0] memory
  goto_uncov [lindex $curr_range 0] memory

  # Get range of previous signal
  set prev_memory_index [lindex [$metric_src(memory).txt tag prevrange uncov_button [lindex $curr_index 0]] 0]

  # Get range of next signal
  set next_memory_index [lindex [$metric_src(memory).txt tag nextrange uncov_button [lindex $curr_range 1]] 0]

  # Now create the memory window
  create_memory_window $curr_signal

}

proc create_memory_window {signal} {

  global memory_udim memory_pdim memory_pdim_array memory_array memory_excluded memory_reason
  global mem_name prev_memory_index next_memory_index
  global curr_block metric_src
  global curr_memory_ptr curr_mem_index
  global preferences
  global HOME
  global memory_geometry memory_gui_saved

  set mem_name $signal

  # Now create the window and set the grab to this window
  if {[winfo exists .memwin] == 0} {

    set memory_gui_saved 0

    # Create new window
    toplevel .memwin
    wm title .memwin "Memory Coverage - Verbose"

    # Create upper frame
    ttk::frame .memwin.f

    # Add addressable memory elements selector widgets
    ttk::frame .memwin.f.fae -relief raised -borderwidth 1
    ttk::label .memwin.f.fae.l -anchor w -text "Addressable Memory Elements"
    text .memwin.f.fae.t -height 10 -xscrollcommand ".memwin.f.fae.hb set" -wrap none -state disabled
    ttk::scrollbar .memwin.f.fae.hb -orient horizontal -command ".memwin.f.fae.t"

    # Create exclude checkbutton
    ttk::checkbutton .memwin.f.fae.excl -text "Excluded" -variable memory_excluded -command {
      set memory_reason "" 
      if {$preferences(exclude_reasons_enabled) == 1 && $memory_excluded == 1} {
        set memory_reason [get_exclude_reason .memwin]
      }
      tcl_func_set_memory_exclude $curr_block $mem_name $memory_excluded $memory_reason
      set text_x [$metric_src(memory).txt xview]
      set text_y [$metric_src(memory).txt yview]
      process_memory_cov
      create_memory_window $mem_name
      $metric_src(memory).txt xview moveto [lindex $text_x 0]
      $metric_src(memory).txt yview moveto [lindex $text_y 0]
      populate_treeview
      enable_cdd_save
      set_pointer curr_memory_ptr $curr_memory_ptr memory
    }
    set_exclude_reason_balloon .memwin.f.fae.excl {$memory_excluded} {$memory_reason}
    set_balloon .memwin.f.fae.excl "If set, excludes this entire memory from coverage consideration"

    # Pack the addressable memory elements frame
    grid rowconfigure    .memwin.f.fae 1 -weight 1
    grid columnconfigure .memwin.f.fae 0 -weight 1
    grid .memwin.f.fae.l    -row 0 -column 0 -sticky new
    grid .memwin.f.fae.excl -row 0 -column 1 -sticky new
    grid .memwin.f.fae.t    -row 1 -column 0 -columnspan 2 -sticky news
    grid .memwin.f.fae.hb   -row 2 -column 0 -columnspan 2 -sticky ew 

    # Add memory toggle widgets
    ttk::frame .memwin.f.ft -relief raised -borderwidth 1
    ttk::label .memwin.f.ft.l -anchor w -text "Element Coverage"
    ttk::label .memwin.f.ft.l_01 -anchor w -text "Toggle 0->1"
    ttk::label .memwin.f.ft.l_10 -anchor w -text "Toggle 1->0"
    text  .memwin.f.ft.t -height 2 -width 40 -xscrollcommand ".memwin.f.ft.hb set" -wrap none -spacing1 2 -spacing3 3 -state disabled
    ttk::scrollbar .memwin.f.ft.hb -orient horizontal -command ".memwin.f.ft.t xview"

    # Create bottom information bar
    ttk::label .memwin.f.ft.info -anchor e

    # Add memory write/read widgets
    ttk::label .memwin.f.ft.l_wr -anchor w -text "Written:"
    ttk::label .memwin.f.ft.l_wv -anchor w -width 3
    ttk::label .memwin.f.ft.l_rd -anchor w -text "Read:"
    ttk::label .memwin.f.ft.l_rv -anchor w -width 3

    # Pack the toggle/write/read widgets
    grid columnconfigure .memwin.f.ft 0 -weight 1
    grid .memwin.f.ft.l    -row 0 -column 0 -sticky new
    grid .memwin.f.ft.t    -row 1 -rowspan 2 -column 0 -columnspan 5 -sticky new
    grid .memwin.f.ft.l_01 -row 1 -column 5 -sticky new
    grid .memwin.f.ft.l_10 -row 2 -column 5 -sticky new
    grid .memwin.f.ft.hb   -row 3 -column 0 -columnspan 5 -sticky new
    grid .memwin.f.ft.info -row 4 -column 0 -columnspan 5 -sticky new
    grid .memwin.f.ft.l_wr -row 5 -column 1 -sticky new
    grid .memwin.f.ft.l_wv -row 5 -column 2 -sticky new
    grid .memwin.f.ft.l_rd -row 5 -column 3 -sticky new
    grid .memwin.f.ft.l_rv -row 5 -column 4 -sticky new

    # Create bottom button bar
    ttk::frame .memwin.bf -relief raised -borderwidth 1
    ttk::button .memwin.bf.close -text "Close" -width 10 -command {
      rm_pointer curr_memory_ptr memory
      destroy .memwin
    }
    help_button .memwin.bf.help chapter.gui.memory ""
    ttk::label .memwin.bf.prev -image [image create photo -file [file join $HOME scripts left_arrow.gif]]
    bind .memwin.bf.prev <Button-1> {
      display_memory $prev_memory_index
    }
    set_balloon .memwin.bf.prev "Click to view the previous uncovered memory in this window"
    ttk::label .memwin.bf.next -image [image create photo -file [file join $HOME scripts right_arrow.gif]]
    bind .memwin.bf.next <Button-1> {
      display_memory $next_memory_index
    }
    set_balloon .memwin.bf.next "Click to view the next uncovered memory in this window"

    # Pack the buttons into the button frame
    pack .memwin.bf.prev  -side left
    pack .memwin.bf.next  -side left
    pack .memwin.bf.help  -side right -padx 4 -pady 4
    pack .memwin.bf.close -side right -padx 4 -pady 4

    # Pack the upper frames
    pack .memwin.f.fae -fill both -expand yes
    pack .memwin.f.ft  -fill both

    pack .memwin.f  -fill both -expand yes
    pack .memwin.bf -fill x

    # Set the geometry of the window if necessary
    if {$memory_geometry != ""} {
      wm geometry .memwin $memory_geometry
    }

    # Handle the destruction of this window
    wm protocol .memwin WM_DELETE_WINDOW {
      save_memory_gui_elements 0
      destroy .memwin
    }
    bind .memwin <Destroy> {
      save_memory_gui_elements 0
    }

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
  set memory_info       [tcl_func_get_memory_coverage $curr_block $signal]
  set memory_udim       [lindex $memory_info 0]
  set memory_pdim       [lindex $memory_info 1]
  set memory_pdim_array [lindex $memory_info 2]
  set memory_array      [lindex $memory_info 3]
  set memory_excluded   [lindex $memory_info 4]
  set memory_reason     [lindex $memory_info 5]

  #################################################
  # POPULATE THE ADDRESSABLE MEMORY ELEMENT TEXTBOX
  #################################################

  # Start creating the textbox tag add commands
  set cmd_enter   ".memwin.f.fae.t tag add mem_enter"
  set cmd_cbutton ".memwin.f.fae.t tag add mem_cbutton"
  set cmd_ubutton ".memwin.f.fae.t tag add mem_ubutton"
  set cmd_leave   ".memwin.f.fae.t tag add mem_leave"

  # Allow us to clear out the memory element text box
  .memwin.f.fae.t configure -state normal

  # Remove any tags associated with the memory element text box

  # Clear the text-box before any insertion is being made
  .memwin.f.fae.t delete 1.0 end

  # Create initial lines
  for {set i 0} {$i < 10} {incr i} {
    .memwin.f.fae.t insert end "\n"
  }

  # Insert memory entries in the given textbox
  for {set j 0} {$j < [llength $memory_array]} {incr j 10} {

    for {set i 0} {$i < 10} {incr i} {

      # Populate textbox saving positions
      set entry [lindex $memory_array [expr $j + $i]]
      set start_pos [.memwin.f.fae.t index "[expr $i + 1].end"]
      .memwin.f.fae.t insert $start_pos "[lindex $entry 0]"
      set end_pos [.memwin.f.fae.t index "[expr $i + 1].end"]
      .memwin.f.fae.t insert "[expr $i + 1].end" "   "

      # Append to appropriate tags
      set cmd_enter [concat $cmd_enter $start_pos $end_pos]
      set cmd_leave [concat $cmd_leave $start_pos $end_pos]
      if {[lindex $entry 1] == 0 && $memory_excluded == 0} {
        set cmd_ubutton [concat $cmd_ubutton $start_pos $end_pos]
      } else {
        set cmd_cbutton [concat $cmd_cbutton $start_pos $end_pos]
      }

    }

  }

  # Evaluate tag commands
  eval $cmd_enter
  eval $cmd_leave
  if {$cmd_ubutton != ".memwin.f.fae.t tag add mem_ubutton"} {
    eval $cmd_ubutton
  }
  if {$cmd_cbutton != ".memwin.f.fae.t tag add mem_cbutton"} {
    eval $cmd_cbutton
  }

  # Configure tags
  .memwin.f.fae.t tag configure mem_ubutton -background $preferences(uncov_bgColor) -foreground $preferences(uncov_fgColor)
  .memwin.f.fae.t tag configure mem_cbutton -background $preferences(cov_bgColor)   -foreground $preferences(cov_fgColor)

  # Bind tags
  .memwin.f.fae.t tag bind mem_enter <Enter> {
    set curr_mem_cursor [.memwin.f.fae.t cget -cursor]
    .memwin.f.fae.t configure -cursor hand2
  }
  .memwin.f.fae.t tag bind mem_leave <Leave> {
    .memwin.f.fae.t configure -cursor $curr_mem_cursor
  }
  .memwin.f.fae.t tag bind mem_ubutton <ButtonPress-1> {
    set mem_curr_range [.memwin.f.fae.t tag prevrange mem_enter "current + 1 chars"]
    .memwin.f.fae.t tag delete mem_select
    .memwin.f.fae.t tag add mem_select [lindex $mem_curr_range 0] [lindex $mem_curr_range 1]
    .memwin.f.fae.t tag configure mem_select -background $preferences(uncov_fgColor) -foreground $preferences(uncov_bgColor)
    set curr_mem_index [expr [lsearch [.memwin.f.fae.t tag ranges mem_enter] [lindex $mem_curr_range 0]] / 2]
    populate_memory_entry_frame $curr_mem_index
  }
  .memwin.f.fae.t tag bind mem_cbutton <ButtonPress-1> {
    set mem_curr_range [.memwin.f.fae.t tag prevrange mem_enter "current + 1 chars"]
    .memwin.f.fae.t tag delete mem_select
    .memwin.f.fae.t tag add mem_select [lindex $mem_curr_range 0] [lindex $mem_curr_range 1]
    .memwin.f.fae.t tag configure mem_select -background $preferences(cov_fgColor) -foreground $preferences(cov_bgColor)
    set curr_mem_index [expr [lsearch [.memwin.f.fae.t tag ranges mem_enter] [lindex $mem_curr_range 0]] / 2]
    populate_memory_entry_frame $curr_mem_index
  }
  
  # Keep user from writing in text box
  .memwin.f.fae.t configure -state disabled

  # Set the information bar in the toggle frame
  .memwin.f.ft.info configure -text "$mem_name$memory_udim$memory_pdim"

  # Repopulate current memory index, if you has been set
  if {$curr_mem_index != ""} {
    populate_memory_entry_frame $curr_mem_index
  }

  # Raise this window to the foreground
  raise .memwin

}

proc populate_memory_entry_frame {sel_mem_index} {

  global memory_array memory_pdim memory_udim memory_pdim mem_name
  global memory_msb memory_lsb mem_curr_entry memory_excluded
  global preferences
  global memory_pdim_array

  # Get memory entry that corresponds to the given memory index
  set full_cols [expr [llength $memory_array] / 10]
  set last_rows [expr [llength $memory_array] % 10]
  if {$last_rows != 0} {
    set upper_blk [expr ($full_cols + 1) * $last_rows]
  } else {
    set upper_blk 0
  }
  if {$sel_mem_index < $upper_blk} {
    set index [expr (($sel_mem_index % ($full_cols + 1)) * 10) + ($sel_mem_index / ($full_cols + 1))]
  } else {
    set sel_mem_index [expr $sel_mem_index - $upper_blk]
    set index [expr (($sel_mem_index % $full_cols) * 10) + ($sel_mem_index / $full_cols) + $last_rows]
  }
  set entry [lindex $memory_array $index]

  # Save off the currently selected memory entry
  set mem_curr_entry [lindex $entry 0]

  # Setup MSB and LSB information for justification code below
  set memory_msb [expr [llength $memory_pdim_array] - 1]
  set memory_lsb 0

  # Allow us to clear out text box
  .memwin.f.ft.t configure -state normal

  # Remove any tags associated with the memory text box
  .memwin.f.ft.t tag delete mem_motion

  # Clear the text-box before any insertion is being made
  .memwin.f.ft.t delete 1.0 end

  # Write memory 0->1 and 1->0 information in text box
  .memwin.f.ft.t insert end "[lindex $entry 4]\n[lindex $entry 5]"

  # Keep user from writing in text boxes
  .memwin.f.ft.t configure -state disabled

  # Configure written/read labels
  if {[lindex $entry 2] == 0} {
    .memwin.f.ft.l_wv configure -text "No"
    if {$memory_excluded == 0} {
      .memwin.f.ft.l_wv configure -background $preferences(uncov_bgColor) -foreground $preferences(uncov_fgColor)
    } else {
      .memwin.f.ft.l_wv configure -background $preferences(cov_bgColor)   -foreground $preferences(uncov_fgColor)
    }
  } else {
    .memwin.f.ft.l_wv configure -text "Yes" -background $preferences(cov_bgColor) -foreground $preferences(cov_fgColor)
  }
  if {[lindex $entry 3] == 0} {
    .memwin.f.ft.l_rv configure -text "No"
    if {$memory_excluded == 0} {
      .memwin.f.ft.l_rv configure -background $preferences(uncov_bgColor) -foreground $preferences(uncov_fgColor)
    } else {
      .memwin.f.ft.l_rv configure -background $preferences(cov_bgColor)   -foreground $preferences(cov_fgColor)
    }
  } else {
    .memwin.f.ft.l_rv configure -text "Yes" -background $preferences(cov_bgColor) -foreground $preferences(cov_fgColor)
  }

  # Create leave bindings for textboxes
  bind .memwin.f.ft.t <Leave> {
    .memwin.f.ft.info configure -text "$mem_name$mem_curr_entry$memory_pdim"
  }

  # Add memory tags and bindings
  .memwin.f.ft.t tag add  mem_motion 1.0 {end - 1 chars}
  .memwin.f.ft.t tag bind mem_motion <Motion> {
    set tmp [expr ([llength $memory_pdim_array] - 1) - [lindex [split [.memwin.f.ft.t index current] "."] 1]]
    set tmp [lindex $memory_pdim_array $tmp]
    .memwin.f.ft.info configure -text "$mem_name$mem_curr_entry$tmp"
  }

  # Right justify the memory information
  if {[expr [expr $memory_msb - $memory_lsb] + 1] <= 32} {
    .memwin.f.ft.t tag configure mem_motion -justify right
  } else {
    .memwin.f.ft.t tag configure mem_motion -justify left
    .memwin.f.ft.t xview moveto 1.0
  }

  .memwin.f.ft.info configure -text "$mem_name$mem_curr_entry$memory_pdim"

}

# Updates the GUI elements of the memory window when some type of event causes the
# current metric mode to change.
proc update_memory {} {

  global cov_rb prev_memory_index next_memory_index curr_memory_ptr

  # If the combinational window exists, update the GUI
  if {[winfo exists .memwin] == 1} {

    # If the current metric mode is not memory, disable the prev/next buttons
    if {$cov_rb != "Memory"} {

      .memwin.bf.prev configure -state disabled
      .memwin.bf.next configure -state disabled

    } else {

      # Restore the pointer if it is set
      set_pointer curr_memory_ptr $curr_memory_ptr memory

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

# Saves the GUI elements from the memory window setup that should be saved
proc save_memory_gui_elements {main_exit} {

  global memory_geometry memory_gui_saved

  if {$memory_gui_saved == 0} {
    if {$main_exit == 0 || [winfo exists .memwin] == 1} {
      set memory_gui_saved 1
      set memory_geometry  [winfo geometry .memwin]
    }
  }

}

