# Contains procs for handling help menu windows

proc help_show_about {} {

  global VERSION HOME

  if {[winfo exists .helpabout]} {

    raise .helpabout

  } else {

    # Create window and populate with widgets
    toplevel .helpabout -background white
    wm title .helpabout "About Covered"

    button .helpabout.b -text "Close" -command {
      destroy .helpabout
    }
    label .helpabout.i -background white -image [image create photo -file [file join $HOME scripts banner.gif]]
    label .helpabout.l -background white -justify left -text "Version:      $VERSION\nAuthor:       Trevor Williams\nEmail:         phase1geo@gmail.com\nHomepage:  http://covered.sourceforge.net\n\nFreely distributable under the GPL license"

    pack .helpabout.i -pady 10
    pack .helpabout.l -padx 10 -pady 10
    pack .helpabout.b -pady 10

    focus -force .helpabout.b

  } 

}

proc help_show_manual {section} {

  global HOME

  # Load the help system
  help::init [file join $HOME doc gui help.help] {} {} 500 800

  # Find the valid section
  if {[lsearch -exact $help::index $section] == -1} {
    set i 1
    while {[expr $i <= 3] && [expr [lsearch -exact $help::index "$section.help?internal_$i"] == -1]} {incr i}
    if {$i > 3} {
      bgerror "Internal error:  Unable to find user manual for section $section"
      set section ""
    } else {
      set section "$section.help?internal_$i"
    }
  }

  # Display the section
  if {$section != ""} {
    help::init [file join $HOME doc gui help.help] $section
  } else {
    help::destroy
  }

}
