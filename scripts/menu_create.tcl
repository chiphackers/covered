#!/usr/bin/env wish

set uncov_fgColor blue
set uncov_bgColor yellow
set cov_fgColor   black
set cov_bgColor   white
set uncov_type    1
set cov_type      0
set mod_inst_type "module"

set file_types {
  {{Code Coverage Database Files} {.cdd}}
  {{All Files}                    * }
}

proc do_keybind {.menubar} {

  bind all <Alt-f> {.menubar.file invoke}

}

proc menu_create {.menubar} {

  global start_line end_line
  global file_name file_types

  # Create the menu-buttons for File, Preferences and About
  menubutton .menubar.file   -text "File"        -underline 0
  menubutton .menubar.report -text "Report"      -underline 0
  menubutton .menubar.pref   -text "Preferences" -underline 0
  menubutton .menubar.help   -text "Help"        -underline 0

  # Configure the file option
  .menubar.file config -menu .menubar.file.menu
  set tfm [menu .menubar.file.menu -tearoff false]

  # Now add open and close options
  $tfm add command -label "Open Initial CDD..." -command {
    set file_name [tk_getOpenFile -filetypes $file_types]
    message .status -text "Opening $file_name..." -width 500 -relief raised
    place .status -in . -relx 0.33 -rely 0.5
    after 100 {
      tcl_func_open_cdd $file_name
      populate_listbox .bot.l
      destroy .status
      .bot.info configure -text "Select a module/instance at left for coverage details"
    }
    ;# Disable ourselves and allow user to replace or merge new CDD
    .menubar.file.menu entryconfigure 0 -state disabled
    .menubar.file.menu entryconfigure 1 -state normal
    .menubar.file.menu entryconfigure 2 -state normal
  }
  $tfm add command -label "Open Related CDD..." -state disabled -command {
    set file_name [tk_getOpenFile -filetypes $file_types]
    message .status -text "Opening $file_name..." -width 500 -relief raised
    place .status -in . -relx 0.33 -rely 0.5
    after 100 {
      tcl_func_replace_cdd $file_name
      populate_listbox .bot.l
      destroy .status
      .bot.info configure -text "Select a module/instance at left for coverage details"
    }
  }
  $tfm add command -label "Merge Related CDD..." -state disabled -command {
    set file_name [tk_getOpenFile -filetypes $file_types]
    message .status -text "Merging $file_name..." -width 500 -relief raised
    place .status -in . -relx 0.33 -rely 0.5
    after 100 {
      tcl_func_merge_cdd $file_name
      populate_listbox .bot.l
      destroy .status
      .bot.info configure -text "Select a module/instance at left for coverage details"
    }
  }
  $tfm add separator
  $tfm add command -label Exit -command exit

  # Configure the report option
  .menubar.report config -menu .menubar.report.menu
  set report [menu .menubar.report.menu -tearoff false]

  global mod_inst_type cov_uncov_type cov_rb

  $report add radiobutton -label "Module-based"   -variable mod_inst_type -value "module" -command {
    populate_listbox .bot.l
  }
  $report add radiobutton -label "Instance-based" -variable mod_inst_type -value "instance" -command {
    populate_listbox .bot.l
  }
  $report add separator
  $report add checkbutton -label "Show Uncovered" -variable uncov_type -onvalue 1 -offvalue 0 -command {
    set text_x [.bot.txt xview]
    set text_y [.bot.txt yview]
    if {$cov_rb == "line"} {
      calc_and_display_line_cov
    } elseif {$cov_rb == "toggle"} {
      calc_and_display_toggle_cov
    } else {
      # Error
    }
    .bot.txt xview moveto [lindex $text_x 0]
    .bot.txt yview moveto [lindex $text_y 0]
  }
  $report add checkbutton -label "Show Covered" -variable cov_type -onvalue 1 -offvalue 0 -command {
    set text_x [.bot.txt xview]
    set text_y [.bot.txt yview]
    if {$cov_rb == "line"} {
      calc_and_display_line_cov
    } elseif {$cov_rb == "toggle"} {
      calc_and_display_toggle_cov
    } else {
      # Error
    }
    .bot.txt xview moveto [lindex $text_x 0]
    .bot.txt yview moveto [lindex $text_y 0]
  }
  set mod_inst_type  "module"

  # Configure the color options
  .menubar.pref config -menu .menubar.pref.menu
  set m [menu .menubar.pref.menu -tearoff false]

  # Choose foreground color of uncovered lines
  global uncov_fgColor
  $m add command -label "Choose Uncovered Foreground ..." -command { 
    set uncov_fgColor [tk_chooseColor -initialcolor $uncov_fgColor -title \
                       "Choose Foreground Color for Uncovered Lines"]
    # Redisplay coverage
    set text_x [.bot.txt xview]
    set text_y [.bot.txt yview]
    if {$cov_rb == "line"} {
      display_line_cov
    } elseif {$cov_rb == "toggle"} {
      display_toggle_cov
    } else {
      # Error
    }
    .bot.txt xview moveto [lindex $text_x 0]
    .bot.txt yview moveto [lindex $text_y 0]
  }

  # Choose background color of uncovered lines
  global uncov_bgColor
  $m add command -label "Choose Uncovered Background ..." -command { 
    set uncov_bgColor [tk_chooseColor -initialcolor $uncov_bgColor -title \
                       "Choose Background Color for Uncovered Lines"]
    # Redisplay coverage
    set text_x [.bot.txt xview]
    set text_y [.bot.txt yview]
    if {$cov_rb == "line"} {
      display_line_cov
    } elseif {$cov_rb == "toggle"} {
      display_toggle_cov
    } else {
      # Error
    }
    .bot.txt xview moveto [lindex $text_x 0]
    .bot.txt yview moveto [lindex $text_y 0]
  }

  $m add separator

  # Choose foreground color of covered lines
  global cov_fgColor
  $m add command -label "Choose Covered Foreground ..." -command {
    set cov_fgColor [tk_chooseColor -initialcolor $cov_fgColor -title \
                     "Choose Foreground Color for Covered Lines"]
    # Redisplay coverage
    set text_x [.bot.txt xview]
    set text_y [.bot.txt yview]
    if {$cov_rb == "line"} {
      display_line_cov
    } elseif {$cov_rb == "toggle"} {
      display_toggle_cov
    } else {
      # Error
    }
    .bot.txt xview moveto [lindex $text_x 0]
    .bot.txt yview moveto [lindex $text_y 0]
  }

  # Choose background color of covered lines
  global cov_bgColor
  $m add command -label "Choose Covered Background ..." -command {
    set cov_bgColor [tk_chooseColor -initialcolor $cov_bgColor -title \
                     "Choose Background Color for Covered Lines"]
    # Redisplay coverage
    set text_x [.bot.txt xview]
    set text_y [.bot.txt yview]
    if {$cov_rb == "line"} {
      display_line_cov
    } elseif {$cov_rb == "toggle"} {
      display_toggle_cov
    } else {
      # Error
    }
    .bot.txt xview moveto [lindex $text_x 0]
    .bot.txt yview moveto [lindex $text_y 0]
  }

  # Configure the help option
  .menubar.help config -menu .menubar.help.menu
  set thm [menu .menubar.help.menu -tearoff false]

  # Add Manual and About information
  $thm add command -label "Manual" -state disabled
  $thm add separator
  $thm add command -label "About Covered" -command {tk_dialog .about "About Covered" "Covered\nWritten by:  Trevor Williams  (trevorw@charter.net)\n" "" 0 "Close"}
    
  # Pack the .menubar frame
  pack .menubar -side top -fill x
  
  # Pack the menu-buttons
  pack .menubar.file   -side left
  pack .menubar.report -side left
  pack .menubar.pref   -side left
  pack .menubar.help   -side right

  # Create a 3D Seperator 
  # Separator .seperator 
  # pack .seperator -fill x

  # Do key bindings for the Top Level Menus
  do_keybind .menubar

}
