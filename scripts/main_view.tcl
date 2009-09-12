################################################################################################
# Copyright (c) 2006-2009 Trevor Williams                                                      #
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

# Include the necessary auxiliary files 
source [file join $HOME scripts menu_create.tcl]
source [file join $HOME scripts cov_create.tcl]
source [file join $HOME scripts process_file.tcl]
source [file join $HOME scripts toggle.tcl]
source [file join $HOME scripts comb.tcl]
source [file join $HOME scripts fsm.tcl]
source [file join $HOME scripts help.tcl]
source [file join $HOME scripts summary.tcl]
source [file join $HOME scripts preferences.tcl]
source [file join $HOME scripts cdd_view.tcl]
source [file join $HOME scripts assert.tcl]
source [file join $HOME scripts verilog.tcl]
source [file join $HOME scripts memory.tcl]
source [file join $HOME scripts wizard.tcl]
source [file join $HOME scripts gen_new.tcl]
source [file join $HOME scripts gen_report.tcl]
source [file join $HOME scripts gen_rank.tcl]
source [file join $HOME scripts viewer.tcl]
source [file join $HOME scripts balloon.tcl]
source [file join $HOME scripts exclude.tcl]

# The Tablelist package is used for displaying instance/module hit/miss/total/percent hit information
package require Tablelist
#package require tile
#package require tablelist_tile 4.8

set last_lb_index           ""
set lwidth                  -1 
set lwidth_h1               -1
set main_start_search_index 1.0
set curr_uncov_index        ""
set prev_uncov_index        ""
set next_uncov_index        ""
array set tablelistopts {
  selectbackground   RoyalBlue1
  selectforeground   white
  stretch	     all
  stripebackground   #EDF3FE
  relief             flat
  border             0
  showseparators     yes
  takefocus          0
  setfocus           1
  activestyle        none
}

