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

    # Add toggle 0->1 information
    label .togwin.f.l_01 -anchor w -text "Toggle 0->1"
    text  .togwin.f.t_01 -height 1 -width 32 -bg white -xscrollcommand ".togwin.f.hb set" -wrap none

    # Add toggle 1->0 information
    label .togwin.f.l_10 -anchor w -text "Toggle 1->0"
    text  .togwin.f.t_10 -height 1 -width 32 -bg white -xscrollcommand ".togwin.f.hb set" -wrap none

    scrollbar .togwin.f.hb -orient horizontal -command ".togwin.f.t_01 xview"

    # Create bottom information bar
    label .togwin.f.info -anchor e

    # Pack the widgets into the bottom frame
    grid rowconfigure    .togwin.f 1 -weight 1
    grid columnconfigure .togwin.f 0 -weight 0
    grid columnconfigure .togwin.f 1 -weight 1
    grid .togwin.f.t_01   -row 0 -column 0 -sticky nsew
    grid .togwin.f.l_01   -row 0 -column 1 -sticky nsew
    grid .togwin.f.t_10   -row 1 -column 0 -sticky nsew
    grid .togwin.f.l_10   -row 1 -column 1 -sticky nsew
    grid .togwin.f.hb     -row 2 -column 0 -sticky ew
    grid .togwin.f.info   -row 3 -column 0 -sticky ew

    pack .togwin.f

  }

  # Get verbose toggle information
  set toggle01_verbose 0
  set toggle10_verbose 0
  set toggle_msb       0
  set toggle_lsb       0
  tcl_func_get_toggle_coverage $mod_name $signal

  # Allow us to clear out text boxes
  .togwin.f.t_01 configure -state normal
  .togwin.f.t_10 configure -state normal

  # Remove any tags associated with the toggle text boxes
  .togwin.f.t_01 tag delete tog01_motion
  .togwin.f.t_10 tag delete tog10_motion

  # Clear the text-box before any insertion is being made
  .togwin.f.t_01 delete 1.0 end
  .togwin.f.t_10 delete 1.0 end

  # Write toggle 0->1 and 1->0 information in text box
  .togwin.f.t_01 insert end $toggle01_verbose
  .togwin.f.t_10 insert end $toggle10_verbose

  # Keep user from writing in text boxes
  #.togwin.f.t_01 configure -state disabled
  #.togwin.f.t_10 configure -state disabled

  # Create leave bindings for textboxes
  bind .togwin.f.t_01 <Leave> {
    .togwin.f.info configure -text "$sig_name\[$toggle_msb:$toggle_lsb\]"
  }
  bind .togwin.f.t_10 <Leave> {
    .togwin.f.info configure -text "$sig_name\[$toggle_msb:$toggle_lsb\]"
  }

  # Add toggle tags and bindings
  .togwin.f.t_01 tag add  tog01_motion 1.0 {end - 1 chars}
  #.togwin.f.t_01 tag configure tog01_motion -justify right
  .togwin.f.t_01 tag bind tog01_motion <Motion> {
    set tmp [lindex [split [.togwin.f.t_01 index current] "."] 1]
    set tmp [expr $toggle_msb - $tmp]
    .togwin.f.info configure -text "$sig_name\[$tmp\]"
  }

  .togwin.f.t_10 tag add  tog10_motion 1.0 {end - 1 chars}
  #.togwin.f.t_10 tag configure tog10_motion -justify right
  .togwin.f.t_10 tag bind tog10_motion <Motion> {
    set tmp [lindex [split [.togwin.f.t_10 index current] "."] 1]
    set tmp [expr $toggle_msb - $tmp]
    .togwin.f.info configure -text "$sig_name\[$tmp\]"
  }

  .togwin.f.info configure -text "$sig_name\[$toggle_msb:$toggle_lsb\]"

}
