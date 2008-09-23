#!/usr/bin/env wish

set cdd_name           ""
set uncov_type         1
set cov_type           0
set race_type          0
set mod_inst_type      "Module"
set last_mod_inst_type ""

set file_types {
  {{Code Coverage Database Files} {.cdd}}
  {{All Files}                    * }
}

set rpt_file_types {
  {{Covered Report Files} {.rpt}}
  {{All Files}            * }
}

proc do_keybind {.menubar} {

  # Add Mac OS X familiar shortcuts for the menubar
  if {[tk windowingsystem] eq "aqua"} {
    bind all <Command-o> {.menubar.file invoke 0}
    bind all <Command-s> {.menubar.file invoke 3}
    bind all <Command-w> {.menubar.file invoke 4}
  }

  bind all <Control-o> {.menubar.file invoke 0}
  bind all <Control-O> {.menubar.file invoke 0}
  bind all <Control-s> {.menubar.file invoke 3}
  bind all <Control-S> {.menubar.file invoke 3}
  bind all <Control-w> {.menubar.file invoke 4}
  bind all <Control-W> {.menubar.file invoke 4}
  bind all <Control-x> {.menubar.file invoke 8}
  bind all <Control-X> {.menubar.file invoke 8}

  bind all <Shift-Control-n> {.menubar.file.gen invoke 0}
  bind all <Shift-Control-N> {.menubar.file.gen invoke 0}
  bind all <Shift-Control-r> {.menubar.file.gen invoke 1}
  bind all <Shift-Control-R> {.menubar.file.gen invoke 1}
  bind all <Shift-Control-c> {.menubar.file.gen invoke 2}
  bind all <Shift-Control-C> {.menubar.file.gen invoke 2}

  bind all <Control-n> {.menubar.view invoke 0}
  bind all <Control-N> {.menubar.view invoke 0}
  bind all <Control-p> {.menubar.view invoke 1}
  bind all <Control-P> {.menubar.view invoke 1}
  bind all <Control-c> {.menubar.view invoke 2}
  bind all <Control-C> {.menubar.view invoke 2}

}

proc enable_cdd_save {} {

  .menubar.file entryconfigure 3 -state normal

}