proc main_view {} {

  global race_msgs prev_uncov_index next_uncov_index
  global HOME main_start_search_index
  global main_geometry
  global mod_inst_tl_columns mod_inst_tl_init_hidden mod_inst_tl_width

  # Start off 

  # Get all race condition reason messages
  set race_msgs ""
  tcl_func_get_race_reason_msgs

  # Create the frame for menubar creation
  menu_create

  # Create the information frame
  frame .covbox -width 710 -height 25 -relief raised -borderwidth 1
  cov_create .covbox

  # Create the bottom frame
  panedwindow .bot -width 1000 -height 500 -sashrelief raised -sashwidth 4

  # Create frames for pane handle
  frame .bot.left  -relief raised -borderwidth 1
  frame .bot.right -relief raised -borderwidth 1

  ###############################
  # POPULATE RIGHT BOTTOM FRAME #
  ###############################

  # Create the textbox header frame
  frame .bot.right.h
  label .bot.right.h.tl -text "Cur   Line #       Verilog Source" -anchor w
  frame .bot.right.h.pn
  button .bot.right.h.pn.prev -image [image create photo -file [file join $HOME scripts left_arrow.gif]] -state disabled -relief flat -command {
    goto_uncov $prev_uncov_index
  }
  set_balloon .bot.right.h.pn.prev "Click to view the previous uncovered item"
  button .bot.right.h.pn.next -image [image create photo -file [file join $HOME scripts right_arrow.gif]] -state disabled -relief flat -command {
    goto_uncov $next_uncov_index
  }
  set_balloon .bot.right.h.pn.next "Click to view the next uncovered item"
  frame .bot.right.h.search -borderwidth 1 -relief ridge -bg white
  label .bot.right.h.search.find -image [image create photo -file [file join $HOME scripts find.gif]] -bg white -state disabled -relief flat -borderwidth 0
  bind .bot.right.h.search.find <ButtonPress-1> {
    perform_search .bot.right.txt .bot.right.h.search.e .info main_start_search_index
  }
  set_balloon .bot.right.h.search.find "Click to find the next occurrence of the search string"
  entry .bot.right.h.search.e -width 15 -bg white -state disabled -relief flat -insertborderwidth 0 -highlightthickness 0 -disabledbackground white
  bind .bot.right.h.search.e <Return> {
    perform_search .bot.right.txt .bot.right.h.search.e .info main_start_search_index
  }
  label .bot.right.h.search.clear -image [image create photo -file [file join $HOME scripts clear.gif]] -bg white -state disabled -relief flat -borderwidth 0
  bind .bot.right.h.search.clear <ButtonPress-1> {
    .bot.right.txt tag delete search_found
    .bot.right.h.search.e delete 0 end
    set main_start_search_index 1.0
  }
  set_balloon .bot.right.h.search.clear "Click to clear the search string"

  # Pack the previous/next frame
  pack .bot.right.h.pn.prev -side left
  pack .bot.right.h.pn.next -side left

  # Pack the search frame
  pack .bot.right.h.search.find  -side left
  pack .bot.right.h.search.e     -side left -padx 3
  pack .bot.right.h.search.clear -side left

  # Pack the textbox header frame
  pack .bot.right.h.tl     -side left
  pack .bot.right.h.pn     -side left -expand yes
  pack .bot.right.h.search -side right

  # Create the text widget to display the modules/instances
  text      .bot.right.txt -yscrollcommand ".bot.right.vb set" -xscrollcommand ".bot.right.hb set" -wrap none -state disabled
  scrollbar .bot.right.vb -command ".bot.right.txt yview"
  scrollbar .bot.right.hb -orient horizontal -command ".bot.right.txt xview"

  # Pack the right paned window
  grid rowconfigure    .bot.right 1 -weight 1
  grid columnconfigure .bot.right 0 -weight 1
  grid .bot.right.h   -row 0 -column 0 -columnspan 2 -sticky nsew
  grid .bot.right.txt -row 1 -column 0 -sticky nsew
  grid .bot.right.vb  -row 1 -column 1 -sticky ns
  grid .bot.right.hb  -row 2 -column 0 -sticky ew

  ##############################
  # POPULATE LEFT BOTTOM FRAME #
  ##############################

  # Create Tablelist and associated scrollbars
  tablelist::tablelist .bot.left.tl \
    -columns $mod_inst_tl_columns \
    -labelcommand tablelist::sortByColumn -xscrollcommand {.bot.left.hb set} -yscrollcommand {.bot.left.sbf.vb set} -stretch all -movablecolumns 1
  .bot.left.tl columnconfigure 0 -hide [lindex $mod_inst_tl_init_hidden 0]
  .bot.left.tl columnconfigure 1 -hide [lindex $mod_inst_tl_init_hidden 1]
  .bot.left.tl columnconfigure 2 -hide [lindex $mod_inst_tl_init_hidden 2] -sortmode integer -stretchable false
  .bot.left.tl columnconfigure 3 -hide [lindex $mod_inst_tl_init_hidden 3] -sortmode integer -stretchable false
  .bot.left.tl columnconfigure 4 -hide [lindex $mod_inst_tl_init_hidden 4] -sortmode integer -stretchable false
  .bot.left.tl columnconfigure 5 -hide [lindex $mod_inst_tl_init_hidden 5] -sortmode integer -stretchable false
  .bot.left.tl columnconfigure 6 -hide [lindex $mod_inst_tl_init_hidden 6] -sortmode integer -stretchable false
  .bot.left.tl columnconfigure 7 -hide true

  # Create vertical scrollbar frame and pack it
  frame      .bot.left.sbf
  ttk::label .bot.left.sbf.ml -relief flat -style TablelistHeader.TLabel -image [image create bitmap -data "#define stuff_width 16\n#define stuff_height 16\nstatic unsigned char stuff_bits[] = {\n0x00, 0x00, 0x00, 0x00, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x00, 0x00, 0x00, 0x00};"]
  set_balloon .bot.left.sbf.ml "Controls the column hide/show state"

  scrollbar  .bot.left.sbf.vb -command {.bot.left.tl yview}
  ttk::label .bot.left.sbf.l
  pack .bot.left.sbf.ml -side top    -fill x
  pack .bot.left.sbf.vb -side top    -fill y -expand 1
  pack .bot.left.sbf.l  -side bottom -fill x

  scrollbar .bot.left.hb -orient horizontal -command {.bot.left.tl xview}

  grid rowconfigure    .bot.left 0 -weight 1
  grid columnconfigure .bot.left 0 -weight 1
  grid .bot.left.tl  -row 0 -column 0 -sticky news
  grid .bot.left.sbf -row 0 -column 1 -sticky ns -rowspan 2
  grid .bot.left.hb  -row 1 -column 0 -sticky ew

  # Bind the listbox selection event
  bind .bot.left.tl <<ListboxSelect>> populate_text

  # Create and bind the listbox label to a popup menu
  menu .lbm -tearoff false
  manage_tl_popup
  bind .bot.left.sbf.ml <ButtonPress-1> {.lbm post %X %Y}
  bind .lbm <Leave> {.lbm unpost}
  bind .bot.left.tl <<TablelistColumnMoved>> {manage_tl_popup}

  # Pack the bottom window
  update
  .bot add .bot.left -minsize [expr [winfo reqheight .bot.left] + 100]
  if {$mod_inst_tl_width != ""} {
    .bot.left configure -width $mod_inst_tl_width
  }
  .bot add .bot.right

  # Create bottom information bar
  label .info -anchor w -relief raised -borderwidth 1

  # Pack the widgets
  pack .bot  -fill both -expand yes
  pack .info -fill both

  # Set the window icon
  wm title . "Covered - Verilog Code Coverage Tool"

  # If window position variables have been set, use them
  if {$main_geometry != ""} {
    wm geometry . $main_geometry
  }

  # Set focus on the new window
  wm focusmodel . active
  raise .

  # Set icon
  set icon_img [image create photo -file [file join $HOME scripts cov_icon.gif]]
  wm iconphoto . -default $icon_img

  # Catch the closing of the application and potentially save GUI elements
  wm protocol . WM_DELETE_WINDOW {
    check_to_save_and_close_cdd exiting
    save_gui_elements . .
    destroy .
  }
  bind . <Destroy> {
    check_to_save_and_close_cdd exiting
    save_gui_elements . %W
  }
  
}

