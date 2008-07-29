proc create_wizard {} {

  global show_wizard

  # Now create the window and set the grab to this window
  if {[winfo exists .wizwin] == 0} {

    # Create new window
    toplevel .wizwin
    wm title .wizwin "Covered Wizard"

    frame .wizwin.f -relief raised -borderwidth 1 

    # Create buttons
    button .wizwin.f.new   -text "Generate New CDD File" -relief flat -command {
      create_new_cdd
      destroy .wizwin
    }
    button .wizwin.f.open  -text "Open/Merge CDD File(s)" -relief flat -command {
      .menubar.file invoke 0
      destroy .wizwin
    }
    button .wizwin.f.grade -text "Rank CDD Coverage" -relief flat -command {
      create_rank_cdds
      destroy .wizwin
    }
    button .wizwin.f.help  -text "Get Help" -relief flat -state disabled -command {
    }

    # Checkbutton to disable this window
    checkbutton .wizwin.f.show -text "Display this window at startup of Covered" -variable show_wizard -onvalue true -offvalue false -command {
      write_coveredrc
    }

    # Pack the buttons
    grid .wizwin.f.new   -row 0 -column 0 -sticky news -padx 4 -pady 4
    grid .wizwin.f.open  -row 1 -column 0 -sticky news -padx 4 -pady 4
    grid .wizwin.f.grade -row 2 -column 0 -sticky news -padx 4 -pady 4
    grid .wizwin.f.help  -row 3 -column 0 -sticky news -padx 4 -pady 4
    grid .wizwin.f.show  -row 4 -column 0 -sticky news -padx 4 -pady 4

    # Pack the frame
    pack .wizwin.f

  }

  # Make sure that the focus is on this window only
  wm transient .wizwin .

}
