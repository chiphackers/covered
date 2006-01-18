set cov_rb line

proc create_summary {} {

  global cov_rb

  # Now create the window and set the grab to this window
  if {[winfo exists .sumwin] == 0} {

    # Create new window
    toplevel .sumwin
    wm title .sumwin "Design Coverage Summary"
    wm resizable .sumwin 0 0

    frame .sumwin.f

    ;# Create first row labels
    label .sumwin.f.fl -anchor w -text "Functional Block"
    label .sumwin.f.pl -anchor w -text "Percent Covered"
    label .sumwin.f.sl -anchor w -text "(Red = <90%, Yellow = <100%, Green = 100%)"

    ;# Create the canvas
    canvas .sumwin.f.c -yscrollcommand ".sumwin.f.vb set"
    scrollbar .sumwin.f.vb -command ".sumwin.f.c yview"

    grid rowconfigure    .sumwin.f.c 1 -weight 0
    grid columnconfigure .sumwin.f.c 0 -weight 1
    grid columnconfigure .sumwin.f.c 1 -weight 1
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

  ;# Get summary information for the current type
  if {$cov_rb == "line"} {
    ;# Get line coverage summary information
  } elseif {$cov_rb == "toggle"} {
    ;# Get toggle coverage summary information
  } elseif {$cov_rb == "comb"} {
    ;# Get combinational coverage summary information
  } elseif {$cov_rb == "fsm"} {
    ;# Get FSM coverage summary information
  } else {
    ;# ERROR
  }

  add_func_unit foo 50 1
  add_func_unit bar 91 2
  add_func_unit foobar 100 3

}

proc add_func_unit { name percent num } {

  append label_funit   .sumwin.f.c.fname $num
  append label_percent .sumwin.f.c.pname $num
  append scale_name    .sumwin.f.c.sname $num

  ;# Calculate color
  if {$percent < 90} {
    set color red
  } elseif {$percent < 100} {
    set color yellow
  } else {
    set color green
  }

  ;# Create module name label
  label $label_funit -text "$name" -anchor w

  ;# Create percent label
  label $label_percent -text "$percent%" -anchor w

  ;# Create scale
  scale $scale_name -bg $color -bd 0 -from 0 -to 100 -orient horizontal -sliderrelief flat -sliderlength $percent -troughcolor #AAAAAA -state normal -showvalue 0 -state disabled

  ;# Pack it in the window manager
  grid $label_funit   -row $num -column 0 -sticky ew
  grid $label_percent -row $num -column 1 -sticky ew
  grid $scale_name    -row $num -column 2 -sticky w

}

create_summary

