proc create_comb_window {mod_name expr_id} {

  # Now create the window and set the grab to this window
  if {[winfo exists .combwin] == 0} {

    # Create new window
    toplevel .combwin
    wm title .combwin "Combinational Logic Coverage - Verbose"

    frame .combwin.f

    # Add toggle information
    label .combwin.f.l0 -anchor w -text "Expression:"
    text  .combwin.f.t -height 20 -width 100 -xscrollcommand ".combwin.f.hb set" -wrap none

    scrollbar .combwin.f.hb -orient horizontal -command ".combwin.f.t xview"

    # Create bottom information bar
    label .combwin.f.info -anchor w

    # Pack the widgets into the bottom frame
    # grid rowconfigure    .combwin.f 1 -weight 1
    # grid columnconfigure .combwin.f 0 -weight 0
    # grid columnconfigure .combwin.f 1 -weight 1
    grid .combwin.f.l0   -row 0 -column 0 -sticky nsew
    grid .combwin.f.t    -row 1 -column 0 -sticky nsew
    grid .combwin.f.hb   -row 2 -column 0 -sticky ew
    grid .combwin.f.info -row 3 -column 0 -sticky ew

    pack .combwin.f

  }

}
