proc create_wizard {} {

  global show_wizard HOME

  # Now create the window and set the grab to this window
  if {[winfo exists .wizwin] == 0} {

    # Create new window
    toplevel .wizwin
    wm title .wizwin "Covered Wizard"

    frame .wizwin.f -relief raised -borderwidth 1 

    # Add banner
    label .wizwin.f.banner -image [image create photo -file [file join $HOME scripts banner.gif]]

    # Create buttons
    button .wizwin.f.new   -text "Generate New CDD File" -relief flat -command {
      destroy .wizwin
      create_new_cdd
    }
    button .wizwin.f.open  -text "Open/Merge CDD File(s)" -relief flat -command {
      destroy .wizwin
      .menubar.file invoke 0
    }
    button .wizwin.f.grade -text "Rank CDD Coverage" -relief flat -command {
      destroy .wizwin
      create_rank_cdds
    }
    button .wizwin.f.help  -text "View Help Manual" -relief flat -command {
      help_show_manual part.gui 
      destroy .wizwin
    }

    # Checkbutton to disable this window
    checkbutton .wizwin.f.show -text "Display this window at startup of Covered" -variable show_wizard -onvalue true -offvalue false -command {
      write_coveredrc 0
    }

    help_button .wizwin.f.hb chapter.gui.wizard 

    # Pack the buttons
    grid .wizwin.f.banner -row 0 -rowspan 4 -column 0 -sticky news -padx 4 -pady 4
    grid .wizwin.f.new    -row 0 -column 1 -sticky news -padx 4 -pady 4
    grid .wizwin.f.open   -row 1 -column 1 -sticky news -padx 4 -pady 4
    grid .wizwin.f.grade  -row 2 -column 1 -sticky news -padx 4 -pady 4
    grid .wizwin.f.help   -row 3 -column 1 -sticky news -padx 4 -pady 4
    grid .wizwin.f.show   -row 4 -column 0 -sticky news -padx 4 -pady 4
    grid .wizwin.f.hb     -row 4 -column 1 -sticky e -pady 4

    # Pack the frame
    pack .wizwin.f -fill both -expand yes

  }

  # Make sure that the focus is on this window only
  wm transient .wizwin .

}
