#!/usr/bin/env wish

# Include the necessary auxiliary files 
source $HOME/scripts/menu_create.tcl
source $HOME/scripts/cov_create.tcl
source $HOME/scripts/process_file.tcl
source $HOME/scripts/toggle.tcl
source $HOME/scripts/comb.tcl
source $HOME/scripts/fsm.tcl
source $HOME/scripts/help.tcl
source $HOME/scripts/summary.tcl
source $HOME/scripts/preferences.tcl
source $HOME/scripts/cdd_view.tcl
source $HOME/scripts/assert.tcl
source $HOME/scripts/verilog.tcl

set last_lb_index      ""
set lwidth             -1 
set start_search_index 1.0
set curr_uncov_index   ""
set prev_uncov_index   ""
set next_uncov_index   ""

proc main_view {} {

  global race_msgs prev_uncov_index next_uncov_index

  # Start off 

  # Get all race condition reason messages
  set race_msgs ""
  tcl_func_get_race_reason_msgs

  # Create the frame for menubar creation
  frame .menubar -width 710 -relief raised -borderwidth 1
  menu_create .menubar

  # Create the information frame
  frame .covbox -width 710 -height 25
  cov_create .covbox

  # Create the bottom frame
  frame .bot -width 120 -height 300

  # Create frames for pane handle
  frame .bot.left -relief raised -borderwidth 1
  frame .bot.right -relief raised -borderwidth 1
  frame .bot.handle -borderwidth 2 -relief raised -cursor sb_h_double_arrow

  place .bot.left  -relheight 1 -width -1
  place .bot.right -relheight 1 -relx 1 -anchor ne -width -1
  place .bot.handle -rely 0.5 -anchor s -width 8 -height 8

  bind .bot <Configure> {
    set W  [winfo width .bot]
    set X0 [winfo rootx .bot]
    if {$lwidth == -1} {
      set lwidth [expr .25 * $W]
    }
    main_place $W $lwidth
  }

  bind .bot.handle <B1-Motion> {
    set lwidth [expr %X - $X0]
    main_place $W $lwidth
  }

  # Create the listbox label
  label .bot.left.ll -text "Modules" -anchor w

  # Create the textbox header frame
  frame .bot.right.h
  label .bot.right.h.tl -text "Cur   Line #       Verilog Source" -anchor w
  button .bot.right.h.prev -text "<--" -state disabled -command {
    goto_uncov $prev_uncov_index
  }
  button .bot.right.h.next -text "-->" -state disabled -command {
    goto_uncov $next_uncov_index
  }
  button .bot.right.h.find -text "Find:" -state disabled -command {
    perform_search [.bot.right.h.e get]
  }
  entry .bot.right.h.e -width 15 -relief sunken -state disabled
  bind .bot.right.h.e <Return> {
    perform_search [.bot.right.h.e get]
  }
  button .bot.right.h.clear -text "Clear" -state disabled -command {
    .bot.right.txt tag delete search_found
    .bot.right.h.e delete 0 end
    set start_search_index 1.0
  }

  # Pack the textbox header frame
  pack .bot.right.h.tl    -side left  -fill both
  pack .bot.right.h.clear -side right -fill both
  pack .bot.right.h.e     -side right -fill both
  pack .bot.right.h.find  -side right -fill both
  pack .bot.right.h.next  -side right -fill both
  pack .bot.right.h.prev  -side right -fill both

  # Create the listbox widget to display file names
  listbox .bot.left.l -yscrollcommand ".bot.left.lvb set" -xscrollcommand ".bot.left.lhb set" -width 30
  bind .bot.left.l <<ListboxSelect>> populate_text
  scrollbar .bot.left.lvb -command ".bot.left.l yview"
  scrollbar .bot.left.lhb -orient horizontal -command ".bot.left.l xview"

  # Create the text widget to display the modules/instances
  text .bot.right.txt -yscrollcommand ".bot.right.vb set" -xscrollcommand ".bot.right.hb set" -wrap none -state disabled
  scrollbar .bot.right.vb -command ".bot.right.txt yview"
  scrollbar .bot.right.hb -orient horizontal -command ".bot.right.txt xview"

  # Create bottom information bar
  label .info -anchor w -relief raised -borderwidth 1

  # Pack the widgets into the bottom left and right frames
  grid rowconfigure    .bot.left 1 -weight 1
  grid columnconfigure .bot.left 0 -weight 1
  grid .bot.left.ll  -row 0 -column 0 -sticky nsew
  grid .bot.left.l   -row 1 -column 0 -sticky nsew
  grid .bot.left.lvb -row 1 -column 1 -sticky ns
  grid .bot.left.lhb -row 2 -column 0 -sticky ew

  grid rowconfigure    .bot.right 1 -weight 1
  grid columnconfigure .bot.right 0 -weight 1
  grid .bot.right.h   -row 0 -column 0 -columnspan 2 -sticky nsew
  grid .bot.right.txt -row 1 -column 0 -sticky nsew
  grid .bot.right.vb  -row 1 -column 1 -sticky ns
  grid .bot.right.hb  -row 2 -column 0 -sticky ew

  # Pack the widgets
  pack .bot  -fill both -expand yes
  pack .info -fill both

  # Set the window icon
  global HOME
  # wm iconbitmap . @$HOME/images/myfile.xbm
  wm title . "Covered - Verilog Code Coverage Tool"

  # Set focus on the new window
  # wm focus .

}

