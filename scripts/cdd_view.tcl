set cdd_files [list]

proc add_cdd_to_filelist {name clear} {

  global cdd_files

  # If we have been told to clear the list, do so now
  if {$clear != 0} {
    set cdd_files [list]
  }

  # Now set the CDD file name
  lappend cdd_files $name

  # Update the CDD file viewer window for these changes
  update_cdd_viewer

}

proc create_cdd_viewer {} {

  global cdd_files

  set create 0

  # Now create the window and set the grab to this window
  if {[winfo exists .cddwin] == 0} {

    set create 1

    # Create new window
    toplevel .cddwin
    wm title .cddwin "CDD File Viewer"

    # Create frame to hold listbox
    frame .cddwin.f -relief raised -borderwidth 1

    # Create listbox/scrollbar that will contain the opened/merged CDD file names
    label .cddwin.f.l -text "Opened/Merged CDD Files"
    listbox .cddwin.f.lb -yscrollcommand ".cddwin.f.vb set" -xscrollcommand ".cddwin.f.hb set" -relief flat -width 30 -height 20 -selectborderwidth 0
    .cddwin.f.lb configure -selectbackground [.cddwin.f.lb cget -background]
    .cddwin.f.lb configure -selectforeground [.cddwin.f.lb cget -foreground]
    scrollbar .cddwin.f.vb -command ".cddwin.f.lb yview"
    scrollbar .cddwin.f.hb -orient horizontal -command ".cddwin.f.lb xview"

    # Pack the widgets into the bottom frame
    grid rowconfigure    .cddwin.f 0 -weight 0
    grid rowconfigure    .cddwin.f 1 -weight 1
    grid rowconfigure    .cddwin.f 2 -weight 0
    grid columnconfigure .cddwin.f 0 -weight 1
    grid columnconfigure .cddwin.f 1 -weight 0
    grid .cddwin.f.l  -row 0 -column 0 -sticky news
    grid .cddwin.f.lb -row 1 -column 0 -sticky news
    grid .cddwin.f.vb -row 1 -column 1 -sticky news
    grid .cddwin.f.hb -row 2 -column 0 -sticky news
    
    ########################
    # Create Close buttons #
    ########################

    frame .cddwin.bf -relief raised -borderwidth 1

    button .cddwin.bf.close -width 10 -text "Close" -command {
      destroy .cddwin
    }

    pack .cddwin.bf.close -side right -padx 8 -pady 4

  }

  # Populate listbox with CDD filenames
  .cddwin.f.lb delete 0 end
  foreach name $cdd_files {
    .cddwin.f.lb insert end $name
  }

  # Display the window if we are creating it
  if {$create == 1} {
    pack .cddwin.f  -fill both -expand yes
    pack .cddwin.bf -fill both
  }

  ;# Bring the window to the top
  raise .cddwin

}

proc update_cdd_viewer {} {

  if {[winfo exists .cddwin] == 1} {
    create_cdd_viewer
  }

}