proc populate_listbox {} {

  global mod_inst_type last_mod_inst_type cdd_name block_list
  global uncov_fgColor uncov_bgColor
  global lb_fgColor lb_bgColor
  global summary_list

  # Make sure that the tablelist columns are setup appropriately
  manage_tl_popup

  # Get the currently loaded indices, if any
  if {$last_mod_inst_type == $mod_inst_type} {
    set curr_indices  [.bot.left.tl getcolumn 7]
    set curr_selected [.bot.left.tl curselection]
  } else {
    set curr_indices  {}
    set curr_selected ""
  }

  # Remove contents currently in listboxes
  .bot.left.tl delete 0 end

  if {$cdd_name != ""} {

    # If we are in module mode, list modules (otherwise, list instances)
    if {$mod_inst_type == "Module"} {

      # Get the list of functional units
      set block_list [tcl_func_get_funit_list]

    } else {

      # Get the list of functional unit instances
      set block_list [tcl_func_get_instance_list]

    }

    # Calculate the summary_list array
    calculate_summary

    for {set i 0} {$i < [llength $summary_list]} {incr i} {
      if {[llength $curr_indices] > 0} {
        set index [lindex $curr_indices $i]
      } else {
        set index $i
      }
      set funit [sort_tl_columns [lindex $summary_list $index]]
      .bot.left.tl insert end [list [lindex $funit 0] [lindex $funit 1] [lindex $funit 2] [lindex $funit 3] [lindex $funit 4] [lindex $funit 5] [lindex $funit 6] $index]
      .bot.left.tl rowconfigure end -background [lindex $funit 8] -selectbackground [lindex $funit 7]
    }

    # Re-activate the currently selected item
    if {$curr_selected != ""} {
      .bot.left.tl selection set $curr_selected
    }

    # Set the last module/instance type variable to the current
    set last_mod_inst_type $mod_inst_type;

  }

}