proc main_place {width value} {

  place .bot.left   -width $value
  place .bot.handle -x $value
  place .bot.right  -width [expr $width - $value]

}
 
proc populate_listbox {listbox_w} {

  global mod_inst_type funit_names funit_types inst_list cov_rb file_name
  global line_summary_total line_summary_hit
  global toggle_summary_total toggle_summary_hit
  global uncov_fgColor uncov_bgColor
  global lb_fgColor lb_bgColor
 
  if {$file_name != 0} {

    # Remove contents currently in listbox
    set lb_size [$listbox_w size]
    $listbox_w delete 0 $lb_size

    # Clear funit_names and funit_types values
    set funit_names ""
    set funit_types ""

    # If we are in module mode, list modules (otherwise, list instances)
    if {$mod_inst_type == "module"} {
      tcl_func_get_funit_list 
      foreach funit_name $funit_names {
        $listbox_w insert end $funit_name
      }
      .bot.left.ll configure -text "Modules"
    } else {
      set inst_list ""
      tcl_func_get_instance_list
      foreach inst_name $inst_list {
        $listbox_w insert end $inst_name
      }
      .bot.left.ll configure -text "Instances"
    }

    # Get default colors of listbox
    set lb_fgColor [.bot.left.l itemcget 0 -foreground]
    set lb_bgColor [.bot.left.l itemcget 0 -background]

    highlight_listbox

  }

}

proc highlight_listbox {} {

  global file_name funit_names funit_types cov_rb
  global uncov_fgColor uncov_bgColor lb_fgColor lb_bgColor
  global line_summary_total line_summary_hit
  global toggle_summary_total toggle_summary_hit
  global comb_summary_total comb_summary_hit
  global fsm_summary_total fsm_summary_hit
  global assert_summary_total assert_summary_hit

  if {$file_name != 0} {

    # If we are in module mode, list modules (otherwise, list instances)
    set funits [llength $funit_names]
    for {set i 0} {$i < $funits} {incr i} {
      if {$cov_rb == "line"} {
        tcl_func_get_line_summary [lindex $funit_names $i] [lindex $funit_types $i]
        set fully_covered [expr $line_summary_total == $line_summary_hit]
      } elseif {$cov_rb == "toggle"} {
        tcl_func_get_toggle_summary [lindex $funit_names $i] [lindex $funit_types $i]
        set fully_covered [expr $toggle_summary_total == $toggle_summary_hit]
      } elseif {$cov_rb == "comb"} {
        tcl_func_get_comb_summary [lindex $funit_names $i] [lindex $funit_types $i]
        set fully_covered [expr $comb_summary_total == $comb_summary_hit]
      } elseif {$cov_rb == "fsm"} {
        tcl_func_get_fsm_summary [lindex $funit_names $i] [lindex $funit_types $i]
        set fully_covered [expr $fsm_summary_total == $fsm_summary_hit]
      } elseif {$cov_rb == "assert"} {
        tcl_func_get_assert_summary [lindex $funit_names $i] [lindex $funit_types $i]
        set fully_covered [expr $assert_summary_total == $assert_summary_hit]
      } else {
        # ERROR
      }
      if {$fully_covered == 0} {
        .bot.left.l itemconfigure $i -foreground $uncov_fgColor -background $uncov_bgColor
      } else {
        .bot.left.l itemconfigure $i -foreground $lb_fgColor -background $lb_bgColor
      }
    }

  }

}

proc populate_text {} {

  global cov_rb mod_inst_type funit_names funit_types
  global curr_funit_name curr_funit_type last_lb_index
  global start_search_index
  global curr_toggle_ptr

  set index [.bot.left.l curselection]

  if {$index != ""} {

    if {$last_lb_index != $index} {

      set last_lb_index $index
      set curr_funit_name [lindex $funit_names $index]
      set curr_funit_type [lindex $funit_types $index]
      set curr_toggle_ptr ""

      if {$cov_rb == "line"} {
        process_funit_line_cov
      } elseif {$cov_rb == "toggle"} {
        process_funit_toggle_cov
      } elseif {$cov_rb == "comb"} {
        process_funit_comb_cov
      } elseif {$cov_rb == "fsm"} {
        process_funit_fsm_cov
      } elseif {$cov_rb == "assert"} {
        process_funit_assert_cov
      } else {
        # ERROR
      }

      # Reset starting search index
      set start_search_index 1.0
      set curr_uncov_index   ""

      # Run initial goto_uncov to initialize previous and next pointers
      goto_uncov $curr_uncov_index

      # Enable widgets
      .bot.right.h.e     configure -state normal -bg white
      .bot.right.h.find  configure -state normal
      .bot.right.h.clear configure -state normal

    }

  }

}

