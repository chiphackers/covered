# Contains procs for handling help menu windows

set covered_version "20040820"
set BROWSER         ""

proc help_show_about {} {

  global covered_version HOME

  if {[winfo exists .helpabout]} {

    raise .helpabout

  } else {

    # Create window and populate with widgets
    toplevel .helpabout -background white
    wm title .helpabout "About Covered"

    label .helpabout.i -background white -image [image create photo -file "$HOME/scripts/banner.gif"]
    label .helpabout.l -background white -justify center -text "Covered $covered_version\n\nAuthor:  Trevor Williams\nEmail:  trevorw@charter.net\nHomepage:  http://covered.sourceforge.net\n\nFreely distributable under the GPL license"
    button .helpabout.b -text "Dismiss" -command {
      destroy .helpabout
    }

    pack .helpabout.i -pady 10
    pack .helpabout.l -padx 10 -pady 10
    pack .helpabout.b -pady 10

  } 

}
