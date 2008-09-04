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

proc help_show_manual {chapter {section ""}} {

  global HOME BROWSER

  set fpath "file://[file join $HOME doc html $chapter].html"

  if {$section != ""} {
    set fpath $fpath#$section
  }

  if {[catch {exec $BROWSER -remote "openURL( $fpath )"}]} {

    # perhaps browser doesn't understand -remote flag
    if {[catch "exec $BROWSER \"$fpath\" &" emsg]} {
      error "Error displaying $fname in browser\n$emsg"
    }

  }

}

proc help_button {w file {section ""}} {

  set help_img [image create bitmap -data "#define help2_width 22\n#define help2_height 22\nstatic unsigned char help2_bits[] = {\n0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x80, 0x7f, 0x00, 0xc0, 0xe1, 0x00, 0xc0, 0xc0, 0x00, 0xc0, 0xc0, 0x00, 0x00, 0xc0, 0x00, 0x00, 0xc0, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x70, 0x00, 0x00, 0x38, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};"]

  button $w -image $help_img -relief flat -command "help_show_manual $file $section"
  set_balloon $w "Click to display context-sensitive documentation for this window"

  return $w

}