proc perform_search {value} {

  global start_search_index

  set index [.bot.right.txt search $value $start_search_index]

  # Delete search_found tag
  .bot.right.txt tag delete search_found

  if {$index != ""} {

    # Highlight found text
    set value_len [string length $value]
    .bot.right.txt tag add search_found $index "$index + $value_len chars"
    .bot.right.txt tag configure search_found -background orange1

    # Make the text visible
    .bot.right.txt see $index 

    # Calculate next starting index
    set indices [split $index .]
    set start_search_index [lindex $indices 0].[expr [lindex $indices 1] + 1]

  } else {

    # Output a message specifying that the searched string could not be found
    tk_messageBox -message "String \"$value\" not found" -type ok -parent .

    # Clear the contents of the search entry box
    .bot.right.h.e delete 0 end

    # Reset search index
    set start_search_index 1.0

  }

  # Set focus to text box
  focus .bot.right.txt

  return 1

}

proc rm_pointer {curr_ptr} {

  upvar $curr_ptr ptr

  # Allow the textbox to be changed
  .bot.right.txt configure -state normal

  # Delete old cursor, if it is displayed
  if {$ptr != ""} {
    .bot.right.txt delete $ptr.0 $ptr.3
    .bot.right.txt insert $ptr.0 "   "
  }

  # Disable textbox
  .bot.right.txt configure -state disabled

  # Disable "Show current selection" menu item
  .menubar.view.menu entryconfigure 4 -state disabled

  # Clear current pointer
  set ptr ""

}

proc set_pointer {curr_ptr line} {

  upvar $curr_ptr ptr

  # Remove old pointer
  rm_pointer ptr

  # Allow the textbox to be changed
  .bot.right.txt configure -state normal

  # Display new pointer
  .bot.right.txt delete $line.0 $line.3
  .bot.right.txt insert $line.0 "-->"

  # Set the textbox to not be changed
  .bot.right.txt configure -state disabled

  # Make sure that we can see the current toggle pointer in the textbox
  .bot.right.txt see $line.0

  # Enable the "Show current selection" menu option
  .menubar.view.menu entryconfigure 4 -state normal

  # Set the current pointer to the specified line
  set ptr $line

}

proc goto_uncov {curr_index} {

  global prev_uncov_index next_uncov_index curr_uncov_index
  global cov_rb

  # Calculate the name of the tag to use
  if {$cov_rb == "line"} {
    set tag_name "uncov_colorMap"
  } else {
    set tag_name "uncov_button"
  }

  # Display the given index, if it has been specified
  if {$curr_index != ""} {
    .bot.right.txt see $curr_index
    set curr_uncov_index $curr_index
  } else {
    set curr_uncov_index 1.0
  }

  # Get previous uncovered index
  set prev_uncov_index [lindex [.bot.right.txt tag prevrange $tag_name $curr_uncov_index] 0]

  # Disable/enable previous button
  if {$prev_uncov_index != ""} {
    .bot.right.h.prev configure -state normal
    .menubar.view.menu entryconfigure 3 -state normal
  } else {
    .bot.right.h.prev configure -state disabled
    .menubar.view.menu entryconfigure 3 -state disabled
  }

  # Get next uncovered index
  set next_uncov_index [lindex [.bot.right.txt tag nextrange $tag_name "$curr_uncov_index + 1 chars"] 0]

  # Disable/enable next button
  if {$next_uncov_index != ""} {
    .bot.right.h.next configure -state normal
    .menubar.view.menu entryconfigure 2 -state normal
  } else {
    .bot.right.h.next configure -state disabled
    .menubar.view.menu entryconfigure 2 -state disabled
  }

}

proc update_all_windows {} {

  global curr_uncov_index

  # Update the main window
  goto_uncov $curr_uncov_index
  .menubar.view.menu entryconfigure 4 -state disabled

  # Update the summary window
  update_summary

  # Update the toggle window
  update_toggle

  # Update the combinational logic window
  update_comb

  # Update the FSM window
  update_fsm

  # Update the assertion window
  update_assert

}

proc bgerror {msg} {

  ;# Remove the status window if it exists
  destroy .status

  ;# Display error message
  set retval [tk_messageBox -message $msg -title "Error" -type ok]

}

# Read configuration file
read_coveredrc

# Display main window
main_view