proc populate_text {} {

  global cov_rb block_list curr_block
  global mod_inst_type last_mod_inst_type
  global last_lb_index
  global start_search_index
  global curr_toggle_ptr

  # Get the index of the current selection
  set index [lindex [.bot.left.tl get [.bot.left.tl curselection]] 7]

  # Update the text, if necessary
  if {$index != ""} {

    if {$last_lb_index != $index || $last_mod_inst_type != $mod_inst_type} {

      set last_lb_index   $index
      set curr_block      [lindex $block_list $index]
      set curr_toggle_ptr ""

      if {$cov_rb == "Line"} {
        process_line_cov
      } elseif {$cov_rb == "Toggle"} {
        process_toggle_cov
      } elseif {$cov_rb == "Memory"} {
        process_memory_cov
      } elseif {$cov_rb == "Logic"} {
        process_comb_cov
      } elseif {$cov_rb == "FSM"} {
        process_fsm_cov
      } elseif {$cov_rb == "Assert"} {
        process_assert_cov
      } else {
        # ERROR
      }

      # Reset starting search index
      set start_search_index 1.0
      set curr_uncov_index   ""

      # Run initial goto_uncov to initialize previous and next pointers
      goto_uncov $curr_uncov_index

      # Enable widgets
      .bot.right.h.search.e     configure -state normal -bg white
      .bot.right.h.search.find  configure -state normal
      .bot.right.h.search.clear configure -state normal

    }

  }

}

proc clear_text {} {

  global last_lb_index

  # Clear the textbox
  .bot.right.txt configure -state normal
  .bot.right.txt delete 1.0 end
  .bot.right.txt configure -state disabled

  # Reset the last_lb_index
  set last_lb_index ""

}

proc perform_search {tbox ebox ibox index} {

  upvar $index start_search_index 

  set value [$ebox get]
  set index [$tbox search $value $start_search_index]

  # Delete search_found tag
  $tbox tag delete search_found

  if {$index != ""} {

    if {$start_search_index > $index} {
      $ibox configure -text "End of file reached.  Searching from the beginning..."
    } else {
      $ibox configure -text ""
    }

    # Highlight found text
    set value_len [string length $value]
    $tbox tag add search_found $index "$index + $value_len chars"
    $tbox tag configure search_found -background orange1

    # Make the text visible
    $tbox see $index 

    # Calculate next starting index
    set indices [split $index .]
    set start_search_index [lindex $indices 0].[expr [lindex $indices 1] + 1]

  } else {

    # Output a message specifying that the searched string could not be found
    $ibox configure -text "String \"$value\" not found"

    # Clear the contents of the search entry box
    $ebox delete 0 end

    # Reset search index
    set start_search_index 1.0

  }

  # Set focus to text box
  focus $tbox

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
  .menubar.view entryconfigure 2 -state disabled

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
  .menubar.view entryconfigure 2 -state normal

  # Set the current pointer to the specified line
  set ptr $line

}

proc goto_uncov {curr_index} {

  global prev_uncov_index next_uncov_index curr_uncov_index
  global cov_rb

  # Calculate the name of the tag to use
  if {$cov_rb == "Line"} {
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
    .bot.right.h.pn.prev configure -state normal
    .menubar.view entryconfigure 1 -state normal
  } else {
    .bot.right.h.pn.prev configure -state disabled
    .menubar.view entryconfigure 1 -state disabled
  }

  # Get next uncovered index
  set next_uncov_index [lindex [.bot.right.txt tag nextrange $tag_name "$curr_uncov_index + 1 chars"] 0]

  # Disable/enable next button
  if {$next_uncov_index != ""} {
    .bot.right.h.pn.next configure -state normal
    .menubar.view entryconfigure 0 -state normal
  } else {
    .bot.right.h.pn.next configure -state disabled
    .menubar.view entryconfigure 0 -state disabled
  }

}

proc update_all_windows {} {

  global curr_uncov_index

  # Update the main window
  goto_uncov $curr_uncov_index
  .menubar.view entryconfigure 2 -state disabled

  # Update the toggle window
  update_toggle

  # Update the memory window
  update_memory

  # Update the combinational logic window
  update_comb

  # Update the FSM window
  update_fsm

  # Update the assertion window
  update_assert

}

