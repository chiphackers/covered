# Contains procs for handling help menu windows

proc help_show_about {} {

  global VERSION HOME

  if {[winfo exists .helpabout]} {

    raise .helpabout

  } else {

    # Create window and populate with widgets
    toplevel .helpabout -background white
    wm title .helpabout "About Covered"

    label .helpabout.i -background white -image [image create photo -file "$HOME/scripts/banner.gif"]
    label .helpabout.l -background white -justify left -text "Version:      $VERSION\nAuthor:       Trevor Williams\nEmail:         trevorw@charter.net\nHomepage:  http://covered.sourceforge.net\n\nFreely distributable under the GPL license"
    button .helpabout.b -text "Close" -command {
      destroy .helpabout
    }

    pack .helpabout.i -pady 10
    pack .helpabout.l -padx 10 -pady 10
    pack .helpabout.b -pady 10

  } 

}

proc help_show_manual {section} {

  global HOME BROWSER

  set fpath [file join $HOME doc $section].html

  if {[catch {exec $BROWSER -remote $fpath}]} {

    # perhaps browser doesn't understand -remote flag
    if {[catch "exec $BROWSER \"$fpath\" &" emsg]} {
      error "Error displaying $fname in browser\n$emsg"
    }

  }

}
