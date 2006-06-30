#!/usr/bin/env wish

set cdd_name      ""
set uncov_type    1
set cov_type      0
set race_type     0
set mod_inst_type "module"

set file_types {
  {{Code Coverage Database Files} {.cdd}}
  {{All Files}                    * }
}

set rpt_file_types {
  {{Covered Report Files} {.rpt}}
  {{All Files}            * }
}

proc do_keybind {.menubar} {

  bind all <Control-o> {.menubar.file.menu invoke 0}
  bind all <Control-s> {.menubar.file.menu invoke 3}
  bind all <Control-w> {.menubar.file.menu invoke 4}
  bind all <Control-x> {.menubar.file.menu invoke 8}

  bind all <Control-r> {.menubar.file.menu.gen invoke 0}

  bind all <Control-n> {.menubar.view.menu invoke 2}
  bind all <Control-p> {.menubar.view.menu invoke 3}
  bind all <Control-c> {.menubar.view.menu invoke 4}

}

proc enable_cdd_save {} {

  .menubar.file.menu entryconfigure 3 -state normal

}

proc menu_create {.menubar} {

  global start_line end_line
  global cdd_name
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

  # FILE - entry 0
  $tfm add command -label "Open/Merge CDDs..." -accelerator "Ctrl-o" -underline 0 -command {
    if {[open_files] != ""} {
      .menubar.file.menu entryconfigure 1 -state normal
      .menubar.file.menu entryconfigure 4 -state normal
      .menubar.file.menu.gen entryconfigure 0 -state normal
      .menubar.view.menu entryconfigure 0 -state normal
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
      .menubar.file.menu entryconfigure 3 -state disabled
    }
  }
  # FILE - entry 4
  $tfm add command -label "Close CDD(s)" -accelerator "Ctrl-w" -state disabled -underline 0 -command {
    if {[.menubar.file.menu entrycget 3 -state] == "normal"} {
      set exit_status [tk_messageBox -message "Opened database has changed.  Would you like to save before closing?" \
                       -type yesnocancel -icon warning]
      if {$exit_status == "yes"} {
        .menubar.file.menu invoke 3
      } elseif {$exit_status == "cancel"} {
        return
      }
    }
    tcl_func_close_cdd
    .info configure -text "$cdd_name closed"
    set cdd_name ""
    clear_cdd_filelist
    populate_listbox .bot.left.l
    clear_all_windows
    .menubar.file.menu entryconfigure 1 -state disabled
    .menubar.file.menu entryconfigure 3 -state disabled
    .menubar.file.menu entryconfigure 4 -state disabled
    .menubar.file.menu.gen entryconfigure 0 -state disabled
    .menubar.view.menu entryconfigure 0 -state disabled
  }
  # FILE - entry 5
  $tfm add separator
  # FILE - entry 6
  $tfm add cascade -label "Generate" -menu $tfm.gen -underline 0
  menu $tfm.gen -tearoff false
  # FILE-GEN - entry 0
  $tfm.gen add command -label "ASCII Report..." -accelerator "Ctrl-r" -state disabled -command {
    create_report_selection_window
  }
  # FILE - entry 7
  $tfm add separator
  # FILE - entry 8
  $tfm add command -label Exit -accelerator "Ctrl-x" -underline 1 -command {
    if {[.menubar.file.menu entrycget 3 -state] == "normal"} {
      set exit_status [tk_messageBox -message "Opened database has changed.  Would you like to save before exiting?" \
                                     -type yesnocancel -icon warning]
      if {$exit_status == "yes"} {
        .menubar.file.menu invoke 3
      } elseif {$exit_status == "cancel"} {
        return
      }
    }
    exit
  }

  # Configure the report option
  .menubar.report config -menu .menubar.report.menu
  set report [menu .menubar.report.menu -tearoff false]

  global mod_inst_type cov_uncov_type cov_rb

  $report add radiobutton -label "Module-based"   -variable mod_inst_type -value "module" -underline 0 -command {
    populate_listbox .bot.left.l
    update_summary
  }
  $report add radiobutton -label "Instance-based" -variable mod_inst_type -value "instance" -underline 1 -command {
    populate_listbox .bot.left.l
    update_summary
  }
  $report add separator
  $report add checkbutton -label "Show Uncovered" -variable uncov_type -onvalue 1 -offvalue 0 -underline 5 -command {
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
  $report add checkbutton -label "Show Covered" -variable cov_type -onvalue 1 -offvalue 0 -underline 5 -command {
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
  $report add checkbutton -label "Show Race Conditions" -variable race_type -onvalue 1 -offvalue 0 -underline 5 -command {
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
  $m add command -label "Summary..." -state disabled -underline 0 -command {
    create_summary
  }
  $m add separator
  $m add command -label "Next Uncovered" -state disabled -accelerator "Ctrl-n" -underline 0 -command {
    goto_uncov $next_uncov_index
  }
  $m add command -label "Previous Uncovered" -state disabled -accelerator "Ctrl-p" -underline 0 -command {
    goto_uncov $prev_uncov_index
  }
  $m add command -label "Show Current Selection" -state disabled -accelerator "Ctrl-c" -underline 5 -command {
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
  $m add command -label "Preferences..." -underline 0 -command {
    create_preferences
  }

  # Configure the help option
  .menubar.help config -menu .menubar.help.menu
  set thm [menu .menubar.help.menu -tearoff false]

  # Add Manual and About information
  $thm add command -label "Manual" -state disabled -underline 0 -command {
    help_show_manual "welcome"
  }
  $thm add separator
  $thm add command -label "About Covered" -underline 0 -command {
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

  # Do key bindings for the Top Level Menus
  do_keybind .menubar

}

# Opens/merges a CDD file and handles the GUI cursors and listbox initialization.
proc open_files {} {

  global file_types cdd_name fname global open_type
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
      }

      # Populate the listbox
      populate_listbox .bot.left.l

      # Update the summary window
      update_summary

    }

    if {$cdd_name == ""} {
      set cdd_name $fname
    }

  }

  # Place new information in the info box
  .info configure -text "Select a module/instance at left for coverage details"

  # Reset the cursors
  .              configure -cursor $win_cursor
  .bot.right.txt configure -cursor $txt_cursor
  .bot.right.h.e configure -cursor $e_cursor

  return $cdd_name

}

proc create_report_selection_window {} {

  global rsel_sdv rsel_mi rsel_cu
  global rsel_l rsel_t rsel_c rsel_f rsel_a rsel_r
  global rsel_width rsel_wsel
  global REPORT_SUMMARY REPORT_DETAILED REPORT_VERBOSE
  global rsel_fname cdd_name

  toplevel .rselwin
  wm title .rselwin "Select report generation options"
  # wm resizable .rselwin 0 0
  wm transient .rselwin .
  grab set .rselwin

  # Set default values for radio/check buttons
  set rsel_sdv   $REPORT_SUMMARY
  set rsel_mi    0
  set rsel_cu    0
  set rsel_l     1
  set rsel_t     1
  set rsel_c     1
  set rsel_f     1
  set rsel_a     0
  set rsel_r     0
  set rsel_wsel  0
  set rsel_width "80"
  set rsel_fname "[file rootname [file tail $cdd_name]].rpt"

  # Create detail selection frame
  frame .rselwin.sdv -relief raised -borderwidth 1
  label .rselwin.sdv.lbl -text "Level of detail" -anchor w
  radiobutton .rselwin.sdv.s -text "Summary"  -variable rsel_sdv -value $REPORT_SUMMARY  -anchor e
  radiobutton .rselwin.sdv.d -text "Detailed" -variable rsel_sdv -value $REPORT_DETAILED -anchor e
  radiobutton .rselwin.sdv.v -text "Verbose"  -variable rsel_sdv -value $REPORT_VERBOSE  -anchor e
  grid .rselwin.sdv.lbl -row 0 -column 0 -sticky news -pady 4
  grid .rselwin.sdv.s   -row 1 -column 0 -sticky news
  grid .rselwin.sdv.d   -row 1 -column 1 -sticky news
  grid .rselwin.sdv.v   -row 1 -column 2 -sticky news

  # Create module/instance selection frame
  frame .rselwin.mi -relief raised -borderwidth 1
  label .rselwin.mi.lbl -text "Report by" -anchor w
  radiobutton .rselwin.mi.m -text "Module"   -variable rsel_mi -value 0 -anchor e
  radiobutton .rselwin.mi.i -text "Instance" -variable rsel_mi -value 1 -anchor e
  grid .rselwin.mi.lbl -row 0 -column 0 -sticky news -pady 4
  grid .rselwin.mi.m   -row 1 -column 0 -sticky news
  grid .rselwin.mi.i   -row 1 -column 1 -sticky news

  # Create metric selection frame
  frame .rselwin.metric -relief raised -borderwidth 1
  label .rselwin.metric.lbl -text "Metrics" -anchor w
  checkbutton .rselwin.metric.l -text "Line"            -variable rsel_l -anchor e
  checkbutton .rselwin.metric.t -text "Toggle"          -variable rsel_t -anchor e
  checkbutton .rselwin.metric.c -text "Logic"           -variable rsel_c -anchor e
  checkbutton .rselwin.metric.f -text "FSM"             -variable rsel_f -anchor e
  checkbutton .rselwin.metric.a -text "Assertion"       -variable rsel_a -anchor e
  checkbutton .rselwin.metric.r -text "Race Conditions" -variable rsel_r -anchor e
  grid .rselwin.metric.lbl -row 0 -column 0 -sticky w -pady 4
  grid .rselwin.metric.l   -row 1 -column 0 -sticky w
  grid .rselwin.metric.t   -row 1 -column 1 -sticky w
  grid .rselwin.metric.c   -row 1 -column 2 -sticky w
  grid .rselwin.metric.f   -row 2 -column 0 -sticky w
  grid .rselwin.metric.a   -row 2 -column 1 -sticky w
  grid .rselwin.metric.r   -row 2 -column 2 -sticky w

  # Create covered/uncovered selection frame
  frame .rselwin.cu -relief raised -borderwidth 1
  label .rselwin.cu.lbl -text "Show coverage" -anchor w
  radiobutton .rselwin.cu.u -text "Uncovered" -variable rsel_cu -value 0 -anchor e
  radiobutton .rselwin.cu.c -text "Covered"   -variable rsel_cu -value 1 -anchor e
  grid .rselwin.cu.lbl -row 0 -column 0 -sticky news -pady 4
  grid .rselwin.cu.u   -row 1 -column 0 -sticky news
  grid .rselwin.cu.c   -row 1 -column 1 -sticky news

  # Create width frame
  frame .rselwin.width -relief raised -borderwidth 1
  checkbutton .rselwin.width.val -text "Limit line width to:" -variable rsel_wsel -anchor e -command {
    if {$rsel_wsel == 0} {
      .rselwin.width.w configure -state disabled
    } else {
      .rselwin.width.w configure -state normal
    }
  }
  entry .rselwin.width.w -textvariable rsel_width -width 3 -validate key -vcmd "check_width" -invalidcommand bell -state disabled
  grid .rselwin.width.val -row 0 -column 0 -sticky news
  grid .rselwin.width.w   -row 0 -column 1 -sticky news

  # Create filename frame
  frame .rselwin.fname -relief raised -borderwidth 1
  label .rselwin.fname.lbl -text "Save to file:" -anchor e
  entry .rselwin.fname.e -textvariable rsel_fname
  button .rselwin.fname.b -text "Browse..." -anchor e -command {
    set rsel_fname [tk_getSaveFile -filetypes $rpt_file_types -initialfile $rpt_name -title "Save Report As"]
  }
  grid .rselwin.fname.lbl -row 0 -column 0 -sticky news
  grid .rselwin.fname.e   -row 0 -column 1 -sticky news
  grid .rselwin.fname.b   -row 0 -column 2 -sticky news

  # Create button frame
  frame .rselwin.bf -relief raised -borderwidth 1
  button .rselwin.bf.create -width 10 -text "Create" -command {
    puts "Creating report..."
    destroy .rselwin
  }
  button .rselwin.bf.cancel -width 10 -text "Cancel" -command {
    destroy .rselwin
  }
  button .rselwin.bf.help -width 10 -text "Help" -command {
    help_show_manual gen_rpt
  }
  pack .rselwin.bf.help   -side right -padx 8 -pady 4
  pack .rselwin.bf.cancel -side right -padx 8 -pady 4
  pack .rselwin.bf.create -side right -padx 8 -pady 4

  # Now pack all of the frames
  grid .rselwin.sdv    -row 0 -column 0 -sticky w
  grid .rselwin.mi     -row 1 -column 0 -sticky w
  grid .rselwin.metric -row 2 -column 0 -sticky w
  grid .rselwin.cu     -row 3 -column 0 -sticky w
  grid .rselwin.width  -row 4 -column 0 -sticky w
  grid .rselwin.fname  -row 5 -column 0 -sticky w
  grid .rselwin.bf     -row 6 -column 0 -sticky w

  # Finally, raise this window
  raise .rselwin

}

proc check_width {} {

  return 1

}