proc clear_all_windows {} {

  # Clear the main window
  clear_text

  # Clear the summary window
  clear_summary

  # Clear the toggle window
  clear_toggle

  # Clear the memory window
  clear_memory

  # Clear the combinational logic window
  clear_comb

  # Clear the FSM window
  clear_fsm

  # Clear the assertion window
  clear_assert

}

proc bgerror {msg} {

  global errorInfo

  ;# Remove the status window if it exists
  destroy .status

  ;# Display error message
  set retval [tk_messageBox -message "$msg\n$errorInfo" -title "Error" -type ok]

}

proc listbox_yset {args} {
    
  eval [linsert $args 0 .bot.left.fvb.lvb set]
  listbox_yview moveto [lindex [.bot.left.fvb.lvb get] 0]

} 
  
proc listbox_yview {args} {

  eval [linsert $args 0 .bot.left.pw.ff.l   yview]
  eval [linsert $args 0 .bot.left.pw.fhmt.l yview]
  eval [linsert $args 0 .bot.left.pw.fp.l   yview]

}

proc listbox_xset {args} {

  eval [linsert $args 0 .bot.left.lhb set]
  listbox_xview moveto [lindex [.bot.left.lhb get] 0]

}

proc listbox_xview {args} {

  eval [linsert $args 0 .bot.left.pw.ff.l   xview]
  eval [linsert $args 0 .bot.left.pw.fhmt.l xview]
  eval [linsert $args 0 .bot.left.pw.fp.l   xview]

}

proc goto_next_pane {w} {
  
  set parent [winfo parent $w]
  set panes  [$parent panes]
  
  $parent paneconfigure $w -hide true
  $parent paneconfigure [lindex $panes [expr [lsearch $panes $w] + 1]] -hide false
  
}

proc goto_prev_pane {w} {
  
  set parent [winfo parent $w]
  set panes  [$parent panes]
  
  $parent paneconfigure $w -hide true
  $parent paneconfigure [lindex $panes [expr [lsearch $panes $w] - 1]] -hide false
  
}

proc manage_tl_popup {} {

  global tableColName tableColHide mod_inst_type

  # Calculate starting index
  if {$mod_inst_type == "Module"} {
    set no_display_cols {{Instance Name} Index}
  } else {
    set no_display_cols {Index}
  }

  # Delete all menu entries
  .lbm delete 0 end

  set num 0

  foreach {width name align} [.bot.left.tl cget -columns] {

    if {[lsearch $no_display_cols $name] == -1} {
      .lbm add checkbutton -label $name -variable tableColHide($num) -command {
        foreach col [array names tableColHide] {
          .bot.left.tl columnconfigure $col -hide [expr ! $tableColHide($col)]
        }
      }
    }

    # Handle the instance name column show/hide status
    if {$name eq "Instance Name"} {
      if {$mod_inst_type eq "Module"} {
        .bot.left.tl columnconfigure $num -hide true
      } else {
        .bot.left.tl columnconfigure $num -hide false
      }
    }

    set tableColName($num) $name
    set tableColHide($num) [expr ! [.bot.left.tl columncget $num -hide]]
    incr num

  }

}

proc sort_tl_columns {info} {

  global tableColName

  foreach col [lsort -integer [array names tableColName]] {
    set name $tableColName($col)
    if {$name eq "Instance Name"} {
      lappend new_info [lindex $info 0]
    } elseif {$name eq "Module Name"} {
      lappend new_info [lindex $info 1]
    } elseif {$name eq "Hit"} {
      lappend new_info [lindex $info 2]
    } elseif {$name eq "Miss"} {
      lappend new_info [lindex $info 3]
    } elseif {$name eq "Excluded"} {
      lappend new_info [lindex $info 4]
    } elseif {$name eq "Total"} {
      lappend new_info [lindex $info 5]
    } elseif {$name eq "Hit %"} {
      lappend new_info [lindex $info 6]
    }
  }
  lappend new_info [lindex $info 7] [lindex $info 8]

  return $new_info

}

# Read configuration file
read_coveredrc

# Display main window
main_view

# Display the wizard, if the configuration option is set
if {$show_wizard} {
  create_wizard
}
