################################################################################################
# Copyright (c) 2006-2010 Trevor Williams                                                      #
#                                                                                              #
# This program is free software; you can redistribute it and/or modify                         #
# it under the terms of the GNU General Public License as published by the Free Software       #
# Foundation; either version 2 of the License, or (at your option) any later version.          #
#                                                                                              #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;    #
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    #
# See the GNU General Public License for more details.                                         #
#                                                                                              #
# You should have received a copy of the GNU General Public License along with this program;   #
# if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. #
################################################################################################

# Contains procs for handling help menu windows

proc help_show_about {} {

  global VERSION HOME

  if {[winfo exists .helpabout]} {

    raise .helpabout

  } else {

    # Create window and populate with widgets
    toplevel .helpabout -background white
    wm title .helpabout "About Covered"

    ttk::button .helpabout.b -text "Close" -command {
      destroy .helpabout
    }
    ttk::label .helpabout.i    -background white -image [image create photo -file [file join $HOME scripts banner.gif]]

    ttk::frame .helpabout.f
    ttk::label .helpabout.f.l0_0 -background white -justify left -text "Version:"
    ttk::label .helpabout.f.l0_1 -background white -justify left -text $VERSION
    ttk::label .helpabout.f.l1_0 -background white -justify left -text "Author:"
    ttk::label .helpabout.f.l1_1 -background white -justify left -text "Trevor Williams"
    ttk::label .helpabout.f.l2_0 -background white -justify left -text "Email:"
    ttk::label .helpabout.f.l2_1 -background white -justify left -text "phase1geo@gmail.com"
    ttk::label .helpabout.f.l3_0 -background white -justify left -text "Homepage:"
    ttk::label .helpabout.f.l3_1 -background white -justify left -text "http://covered.sourceforge.net"
    ttk::label .helpabout.f.l4   -background white -justify left -text ""
    ttk::label .helpabout.f.l5   -background white -justify left -text "Freely distributable under the GPL license"

    grid .helpabout.f.l0_0 -row 0 -column 0 -sticky news
    grid .helpabout.f.l0_1 -row 0 -column 1 -sticky news
    grid .helpabout.f.l1_0 -row 1 -column 0 -sticky news
    grid .helpabout.f.l1_1 -row 1 -column 1 -sticky news
    grid .helpabout.f.l2_0 -row 2 -column 0 -sticky news
    grid .helpabout.f.l2_1 -row 2 -column 1 -sticky news
    grid .helpabout.f.l3_0 -row 3 -column 0 -sticky news
    grid .helpabout.f.l3_1 -row 3 -column 1 -sticky news
    grid .helpabout.f.l4   -row 4 -column 0 -sticky news -columnspan 2
    grid .helpabout.f.l5   -row 5 -column 0 -sticky news -columnspan 2

    pack .helpabout.i -pady 10
    pack .helpabout.f -padx 10 -pady 10
    pack .helpabout.b -pady 10

    focus -force .helpabout.b

  } 

}

proc help_show_manual {chapter {section ""}} {

  global HOME BROWSER

  if {[string first "kfmclient" $BROWSER] != -1} {

    set fpath "file:[file join $HOME doc html $chapter].html"

    if {$section != ""} {
      set fpath $fpath#$section
    }

    if {[catch {exec $BROWSER exec $fpath} emsg]} {
      error "Error displaying $fpath in browser\n$emsg"
    }

  } else {

    set fpath "file://[file join $HOME doc html $chapter].html"

    if {$section != ""} {
      set fpath $fpath#$section
    }

    if {[catch {exec $BROWSER -remote "openURL( $fpath )"}]} {

      # perhaps browser doesn't understand -remote flag
      if {[catch "exec $BROWSER \"$fpath\" &" emsg]} {
        error "Error displaying $fpath in browser\n$emsg"
      }

    }

  }

}

proc help_button {w file {section ""}} {

  set help_img [image create bitmap -data "#define help2_width 22\n#define help2_height 22\nstatic unsigned char help2_bits[] = {\n0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x80, 0x7f, 0x00, 0xc0, 0xe1, 0x00, 0xc0, 0xc0, 0x00, 0xc0, 0xc0, 0x00, 0x00, 0xc0, 0x00, 0x00, 0xc0, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x70, 0x00, 0x00, 0x38, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};"]

  ttk::label $w -image $help_img -relief flat
  bind $w <Button-1> "help_show_manual $file $section"
  set_balloon $w "Click to display context-sensitive documentation for this window"

  return $w

}

