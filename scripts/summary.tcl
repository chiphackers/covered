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

;# Global variables
set line_low_limit   90
set toggle_low_limit 90
set memory_low_limit 90
set comb_low_limit   90
set fsm_low_limit    90
set assert_low_limit 90
set summary_sort     "none"

proc summary_yset {args} {

  eval [linsert $args 0 .sumwin.f.vb set]
  summary_yview moveto [lindex [.sumwin.f.vb get] 0]

}

proc summary_yview {args} {

  eval [linsert $args 0 .sumwin.f.flb yview]
  eval [linsert $args 0 .sumwin.f.plb yview]

}

proc create_summary {} {

  global cov_rb

  set create 0

  # Now create the window and set the grab to this window
  if {[winfo exists .sumwin] == 0} {

    set create 1

    ;# Create new window
    toplevel .sumwin

    ;# Create frame to hold metric menubutton and the menubutton itself
    frame .sumwin.mf -relief raised -borderwidth 1
    tk_optionMenu .sumwin.mf.metrics cov_rb Line Toggle Memory Logic FSM Assert
    pack .sumwin.mf.metrics -side left

    ;# Create frame to hold grid
    frame .sumwin.f -relief raised -borderwidth 1

    ;# Create first row labels
    label .sumwin.f.fl -anchor w -text "Functional Block" -width 30
    label .sumwin.f.pl -anchor w -text "% Covered" -width 10

    ;# Create the listboxes
    listbox .sumwin.f.flb -yscrollcommand summary_yset -relief flat -width 30 -height 20 -selectborderwidth 0
    listbox .sumwin.f.plb -yscrollcommand summary_yset -relief flat -width 10 -height 20 -selectborderwidth 0
    .sumwin.f.plb configure -selectbackground [.sumwin.f.plb cget -background]
    .sumwin.f.plb configure -selectforeground [.sumwin.f.plb cget -foreground]
    scrollbar .sumwin.f.vb -command summary_yview

    ;# Specify listbox select command
    bind .sumwin.f.flb <<ListboxSelect>> select_main_lb

    ;# Pack the widgets into the bottom frame
    grid rowconfigure    .sumwin.f 0 -weight 0
    grid rowconfigure    .sumwin.f 1 -weight 1
    grid columnconfigure .sumwin.f 0 -weight 1
    grid columnconfigure .sumwin.f 1 -weight 0
    grid columnconfigure .sumwin.f 2 -weight 0
    grid .sumwin.f.fl  -row 0 -column 0 -sticky news
    grid .sumwin.f.pl  -row 0 -column 1 -sticky news
    grid .sumwin.f.flb -row 1 -column 0 -sticky news
    grid .sumwin.f.plb -row 1 -column 1 -sticky news
    grid .sumwin.f.vb  -row 1 -column 2 -sticky news

    ;# Create sort order label and radio buttons and pack them
    frame .sumwin.sf -relief raised -borderwidth 1
    label .sumwin.sf.l -text "Sort percentages by:"
    ;# radiobutton .sumwin.sf.rbd -text "Decreasing order" -variable summary_sort -value "dec" -command {
    ;#   populate_summary .sumwin.f
    ;# }
    ;# radiobutton .sumwin.sf.rbi -text "Increasing order" -variable summary_sort -value "inc" -command {
    ;#   populate_summary .sumwin.f
    ;# }
    pack .sumwin.sf.l   -side left
    ;# pack .sumwin.sf.rbd -side left
    ;# pack .sumwin.sf.rbi -side left

    #############################
    # Create Close/Help buttons #
    #############################

    frame .sumwin.bf -relief raised -borderwidth 1

    button .sumwin.bf.close -width 10 -text "Close" -command {
      destroy .sumwin
    }

    button .sumwin.bf.help -width 10 -text "Help" -command {
      help_show_manual "" ""
    }

    pack .sumwin.bf.help  -side right -padx 8 -pady 4
    pack .sumwin.bf.close -side right -padx 8 -pady 4

  }

  ;# Update title of window
  if {$cov_rb == "Line"} {
    wm title .sumwin "Line Coverage Summary"
  } elseif {$cov_rb == "Toggle"} {
    wm title .sumwin "Toggle Coverage Summary"
  } elseif {$cov_rb == "Memory"} {
    wm title .sumwin "Memory Coverage Summary"
  } elseif {$cov_rb == "Logic"} {
    wm title .sumwin "Combinational Logic Coverage Summary"
  } elseif {$cov_rb == "FSM"} {
    wm title .sumwin "FSM State/Arc Coverage Summary"
  } elseif {$cov_rb == "Assert"} {
    wm title .sumwin "Assertion Coverage Summary"
  } else {
    ;# ERROR!
  }

  ;# Now populate the summary box
  populate_summary .sumwin.f

  ;# Display the window if we are creating it
  if {$create == 1} {
    pack .sumwin.mf -fill both
    pack .sumwin.f  -fill both -expand yes
    pack .sumwin.sf -fill both
    pack .sumwin.bf -fill both
  }

  ;# Bring the window to the top
  raise .sumwin

}

proc clear_summary {} {

  destroy .sumwin

}