proc menu_create {} {

  global start_line end_line
  global cdd_name
  global BROWSER
  global prev_uncov_index next_uncov_index

  # Create the menubar frame
  #frame .menubar -width 710 -relief raised -borderwidth 1
  set mb [menu .menubar -tearoff false]
  . configure -menu $mb

  # Configure the file option
  set tfm [menu $mb.file -tearoff false]
  $mb add cascade -label "File" -menu $tfm

  # FILE - entry 0
  $tfm add command -label "Open/Merge CDDs..." -accelerator "Ctrl-o" -underline 0 -command {
    # Get a list of files to open
    set fnames [tk_getOpenFile -multiple 1 -filetypes $file_types]
    if {$fnames ne ""} {
      open_files $fnames
    }
  }
  # FILE - entry 1
  $tfm add command -label "View Loaded CDD(s)..." -state disabled -underline 0 -command {
    create_cdd_viewer
  }
  # FILE - entry 2
  $tfm add separator
  # FILE - entry 3
  $tfm add command -label "Save CDD..." -accelerator "Ctrl-s" -state disabled -underline 0 -command {
    set save_name [tk_getSaveFile -filetypes $file_types -initialfile [file tail $cdd_name] -title "Save CDD As"]
    if {$save_name != ""} {
      if {[file extension $save_name] != ".cdd"} {
        set save_name "$save_name.cdd"
      } 
      tcl_func_save_cdd $save_name
      .info configure -text "$save_name successfully saved"
      .menubar.file entryconfigure 3 -state disabled
    }
  }
  # FILE - entry 4
  $tfm add command -label "Close CDD(s)" -accelerator "Ctrl-w" -state disabled -underline 0 -command {
    check_to_save_and_close_cdd closing
    .info configure -text "$cdd_name closed"
    set cdd_name ""
    set curr_block 0
    clear_cdd_filelist
    populate_listbox
    clear_all_windows
    .menubar.file entryconfigure 1 -state disabled
    .menubar.file entryconfigure 3 -state disabled
    .menubar.file entryconfigure 4 -state disabled
    .menubar.file.gen entryconfigure 1 -state disabled
  }
  # FILE - entry 5
  $tfm add separator
  # FILE - entry 6
  $tfm add cascade -label "Generate" -menu $tfm.gen -underline 0
  menu $tfm.gen -tearoff false
  # FILE-GEN - entry 0
  $tfm.gen add command -label "New CDD..." -accelerator "Ctrl-Shift-n" -command {
    create_new_cdd
  }
  $tfm.gen add command -label "ASCII Report..." -accelerator "Ctrl-Shift-r" -state disabled -command {
    create_report_generation_window
  }
  $tfm.gen add command -label "CDD Ranking Report..." -accelerator "Ctrl-Shift-c" -command {
    create_rank_cdds
  }

  # We don't need the exit function if we are running the Aqua version
  if {[tk windowingsystem] ne "aqua"} {
    # FILE - entry 7
    $tfm add separator
    # FILE - entry 8
    $tfm add command -label Exit -accelerator "Ctrl-x" -underline 1 -command {
      check_to_save_and_close_cdd exiting
      save_gui_elements . .
      destroy .
    }
  }

  # Configure the report option
  set report [menu $mb.report -tearoff false]
  $mb add cascade -label "Report" -menu $report

  global mod_inst_type cov_uncov_type cov_rb

  $report add radiobutton -label "Module-based"   -variable mod_inst_type -value "Module" -underline 0
  $report add radiobutton -label "Instance-based" -variable mod_inst_type -value "Instance" -underline 0
  $report add separator
  $report add radiobutton -label "Line"   -variable cov_rb -value "Line"   -underline 0
  $report add radiobutton -label "Toggle" -variable cov_rb -value "Toggle" -underline 0
  $report add radiobutton -label "Memory" -variable cov_rb -value "Memory" -underline 1
  $report add radiobutton -label "Logic"  -variable cov_rb -value "Logic"  -underline 2
  $report add radiobutton -label "FSM"    -variable cov_rb -value "FSM"    -underline 0
  $report add radiobutton -label "Assert" -variable cov_rb -value "Assert" -underline 0
  $report add separator
  $report add checkbutton -label "Show Uncovered" -variable uncov_type -onvalue 1 -offvalue 0 -underline 5 -command {
    set text_x [.bot.right.txt xview]
    set text_y [.bot.right.txt yview]
    if {$cov_rb == "Line"} {
      display_line_cov
    } elseif {$cov_rb == "Toggle"} {
      display_toggle_cov
    } elseif {$cov_rb == "Memory"} {
      display_memory_cov
    } elseif {$cov_rb == "Logic"} {
      display_comb_cov
    } elseif {$cov_rb == "FSM"} {
      display_fsm_cov
    } elseif {$cov_rb == "Assert"} {
      display_assert_cov
    } else {
      # Error
    }
    .bot.right.txt xview moveto [lindex $text_x 0]
    .bot.right.txt yview moveto [lindex $text_y 0]
  }
  $report add checkbutton -label "Show Covered" -variable cov_type -onvalue 1 -offvalue 0 -underline 5 -command {
    set text_x [.bot.right.txt xview]
    set text_y [.bot.right.txt yview]
    if {$cov_rb == "Line"} {
      display_line_cov
    } elseif {$cov_rb == "Toggle"} {
      display_toggle_cov
    } elseif {$cov_rb == "Memory"} {
      display_memory_cov
    } elseif {$cov_rb == "Logic"} {
      display_comb_cov
    } elseif {$cov_rb == "FSM"} {
      display_fsm_cov
    } elseif {$cov_rb == "Assert"} {
      display_assert_cov
    } else {
      # Error
    }
    .bot.right.txt xview moveto [lindex $text_x 0]
    .bot.right.txt yview moveto [lindex $text_y 0]
  }
  $report add checkbutton -label "Show Race Conditions" -variable race_type -onvalue 1 -offvalue 0 -underline 5 -command {
    set text_x [.bot.right.txt xview]
    set text_y [.bot.right.txt yview]
    if {$cov_rb == "Line"} {
      display_line_cov
    } elseif {$cov_rb == "Toggle"} {
      display_toggle_cov
    } elseif {$cov_rb == "Memory"} {
      display_memory_cov
    } elseif {$cov_rb == "Logic"} {
      display_comb_cov
    } elseif {$cov_rb == "FSM"} {
      display_fsm_cov
    } elseif {$cov_rb == "Assert"} {
      display_assert_cov
    } else {
      # Error
    }
    .bot.right.txt xview moveto [lindex $text_x 0]
    .bot.right.txt yview moveto [lindex $text_y 0]
  }
  set mod_inst_type  "Module"

  # Configure the color options
  set m [menu $mb.view -tearoff false]
  $mb add cascade -label "View" -menu $m

  $m add command -label "Next Uncovered" -state disabled -accelerator "Ctrl-n" -underline 0 -command {
    goto_uncov $next_uncov_index
  }
  $m add command -label "Previous Uncovered" -state disabled -accelerator "Ctrl-p" -underline 0 -command {
    goto_uncov $prev_uncov_index
  }
  $m add command -label "Show Current Selection" -state disabled -accelerator "Ctrl-c" -underline 5 -command {
    if {$cov_rb == "Toggle"} {
      if {$curr_toggle_ptr != ""} {
        .bot.right.txt see $curr_toggle_ptr.0
      }
    } elseif {$cov_rb == "Logic"} {
      if {$curr_comb_ptr != ""} {
        .bot.right.txt see $curr_comb_ptr.0
      }
    }
  }
  $m add separator
  $m add command -label "Wizard..." -underline 0 -command {
    create_wizard
  }
  # If we are running on Mac OS X, add preferences to applications menu
  if {[tk windowingsystem] eq "aqua"} {
    proc ::tk::mac::ShowPreferences {} { create_preferences -1 }
  } else {
    $m add separator
    $m add command -label "Preferences..." -underline 0 -command {
      create_preferences -1
    }
  }

  # Configure the help option
  set thm [menu $mb.help -tearoff false]
  $mb add cascade -label "Help" -menu $thm

  # Add Manual and About information
  $thm add command -label "Manual" -state disabled -underline 0 -command {
    help_show_manual index ""
  }
  # Do not display the About information in the help menu if we are running on Mac OS X
  if {[tk windowingsystem] ne "aqua"} {
    $thm add separator
    $thm add command -label "About Covered" -underline 0 -command {
      help_show_about
    }
  }

  # If we are running on Mac OS X, add items to the applications menu
  if {[tk windowingsystem] eq "aqua"} {
    set appl [menu $mb.apple -tearoff false]
    $mb add cascade -menu $appl

    $appl add command -label "About Covered" -command {
      help_show_about
    }
    $appl add separator
  }

  # Enable the manual help entry if we have a browser to use
  if {$BROWSER != ""} {
    $thm entryconfigure 0 -state normal
  }
    
  # Do key bindings for the Top Level Menus
  do_keybind .menubar

}

