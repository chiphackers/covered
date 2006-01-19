#!/usr/bin/env wish

;# Global variables
set line_low_limit   90
set toggle_low_limit 90
set comb_low_limit   90
set fsm_low_limit    90
set summary_sort     "dec"

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

    ;# Create frame to hold grid
    frame .sumwin.f

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
    frame .sumwin.sf
    label .sumwin.sf.l -text "Sort percentages by:"
    radiobutton .sumwin.sf.rbd -text "Decreasing order" -variable summary_sort -value "dec" -command {
      populate_summary .sumwin.f
    }
    radiobutton .sumwin.sf.rbi -text "Increasing order" -variable summary_sort -value "inc" -command {
      populate_summary .sumwin.f
    }
    pack .sumwin.sf.l   -side left
    pack .sumwin.sf.rbd -side left
    pack .sumwin.sf.rbi -side left

    ;# Create button
    button .sumwin.b -text "Close" -command {
      destroy .sumwin
    }

  }

  ;# Update title of window
  if {$cov_rb == "line"} {
    wm title .sumwin "Line Coverage Summary"
  } elseif {$cov_rb == "toggle"} {
    wm title .sumwin "Toggle Coverage Summary"
  } elseif {$cov_rb == "comb"} {
    wm title .sumwin "Combinational Logic Coverage Summary"
  } elseif {$cov_rb == "fsm"} {
    wm title .sumwin "FSM State/Arc Coverage Summary"
  } else {
    ;# ERROR!
  }

  ;# Now populate the summary box
  populate_summary .sumwin.f

  ;# Display the window if we are creating it
  if {$create == 1} {
    pack .sumwin.f -fill both -expand yes
    pack .sumwin.sf
    pack .sumwin.b
  }

  ;# Bring the window to the top
  raise .sumwin

}

proc update_summary {} {

  if {[winfo exists .sumwin] == 1} {
    create_summary
  }

}

proc populate_summary { w } {

  global mod_inst_type funit_names funit_types cov_rb
  global line_summary_hit line_summary_total line_low_limit
  global toggle_summary_hit toggle_summary_total toggle_low_limit
  global comb_summary_hit comb_summary_total comb_low_limit
  global fsm_low_limit
  global summary_sort

  for {set i 0} {$i < [llength $funit_names]} {incr i} {

    ;# Get summary information for the current type
    if {$cov_rb == "line"} {
      tcl_func_get_line_summary [lindex $funit_names $i] [lindex $funit_types $i]
      set hit       $line_summary_hit
      set total     $line_summary_total
      set low_limit $line_low_limit
    } elseif {$cov_rb == "toggle"} {
      tcl_func_get_toggle_summary [lindex $funit_names $i] [lindex $funit_types $i]
      set hit       $toggle_summary_hit
      set total     $toggle_summary_total
      set low_limit $toggle_low_limit
    } elseif {$cov_rb == "comb"} {
      tcl_func_get_comb_summary [lindex $funit_names $i] [lindex $funit_types $i]
      set hit       $comb_summary_hit
      set total     $comb_summary_total
      set low_limit $comb_low_limit
    } elseif {$cov_rb == "fsm"} {
      set percent 0
      set low_limit $fsm_low_limit
    } else {
      ;# ERROR
    }

    ;# Calculate hit percent
    if {$total == 0} {
      set percent 100
    } else {
      set percent [expr round((($hit * 1.0) / $total) * 100)]
    }

    ;# Add this functional unit to the list to sort
    lappend summary_list [list [lindex $funit_names $i] $percent]

  }

  ;# Sort the summary information based on the percent value
  if {$summary_sort == "dec"} {
    set summary_list [lsort -integer -index 1 -decreasing $summary_list]
  } else {
    set summary_list [lsort -integer -index 1 -increasing $summary_list]
  }

  ;# Remove all values from listboxes
  .sumwin.f.flb delete 0 end
  .sumwin.f.plb delete 0 end

  ;# Add the summary information from the sorted list
  for {set i 0} {$i < [llength $summary_list]} {incr i} {

    ;# Add the summary information to the summary box
    add_func_unit $w [lindex [lindex $summary_list $i] 0] [lindex [lindex $summary_list $i] 1] $low_limit $i

  }

}

proc add_func_unit { w name percent low_limit num } {

  ;# Calculate color
  if {$percent < $low_limit} {
    set color red
  } elseif {$percent < 100} {
    set color yellow
  } else {
    set color green
  }

  ;# Populate name and percent listboxes
  .sumwin.f.flb insert end $name
  .sumwin.f.plb insert end "$percent%"

  ;# Modify the coverage colors of the name box (make selection look like it doesn't exist)
  .sumwin.f.flb itemconfigure end -foreground black -background $color -selectforeground black -selectbackground $color

}

proc select_main_lb {} {

  global funit_names funit_types
  global curr_funit_name curr_funit_type

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
