set sig_name ""

proc create_toggle_window {mod_name signal} {

  global toggle01_verbose toggle10_verbose toggle_msb toggle_lsb
  global sig_name

  set sig_name $signal

  # Now create the window and set the grab to this window
  if {[winfo exists .togwin] == 0} {

    # Create new window
    toplevel .togwin
    wm title .togwin "Toggle Coverage - Verbose"

    frame .togwin.f

    # Add toggle information
    label .togwin.f.l_01 -anchor w -text "Toggle 0->1"
    label .togwin.f.l_10 -anchor w -text "Toggle 1->0"
    text  .togwin.f.t -height 2 -width 32 -xscrollcommand ".togwin.f.hb set" -wrap none -spacing1 2 -spacing3 3

    scrollbar .togwin.f.hb -orient horizontal -command ".togwin.f.t xview"

    # Create bottom information bar
    label .togwin.f.info -anchor e

    # Pack the widgets into the bottom frame
    grid rowconfigure    .togwin.f 1 -weight 1
    grid columnconfigure .togwin.f 0 -weight 0
    grid columnconfigure .togwin.f 1 -weight 1
    grid .togwin.f.t    -row 0 -rowspan 2 -column 0 -sticky nsew
    grid .togwin.f.l_01 -row 0 -column 1 -sticky nsew
    grid .togwin.f.l_10 -row 1 -column 1 -sticky nsew
    grid .togwin.f.hb   -row 2 -column 0 -sticky ew
    grid .togwin.f.info -row 3 -column 0 -sticky ew

    pack .togwin.f

  }

  # Get verbose toggle information
  set toggle01_verbose 0
  set toggle10_verbose 0
  set toggle_msb       0
  set toggle_lsb       0
  tcl_func_get_toggle_coverage $mod_name $signal

  # Allow us to clear out text box
  .togwin.f.t configure -state normal

  # Remove any tags associated with the toggle text box
  .togwin.f.t tag delete tog_motion

  # Clear the text-box before any insertion is being made
  .togwin.f.t delete 1.0 end

  # Write toggle 0->1 and 1->0 information in text box
  .togwin.f.t insert end "$toggle01_verbose\n$toggle10_verbose" 

  # Keep user from writing in text boxes
  .togwin.f.t configure -state disabled

  # Create leave bindings for textboxes
  bind .togwin.f.t <Leave> {
    .togwin.f.info configure -text "$sig_name\[$toggle_msb:$toggle_lsb\]"
  }

  # Add toggle tags and bindings
  .togwin.f.t tag add  tog_motion 1.0 {end - 1 chars}
  .togwin.f.t tag bind tog_motion <Motion> {
    set tmp [lindex [split [.togwin.f.t index current] "."] 1]
    set tmp [expr $toggle_msb - $tmp]
    .togwin.f.info configure -text "$sig_name\[$tmp\]"
  }

  # Right justify the toggle information
  if {[expr [expr $toggle_msb - $toggle_lsb] + 1] <= 32} { 
    .togwin.f.t tag configure tog_motion -justify right
  } else {
    .togwin.f.t tag configure tog_motion -justify left
    .togwin.f.t xview moveto 1.0
  }

  .togwin.f.info configure -text "$sig_name\[$toggle_msb:$toggle_lsb\]"

}