proc calculate_summary {} {

  global mod_inst_type block_list cov_rb
  global line_summary_hit line_summary_excluded line_summary_total line_low_limit
  global toggle_summary_hit toggle_summary_excluded toggle_summary_total toggle_low_limit
  global memory_summary_hit memory_summary_excluded memory_summary_total memory_low_limit
  global comb_summary_hit comb_summary_excluded comb_summary_total comb_low_limit
  global fsm_summary_hit fsm_summary_excluded fsm_summary_total fsm_low_limit
  global assert_summary_hit assert_summary_excluded assert_summary_total assert_low_limit
  global summary_list summary_sort

  ;# Clear the list
  set summary_list ""

  foreach block $block_list {

    ;# Get summary information for the current type
    if {$cov_rb == "Line"} {
      tcl_func_get_line_summary $block
      set hit       $line_summary_hit
      set excluded  $line_summary_excluded
      set total     $line_summary_total
      set low_limit $line_low_limit
    } elseif {$cov_rb == "Toggle"} {
      tcl_func_get_toggle_summary $block
      set hit       $toggle_summary_hit
      set excluded  $toggle_summary_excluded
      set total     $toggle_summary_total
      set low_limit $toggle_low_limit
    } elseif {$cov_rb == "Memory"} {
      tcl_func_get_memory_summary $block
      set hit       $memory_summary_hit
      set excluded  $memory_summary_excluded
      set total     $memory_summary_total
      set low_limit $memory_low_limit
    } elseif {$cov_rb == "Logic"} {
      tcl_func_get_comb_summary $block
      set hit       $comb_summary_hit
      set excluded  $comb_summary_excluded
      set total     $comb_summary_total
      set low_limit $comb_low_limit
    } elseif {$cov_rb == "FSM"} {
      tcl_func_get_fsm_summary $block
      set hit       $fsm_summary_hit
      set excluded  $fsm_summary_excluded
      set total     $fsm_summary_total
      set low_limit $fsm_low_limit
    } elseif {$cov_rb == "Assert"} {
      tcl_func_get_assert_summary $block
      set hit       $assert_summary_hit
      set excluded  $assert_summary_excluded
      set total     $assert_summary_total
      set low_limit $assert_low_limit
    } else {
      ;# ERROR
    }

    ;# Calculate miss value
    set miss [expr $total - $hit]

    ;# Calculate hit percent
    if {$total <= 0} {
      set percent 100
    } else {
      set percent [expr round((($hit * 1.0) / $total) * 100)]
    }

    ;# Calculate color
    if {$percent < $low_limit} {
      set bcolor "#c00000"
      set lcolor "#ff7088"
    } elseif {$percent < 100} {
      set bcolor "#e9e500"
      set lcolor "#ffff80"
    } else {
      set bcolor "#006400"
      set lcolor "#90ee90"
    }

    # If the total was less than 0, make sure that we set miss, total and percent values to ?
    if {$total < 0} {
      set miss    " ? "
      set total   " ? "
      set percent " ? "
    }

    ;# Add this functional unit to the list to sort
    if {$mod_inst_type == "Module"} {
      lappend summary_list [list "" [tcl_func_get_funit_name $block] $hit $miss $excluded $total $percent $bcolor $lcolor]
    } else {
      lappend summary_list [list [tcl_func_get_inst_scope $block] [tcl_func_get_funit_name $block] $hit $miss $excluded $total $percent $bcolor $lcolor]
    }

  }

  ;# Sort the summary information based on the percent value
  if {$summary_sort == "dec"} {
    set summary_list [lsort -integer -index 2 -decreasing $summary_list]
  } elseif {$summary_sort == "inc"} {
    set summary_list [lsort -integer -index 2 -increasing $summary_list]
  }

}

proc populate_summary { w } {

  global summary_list

  ;# Calculate summary array
  calculate_summary

  ;# Remove all values from listboxes
  .sumwin.f.flb delete 0 end
  .sumwin.f.plb delete 0 end

  ;# Add the summary information from the sorted list
  for {set i 0} {$i < [llength $summary_list]} {incr i} {

    ;# Add the summary information to the summary box
    add_func_unit $w [lindex [lindex $summary_list $i] 0] [lindex [lindex $summary_list $i] 4] [lindex [lindex $summary_list $i] 6] $i

  }

}

proc add_func_unit { w name percent color num } {

  ;# Populate name and percent listboxes
  .sumwin.f.flb insert end $name
  .sumwin.f.plb insert end "$percent%"

  ;# Modify the coverage colors of the name box (make selection look like it doesn't exist)
  .sumwin.f.flb itemconfigure end -foreground black -background $color -selectforeground black -selectbackground $color

}

proc select_main_lb {} {

  global funit_names funit_types

  ;# Get the selected name
  set sel_name [.sumwin.f.flb get [.sumwin.f.flb curselection]]

  ;# Get the index of the currently selected name
  set index [lsearch $funit_names $sel_name]

  ;# Clear current selection in main window listbox and select new
  .bot.left.l selection clear 0 end
  .bot.left.l selection set $index

  ;# Populate the text box
  populate_text

}
