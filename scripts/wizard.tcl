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

proc create_wizard {} {

  global preferences HOME

  # Now create the window and set the grab to this window
  if {[winfo exists .wizwin] == 0} {

    # Create new window
    toplevel .wizwin
    wm title .wizwin "Covered Wizard"

    ttk::frame .wizwin.f -relief raised -borderwidth 1 

    # Add banner
    ttk::label .wizwin.f.banner -image [image create photo -file [file join $HOME scripts banner.gif]]

    # Create buttons
    ttk::button .wizwin.f.new   -text "Generate New CDD File" -command {
      destroy .wizwin
      create_new_cdd
    }
    ttk::button .wizwin.f.open  -text "Open/Merge CDD File(s)" -command {
      destroy .wizwin
      .menubar.file invoke 0
    }
    ttk::button .wizwin.f.grade -text "Rank CDD Coverage" -command {
      destroy .wizwin
      create_rank_cdds
    }
    ttk::button .wizwin.f.help  -text "View Help Manual" -command {
      destroy .wizwin
      help_show_manual part.gui 
    }

    # Checkbutton to disable this window
    ttk::checkbutton .wizwin.f.show -text "Display this window at startup of Covered" -variable preferences(show_wizard) -onvalue true -offvalue false -command {
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
    grid .wizwin.f.hb     -row 4 -column 1 -sticky e    -padx 4 -pady 4

    # Pack the frame
    pack .wizwin.f -fill both -expand yes

  }

  # Make sure that the focus is on this window only
  wm transient .wizwin .

}
