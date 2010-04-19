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

set curr_assert_ptr  ""
set assert_geometry  ""
set assert_gui_saved 0
set assert_excluded  0
set assert_reason    ""

proc assert_yset {args} {

  eval [linsert $args 0 .assertwin.f.vb set]
  assert_yview moveto [lindex [.assertwin.f.vb get] 0]

}

proc assert_yview {args} {

  eval [linsert $args 0 .assertwin.f.td yview]
  eval [linsert $args 0 .assertwin.f.tc yview]

}

proc display_assert {curr_index} {

  global prev_assert_index next_assert_index curr_assert_ptr curr_assert_inst metric_src

  # Get range of current instance
  set curr_range [$metric_src(assert).txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Calculate the current instance string
  set curr_assert_inst [string trim [lindex [split [$metric_src(assert).txt get [lindex $curr_range 0] [lindex $curr_range 1]] "\["] 0]]

  # Make sure that the selected instance is visible in the text box and is shown as selected
  set_pointer curr_assert_ptr [lindex [split [lindex $curr_range 0] .] 0] assert
  goto_uncov [lindex $curr_range 0] assert

  # Get range of previous instance
  set prev_assert_index [lindex [$metric_src(assert).txt tag prevrange uncov_button [lindex $curr_range 0]] 0]

  # Get range of next instance
  set next_assert_index [lindex [$metric_src(assert).txt tag nextrange uncov_button [lindex $curr_range 1]] 0]

  # Now create the assertion window
  create_assert_window $curr_assert_inst

}

proc create_assert_window {inst} {

  global prev_assert_index next_assert_index
  global curr_block metric_src
  global curr_assert_ptr assert_cov_points assert_cov_mod
  global preferences
  global HOME assert_geometry assert_gui_saved

  # Now create the window and set the grab to this window
  if {[winfo exists .assertwin] == 0} {

    set assert_gui_saved 0

    # Create new window
    toplevel .assertwin
    wm title .assertwin "Assertion Coverage - Verbose"

    ttk::frame .assertwin.f -relief raised -borderwidth 1

    # Add toggle information
    ttk::label .assertwin.f.ld -anchor w -text "Coverage point description"
    ttk::label .assertwin.f.lc -anchor w -text "# of hits"
    text  .assertwin.f.td -height 5 -width 40 -borderwidth 0 -xscrollcommand ".assertwin.f.hb set" \
          -yscrollcommand assert_yset -wrap none -spacing1 2 -spacing3 3
    text  .assertwin.f.tc -height 5 -width 10 -borderwidth 0 -yscrollcommand assert_yset -wrap none -spacing1 2 -spacing3 3
    ttk::scrollbar .assertwin.f.hb -orient horizontal -command ".assertwin.f.td xview"
    ttk::scrollbar .assertwin.f.vb -command assert_yview
    ttk::button .assertwin.f.b -text "Show Code" -command {
      display_assertion_module [lindex $assert_cov_mod 0] [lindex $assert_cov_mod 1]
    }

    # Create bottom information bar
    ttk::label .assertwin.f.info -anchor w

    # Create bottom button bar
    ttk::frame .assertwin.bf -relief raised -borderwidth 1
    ttk::button .assertwin.bf.close -text "Close" -width 10 -command {
      rm_pointer curr_assert_ptr assert
      destroy .assertwin
      destroy .amodwin
    }
    help_button .assertwin.bf.help chapter.gui.assert
    ttk::label .assertwin.bf.prev -image [image create photo -file [file join $HOME scripts left_arrow.gif]]
    bind .assertwin.bf.prev <Button-1> {
      display_assert $prev_assert_index
    }
    set_balloon .assertwin.bf.prev "Click to view the previous uncovered assertion in this window"
    ttk::label .assertwin.bf.next -image [image create photo -file [file join $HOME scripts right_arrow.gif]]
    bind .assertwin.bf.next <Button-1> {
      display_assert $next_assert_index
    }
    set_balloon .assertwin.bf.next "Click to view the next uncovered assertion in this window"

    # Pack the buttons into the button frame
    pack .assertwin.bf.prev  -side left
    pack .assertwin.bf.next  -side left
    pack .assertwin.bf.help  -side right -pady 3
    pack .assertwin.bf.close -side right -padx 3 -pady 3

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

    # Set the geometry if it is known
    if {$assert_geometry != ""} {
      wm geometry .assertwin $assert_geometry
    }

    # Handle the destruction of the assertion window
    wm protocol .assertwin WM_DELETE_WINDOW {
      save_assert_gui_elements 0
      destroy .assertwin
    }
    bind .assertwin <Destroy> {
      save_assert_gui_elements 0
    }

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
  tcl_func_get_assert_coverage $curr_block $inst

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
  .assertwin.f.td tag configure td_uncov_colorMap -foreground $preferences(uncov_fgColor) -background $preferences(uncov_bgColor)
  .assertwin.f.td tag configure td_cov_colorMap   -foreground $preferences(cov_fgColor)   -background $preferences(cov_bgColor)
  .assertwin.f.tc tag configure tc_uncov_colorMap -foreground $preferences(uncov_fgColor) -background $preferences(uncov_bgColor)
  .assertwin.f.tc tag configure tc_cov_colorMap   -foreground $preferences(cov_fgColor)   -background $preferences(cov_bgColor)
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
    set assert_reason ""
    if {$preferences(exclude_reasons_enabled) == 1 && $curr_excl == 1} {
      set assert_reason [get_exclude_reason .assertwin]
    }
    tcl_func_set_assert_exclude $curr_block $curr_assert_inst $curr_exp $curr_excl $assert_reason
    set text_x [$metric_src(assert).txt xview]
    set text_y [$metric_src(assert).txt yview]
    process_assert_cov
    $metric_src(assert).txt xview moveto [lindex $text_x 0]
    $metric_src(assert).txt yview moveto [lindex $text_y 0]
    populate_treeview
    enable_cdd_save
    set_pointer curr_assert_ptr $curr_assert_ptr assert
    create_assert_window $curr_assert_inst
  }
  .assertwin.f.tc tag bind tc_uncov_uline <ButtonPress-3> {
    set curr_index      [expr [lindex [split [.assertwin.f.tc index current] .] 0] - 1]
    set assert_excluded [lindex [lindex $assert_cov_points $curr_index] 3]
    set assert_reason   [lindex [lindex $assert_cov_points $curr_index] 4]
    if {$assert_excluded == 1 && $assert_reason != ""} {
      balloon::show .assertwin.f.tc "Exclude Reason: $assert_reason" $preferences(cov_bgColor) $preferences(cov_fgColor)
    }
  }
  .assertwin.f.tc tag bind tc_uncov_uline <ButtonRelease-3> {
    set curr_index [expr [lindex [split [.assertwin.f.tc index current] .] 0] - 1]
    set assert_excluded [lindex [lindex $assert_cov_points $curr_index] 3]
    set assert_reason   [lindex [lindex $assert_cov_points $curr_index] 4]
    if {$assert_excluded == 1 && $assert_reason != ""} {
      balloon::hide .assertwin.f.tc
    }
  }

  # Keep user from writing in text boxes
  .assertwin.f.td configure -state disabled
  .assertwin.f.tc configure -state disabled
  
  # Update informational bar
  .assertwin.f.info configure -text "Assertion module: [lindex $assert_cov_mod 0]"

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
    if {$cov_rb != "Assert"} {

      .assertwin.bf.prev configure -state disabled
      .assertwin.bf.next configure -state disabled

    } else {

      # Restore the pointer if it is set
      set_pointer curr_assert_ptr $curr_assert_ptr assert

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

proc display_assertion_module {name filename} {

  global open_files

  if {[winfo exists .amodwin] == 0} {

    set open_files ""

    # Create the window viewer
    toplevel .amodwin
    set assert_filename [file normalize [file join [tcl_func_get_score_path] $filename]]
    wm title .amodwin "Assertion Module - $name ($assert_filename)"

    # Create text viewer windows
    ttk::frame .amodwin.f -relief raised -borderwidth 1
    text .amodwin.f.t -height 30 -width 80 -xscrollcommand ".amodwin.f.hb set" -yscrollcommand ".amodwin.f.vb set" -state disabled
    ttk::scrollbar .amodwin.f.hb -orient horizontal -command ".amodwin.f.t xview"
    ttk::scrollbar .amodwin.f.vb -command ".amodwin.f.t yview"
    ttk::label .amodwin.f.info -anchor w

    # Layout the text viewer windows
    grid rowconfigure    .amodwin.f 0 -weight 1
    grid columnconfigure .amodwin.f 0 -weight 1
    grid .amodwin.f.t    -row 0 -column 0 -sticky news
    grid .amodwin.f.vb   -row 0 -column 1 -sticky news
    grid .amodwin.f.hb   -row 1 -column 0 -sticky news
    grid .amodwin.f.info -row 2 -column 0 -columnspan 2 -sticky we

    # Create the button bar
    ttk::frame  .amodwin.bf -relief raised -borderwidth 1
    ttk::button .amodwin.bf.back -text "Back" -width 10 -command {
      set fname [lindex $open_files 1]
      set open_files [lreplace $open_files 0 1]
      populate_assertion_text $fname
    } -state disabled
    ttk::button .amodwin.bf.close -text "Close" -width 10 -command {
      destroy .amodwin
    }
    help_button .amodwin.bf.help chapter.gui.assert.source

    # Pack the button bar
    pack .amodwin.bf.back  -side left  -padx 3 -pady 3
    pack .amodwin.bf.help  -side right -pady 3
    pack .amodwin.bf.close -side right -padx 3 -pady 3

    # Pack the frames
    pack .amodwin.f -fill both -expand yes
    pack .amodwin.bf -fill x
    
  }

  # Write the module information
  populate_assertion_text $filename

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

# Saves the contents of the assertion detailed window for later usage
proc save_assert_gui_elements {main_exit} {

  global assert_geometry assert_gui_saved

  if {$assert_gui_saved == 0} {
    if {$main_exit == 0 || [winfo exists .assertwin] == 1} {
      set assert_gui_saved 1
      set assert_geometry  [winfo geometry .assertwin]
    }
  }

}
