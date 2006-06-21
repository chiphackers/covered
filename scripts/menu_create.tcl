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

  bind all <Control-o> {.menubar.file.menu invoke 0}
  bind all <Control-s> {.menubar.file.menu invoke 4}
  bind all <Control-w> {.menubar.file.menu invoke 6}
  bind all <Control-x> {.menubar.file.menu invoke 8}

  bind all <Control-n> {.menubar.view.menu invoke 2}
  bind all <Control-p> {.menubar.view.menu invoke 3}
  bind all <Control-c> {.menubar.view.menu invoke 4}

}

proc menu_create {.menubar} {

  global start_line end_line
  global file_name
  global BROWSER
  global prev_uncov_index next_uncov_index

  # Create the menu-buttons for File, Preferences and About
  menubutton .menubar.file   -text "File"        -underline 0
  menubutton .menubar.report -text "Report"      -underline 0
  menubutton .menubar.view   -text "View"        -underline 0
  menubutton .menubar.help   -text "Help"        -underline 0

  # Configure the file option
  .menubar.file config -menu .menubar.file.menu
  set tfm [menu .menubar.file.menu -tearoff false]

  # Now add open and close options
  $tfm add command -label "Open/Merge CDDs..." -accelerator "Ctrl-o" -command {
    if {[open_files] != ""} {
      .menubar.file.menu entryconfigure 1 -state normal
      .menubar.file.menu entryconfigure 3 -state normal
      .menubar.file.menu entryconfigure 6 -state normal
      .menubar.view.menu entryconfigure 0 -state normal
    }
  }
  $tfm add command -label "View Loaded CDD(s)..." -state disabled -command {
    create_cdd_viewer
  }
  $tfm add separator
  $tfm add command -label "Save CDD As..." -state disabled -command {
    set save_name "[tk_getSaveFile -filetypes $file_types].cdd"
    if {$save_name != ".cdd"} {
      tcl_func_save_cdd $save_name
      .menubar.file.menu entryconfigure 4 -state disabled
    }
  }
  $tfm add command -label "Save CDD" -accelerator "Ctrl-s" -state disabled -command {
    tcl_func_save_cdd $file_name
    .menubar.file.menu entryconfigure 4 -state disabled
  }
  $tfm add separator
  $tfm add command -label "Close CDD(s)" -accelerator "Ctrl-w" -state disabled -command {
    tcl_func_close_cdd
    set file_name ""
    clear_cdd_filelist
    populate_listbox .bot.left.l
    clear_all_windows
    .info configure -text ""
    .menubar.file.menu entryconfigure 1 -state disabled
    .menubar.file.menu entryconfigure 3 -state disabled
    .menubar.file.menu entryconfigure 4 -state disabled
    .menubar.file.menu entryconfigure 6 -state disabled
  }
  $tfm add separator
  $tfm add command -label Exit -accelerator "Ctrl-x" -command exit

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
  .menubar.view config -menu .menubar.view.menu
  set m [menu .menubar.view.menu -tearoff false]

  # Summary window
  $m add command -label "Summary..." -state disabled -command {
    create_summary
  }
  $m add separator
  $m add command -label "Next Uncovered" -state disabled -accelerator "Ctrl-n" -command {
    goto_uncov $next_uncov_index
  }
  $m add command -label "Previous Uncovered" -state disabled -accelerator "Ctrl-p" -command {
    goto_uncov $prev_uncov_index
  }
  $m add command -label "Show Current Selection" -state disabled -accelerator "Ctrl-c" -command {
    if {$cov_rb == "toggle"} {
      if {$curr_toggle_ptr != ""} {
        .bot.right.txt see $curr_toggle_ptr.0
      }
    } elseif {$cov_rb == "comb"} {
      if {$curr_comb_ptr != ""} {
        .bot.right.txt see $curr_comb_ptr.0
      }
    }
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
  pack .menubar.view   -side left
  pack .menubar.help   -side right

  # Create a 3D Seperator 
  # Separator .seperator 
  # pack .seperator -fill x

  # Do key bindings for the Top Level Menus
  do_keybind .menubar

}

# Opens/merges a CDD file and handles the GUI cursors and listbox initialization.
proc open_files {} {

  global file_types file_name fname global open_type
  global win_cursor txt_cursor e_cursor

  # Get a list of files to open
  if {[catch {tk_getOpenFile -multiple 1 -filetypes $file_types} fnames]} {
    set fnames [tk_getOpenFile -filetypes $file_types]
  }

  # Get all cursor values from various widgets (so we can properly restore them after the open)
  set win_cursor [. cget -cursor]
  set txt_cursor [.bot.right.txt cget -cursor]
  set e_cursor   [.bot.right.h.e cget -cursor]

  foreach fname $fnames {

    if {$file_name == ""} {
      set open_type "open"
      .info configure -text "Opening $fname..."
      puts "Opening $fname..."
      add_cdd_to_filelist $fname 1
    } else {
      set open_type "merge"
      .info configure -text "Merging $fname..."
      puts "Merging $fname..."
      add_cdd_to_filelist $fname 0
    }

    # Set all widget cursors to the watch
    .              configure -cursor watch
    .bot.right.txt configure -cursor watch
    .bot.right.h.e configure -cursor watch

    after 100 {

      # Open the CDD file
      if {$open_type == "open"} {
        tcl_func_open_cdd $fname
      } else {
        tcl_func_merge_cdd $fname
        .menubar.file.menu entryconfigure 3 -state normal
        .menubar.file.menu entryconfigure 4 -state normal
      }

      # Populate the listbox
      populate_listbox .bot.left.l

      # Update the summary window
      update_summary

    }

    if {$file_name == ""} {
      set file_name $fname
    }

  }

  # Place new information in the info box
  .info configure -text "Select a module/instance at left for coverage details"

  # Reset the cursors
  .              configure -cursor $win_cursor
  .bot.right.txt configure -cursor $txt_cursor
  .bot.right.h.e configure -cursor $e_cursor

  return $file_name

}