# Opens/merges a CDD file and handles the GUI cursors and listbox initialization.
proc open_files {fnames} {

  global file_types cdd_name fname open_type
  global win_cursor txt_cursor e_cursor
  global exclude_resolution

  # Get all cursor values from various widgets (so we can properly restore them after the open)
  set win_cursor [. cget -cursor]
  set txt_cursor [.bot.right.txt cget -cursor]
  set e_cursor   [.bot.right.h.search.e cget -cursor]

  foreach fname $fnames {

    if {$cdd_name == ""} {
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
    .                     configure -cursor watch
    .bot.right.txt        configure -cursor watch
    .bot.right.h.search.e configure -cursor watch

    after 100 {

      # Open the CDD file
      if {$open_type == "open"} {
        tcl_func_open_cdd $fname
      } else {
        tcl_func_merge_cdd $fname $exclude_resolution
        .menubar.file entryconfigure 3 -state normal
      }

      # Populate the listbox
      populate_listbox

    }

    if {$cdd_name == ""} {
      set cdd_name $fname
    }

  }

  # Place new information in the info box
  .info configure -text "Select a module/instance at left for coverage details"

  # Reset the cursors
  .                     configure -cursor $win_cursor
  .bot.right.txt        configure -cursor $txt_cursor
  .bot.right.h.search.e configure -cursor $e_cursor

  # Change some of the GUI elements
  if {$cdd_name ne ""} {
    .menubar.file     entryconfigure 1 -state normal
    .menubar.file     entryconfigure 4 -state normal
    .menubar.file.gen entryconfigure 1 -state normal
  }

  return $cdd_name

}

# Call this function when the main window will be destroyed or when a CDD is attempting to be closed
proc check_to_save_and_close_cdd {msg} {

  if {[.menubar.file entrycget 3 -state] == "normal"} {
    set exit_status [tk_messageBox -message "Opened database has changed.  Would you like to save before $msg?" -type yesnocancel -icon warning]
    if {$exit_status == "yes"} {
      .menubar.file invoke 3
    } elseif {$exit_status == "cancel"} {
      return
    }
  }

  if {[.menubar.file entrycget 4 -state] == "normal"} {
    tcl_func_close_cdd
    puts "Closed all opened/merged CDD files"
  }

}
