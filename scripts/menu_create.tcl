#!/usr/bin/env wish

set uncov_type    1
set cov_type      0
set race_type     0
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
  global BROWSER

  # Create the menu-buttons for File, Preferences and About
  menubutton .menubar.file   -text "File"        -underline 0
  menubutton .menubar.report -text "Report"      -underline 0
  menubutton .menubar.tools  -text "Tools"       -underline 0
  menubutton .menubar.help   -text "Help"        -underline 0

  # Configure the file option
  .menubar.file config -menu .menubar.file.menu
  set tfm [menu .menubar.file.menu -tearoff false]

  # Now add open and close options
  $tfm add command -label "Open Initial CDD..." -command {
    set file_name [tk_getOpenFile -filetypes $file_types]
    if {$file_name != ""} {
      message .status -text "Opening $file_name..." -width 500 -relief raised
      place .status -in . -relx 0.33 -rely 0.5
      after 100 {
        tcl_func_open_cdd $file_name
        populate_listbox .bot.left.l
        destroy .status
        .info configure -text "Select a module/instance at left for coverage details"
      }
      ;# Disable ourselves and allow user to replace or merge new CDD and view summary window
      .menubar.file.menu entryconfigure 0 -state disabled
      .menubar.file.menu entryconfigure 1 -state normal
      .menubar.file.menu entryconfigure 2 -state normal
      .menubar.tools.menu entryconfigure 0 -state normal
    }
  }
  $tfm add command -label "Open Related CDD..." -state disabled -command {
    set file_name [tk_getOpenFile -filetypes $file_types]
    if {$file_name != ""} {
      message .status -text "Opening $file_name..." -width 500 -relief raised
      place .status -in . -relx 0.33 -rely 0.5
      after 100 {
        tcl_func_replace_cdd $file_name
        populate_listbox .bot.left.l
        destroy .status
        .info configure -text "Select a module/instance at left for coverage details"
      }
    }
  }
  $tfm add command -label "Merge Related CDD..." -state disabled -command {
    set file_name [tk_getOpenFile -filetypes $file_types]
    if {$file_name != ""} {
      message .status -text "Merging $file_name..." -width 500 -relief raised
      place .status -in . -relx 0.33 -rely 0.5
      after 100 {
        tcl_func_merge_cdd $file_name
        populate_listbox .bot.left.l
        destroy .status
        .info configure -text "Select a module/instance at left for coverage details"
      }
    }
  }
  $tfm add separator
  $tfm add command -label Exit -command exit

  # Configure the report option
  .menubar.report config -menu .menubar.report.menu
  set report [menu .menubar.report.menu -tearoff false]

  global mod_inst_type cov_uncov_type cov_rb

  $report add radiobutton -label "Module-based"   -variable mod_inst_type -value "module" -command {
    populate_listbox .bot.left.l
    update_summary
  }
  $report add radiobutton -label "Instance-based" -variable mod_inst_type -value "instance" -command {
    populate_listbox .bot.left.l
    update_summary
  }
  $report add separator
  $report add checkbutton -label "Show Uncovered" -variable uncov_type -onvalue 1 -offvalue 0 -command {
    set text_x [.bot.right.txt xview]
    set text_y [.bot.right.txt yview]
    if {$cov_rb == "line"} {
      display_line_cov
    } elseif {$cov_rb == "toggle"} {
      display_toggle_cov
    } elseif {$cov_rb == "comb"} {
      display_comb_cov
    } else {
      # Error
    }
    .bot.right.txt xview moveto [lindex $text_x 0]
    .bot.right.txt yview moveto [lindex $text_y 0]
  }
  $report add checkbutton -label "Show Covered" -variable cov_type -onvalue 1 -offvalue 0 -command {
    set text_x [.bot.right.txt xview]
    set text_y [.bot.right.txt yview]
    if {$cov_rb == "line"} {
      display_line_cov
    } elseif {$cov_rb == "toggle"} {
      display_toggle_cov
    } elseif {$cov_rb == "comb"} {
      display_comb_cov
    } else {
      # Error
    }
    .bot.right.txt xview moveto [lindex $text_x 0]
    .bot.right.txt yview moveto [lindex $text_y 0]
  }
  $report add checkbutton -label "Show Race Conditions" -variable race_type -onvalue 1 -offvalue 0 -command {
    set text_x [.bot.right.txt xview]
    set text_y [.bot.right.txt yview]
    if {$cov_rb == "line"} {
      display_line_cov
    } elseif {$cov_rb == "toggle"} {
      display_toggle_cov
    } elseif {$cov_rb == "comb"} {
      display_comb_cov
    } else {
      # Error
    }
    .bot.right.txt xview moveto [lindex $text_x 0]
    .bot.right.txt yview moveto [lindex $text_y 0]
  }
  set mod_inst_type  "module"

  # Configure the color options
  .menubar.tools config -menu .menubar.tools.menu
  set m [menu .menubar.tools.menu -tearoff false]

  # Summary window
  $m add command -label "Summary Window..." -state disabled -command {
    create_summary
  }
  $m add separator
  $m add command -label "Preferences..." -command {
    create_preferences
  }

  # Configure the help option
  .menubar.help config -menu .menubar.help.menu
  set thm [menu .menubar.help.menu -tearoff false]

  # Add Manual and About information
  $thm add command -label "Manual" -state disabled -command {
    help_show_manual "welcome"
  }
  $thm add separator
  $thm add command -label "About Covered" -command {
    help_show_about
  }

  # Enable the manual help entry if we have a browser to use
  if {$BROWSER != ""} {
    $thm entryconfigure 0 -state normal
  }
    
  # Pack the .menubar frame
  pack .menubar -side top -fill x
  
  # Pack the menu-buttons
  pack .menubar.file   -side left
  pack .menubar.report -side left
  pack .menubar.tools  -side left
  pack .menubar.help   -side right

  # Create a 3D Seperator 
  # Separator .seperator 
  # pack .seperator -fill x

  # Do key bindings for the Top Level Menus
  do_keybind .menubar

}
