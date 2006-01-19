#!/usr/bin/env wish

proc create_summary {} {

  global cov_rb

  # Now create the window and set the grab to this window
  if {[winfo exists .sumwin] == 0} {

    # Create new window
    toplevel .sumwin
    wm resizable .sumwin 0 0

    frame .sumwin.f

    ;# Create first row labels
    label .sumwin.f.fl -anchor w -text "Functional Block" -width 20
    label .sumwin.f.pl -anchor w -text "Percent Covered" -width 15
    label .sumwin.f.sl -anchor w -text "(Red = <90%, Yellow = <100%, Green = 100%)"

    ;# Create the canvas
    canvas .sumwin.f.c -yscrollcommand ".sumwin.f.vb set"
    scrollbar .sumwin.f.vb -command ".sumwin.f.c yview"

    grid rowconfigure    .sumwin.f.c 1 -weight 0
    grid columnconfigure .sumwin.f.c 0 -weight 0
    grid columnconfigure .sumwin.f.c 1 -weight 0
    grid columnconfigure .sumwin.f.c 2 -weight 1

    ;# Pack the widgets into the bottom frame
    grid rowconfigure    .sumwin.f 1 -weight 0
    grid columnconfigure .sumwin.f 0 -weight 0
    grid columnconfigure .sumwin.f 1 -weight 0
    grid columnconfigure .sumwin.f 2 -weight 0
    grid .sumwin.f.fl -row 0 -column 0     -sticky news
    grid .sumwin.f.pl -row 0 -column 1     -sticky news
    grid .sumwin.f.sl -row 0 -column 2     -sticky news
    grid .sumwin.f.c  -row 1 -columnspan 3 -sticky news
    grid .sumwin.f.vb -row 1 -column 4     -sticky news

    button .sumwin.b -text "OK" -command {
      destroy .sumwin
    }

    pack .sumwin.f -anchor nw
    pack .sumwin.b -anchor s

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
  populate_summary .sumwin.f.c

}

proc update_summary {} {

  if {[winfo exists .sumwin] == 1} {
    create_summary
  }

}

proc populate_summary { w } {

  global mod_inst_type funit_names funit_types cov_rb
  global line_summary_hit line_summary_total
  global toggle_summary_hit toggle_summary_total
  global comb_summary_hit comb_summary_total

  for {set i 0} {$i < [llength $funit_names]} {incr i} {

    ;# Get summary information for the current type
    if {$cov_rb == "line"} {
      tcl_func_get_line_summary [lindex $funit_names $i] [lindex $funit_types $i]
      set hit   $line_summary_hit
      set total $line_summary_total
    } elseif {$cov_rb == "toggle"} {
      tcl_func_get_toggle_summary [lindex $funit_names $i] [lindex $funit_types $i]
      set hit   $toggle_summary_hit
      set total $toggle_summary_total
    } elseif {$cov_rb == "comb"} {
      tcl_func_get_comb_summary [lindex $funit_names $i] [lindex $funit_types $i]
      set hit   $comb_summary_hit
      set total $comb_summary_total
    } elseif {$cov_rb == "fsm"} {
      set percent 0
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
  lsort -integer -index 1 $summary_list

  ;# Add the summary information from the sorted list
  for {set i 0} {$i < [llength $summary_list]} {incr i} {

    ;# Add the summary information to the summary box
    add_func_unit $w [lindex [lindex $summary_list $i] 0] [lindex [lindex $summary_list $i] 1] $i

  }

}

proc add_func_unit { w name percent num } {

  append text_funit    $w .fname $num
  append label_percent $w .pname $num
  append scale_name    $w .sname $num

  ;# Calculate color
  if {$percent < 90} {
    set color red
  } elseif {$percent < 100} {
    set color yellow
  } else {
    set color green
  }

  ;# Figure out if this is a create or update
  set update [winfo exists $label_percent]

  ;# Create/update percent label
  if {$update == 0} {
    label $label_percent -text "$percent%" -width 15
  } else {
    $label_percent configure -text "$percent%"
  }

  ;# Create/update module name label
  if {$update == 0} {
    text $text_funit -bd 0 -height 1 -width 20 -font [$label_percent cget -font] -relief flat -selectborderwidth 0
  } else {
    $text_funit configure -state normal
    $text_funit delete 1.0 end
  }
  $text_funit insert end $name
  $text_funit configure -state disabled

  ;# Create/update scale
  if {$update == 0} {
    scale $scale_name -bg $color -bd 0 -from 0 -to 100 -orient horizontal -sliderrelief flat -troughcolor #AAAAAA -showvalue 0 -state disabled
  } else {
    $scale_name configure -bg $color
  }

  ;# Pack it in the window manager
  if {$update == 0} {
    grid $text_funit    -row $num -column 0 -sticky ew
    grid $label_percent -row $num -column 1 -sticky ew
    grid $scale_name    -row $num -column 2 -sticky ew
  }

  ;# Set scale percentage
  update
  set w [winfo width $scale_name]
  set p [expr {($percent * 1.0) / 100}]
  $scale_name configure -sliderlength [expr $w * $p]

}
