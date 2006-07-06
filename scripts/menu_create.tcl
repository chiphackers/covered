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
    puts "Closed all opened/merged CDD files"
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
    create_preferences -1
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
  global rsel_fname cdd_name

  toplevel .rselwin
  wm title .rselwin "Create ASCII report"
  wm resizable .rselwin 0 0
  wm geometry .rselwin =450x220

  # Create default report filename
  set rsel_fname "[file rootname $cdd_name].rpt"

  # Create selected options window
  frame .rselwin.of -relief raised -borderwidth 1
  label .rselwin.of.lbl -text "Selected ASCII Report Options" -anchor w
  button .rselwin.of.b -text "Edit Options..." -command {
    create_preferences 3
  }
  label .rselwin.of.sdv_lbl    -anchor w -text "Level of Detail:"
  label .rselwin.of.sdv_val    -anchor w -text ""
  label .rselwin.of.mi_lbl     -anchor w -text "Accumulated By:"
  label .rselwin.of.mi_val     -anchor w -text ""
  label .rselwin.of.metric_lbl -anchor w -text "Show Metrics:"
  label .rselwin.of.metric_val -anchor w -text ""
  label .rselwin.of.cu_lbl     -anchor w -text "Coverage Type:"
  label .rselwin.of.cu_val     -anchor w -text ""
  label .rselwin.of.width_lbl  -anchor w -text "Line Width:"
  label .rselwin.of.width_val  -anchor w -text ""
  grid columnconfigure .rselwin.of 1 -weight 1
  grid .rselwin.of.lbl         -row 0 -column 0 -columnspan 2 -sticky news -pady 4
  grid .rselwin.of.b           -row 0 -column 2 -sticky news
  grid .rselwin.of.sdv_lbl     -row 1 -column 0 -sticky news -padx 12
  grid .rselwin.of.sdv_val     -row 1 -column 1 -columnspan 2 -sticky news
  grid .rselwin.of.mi_lbl      -row 2 -column 0 -sticky news -padx 12
  grid .rselwin.of.mi_val      -row 2 -column 1 -columnspan 2 -sticky news
  grid .rselwin.of.metric_lbl  -row 3 -column 0 -sticky news -padx 12
  grid .rselwin.of.metric_val  -row 3 -column 1 -columnspan 2 -sticky news
  grid .rselwin.of.cu_lbl      -row 4 -column 0 -sticky news -padx 12
  grid .rselwin.of.cu_val      -row 4 -column 1 -columnspan 2 -sticky news
  grid .rselwin.of.width_lbl   -row 5 -column 0 -sticky news -padx 12
  grid .rselwin.of.width_val   -row 5 -column 1 -columnspan 2 -sticky news

  # Create filename frame
  frame .rselwin.fname -relief raised -borderwidth 1
  label .rselwin.fname.lbl -text "Save to file:" -anchor e
  entry .rselwin.fname.e -textvariable rsel_fname
  button .rselwin.fname.b -text "Browse..." -anchor e -command {
    set rsel_fname [tk_getSaveFile -filetypes $rpt_file_types -initialfile $rsel_fname -title "Save Generated Report As"]
  }
  grid .rselwin.fname.lbl -row 0 -column 0 -sticky news -pady 4
  grid .rselwin.fname.e   -row 0 -column 1 -sticky news -pady 4
  grid .rselwin.fname.b   -row 0 -column 2 -sticky news -pady 4
  grid columnconfigure .rselwin.fname 1 -weight 1

  # Create button frame
  frame .rselwin.bf -relief raised -borderwidth 1
  button .rselwin.bf.create -width 10 -text "Create" -command {
    # Create command-line to report command of Covered
    if {$rsel_wsel == 0} {
      set w ""
    } else {
      set w "-w $rsel_width"
    }
    set cmd "-d $rsel_sdv $rsel_mi $rsel_cu -m $rsel_l$rsel_t$rsel_c$rsel_f$rsel_a$rsel_r $w -o $rsel_fname $cdd_name"
    eval "tcl_func_generate_report $cmd"
    destroy .rselwin
  }
  button .rselwin.bf.cancel -width 10 -text "Cancel" -command {
    destroy .rselwin
  }
  button .rselwin.bf.help -width 10 -text "Help" -command {
    help_show_manual report_gen
  }
  pack .rselwin.bf.help   -side right -padx 8 -pady 4
  pack .rselwin.bf.cancel -side right -padx 8 -pady 4
  pack .rselwin.bf.create -side right -padx 8 -pady 4

  # Now pack all of the frames
  pack .rselwin.of    -fill both -side top
  pack .rselwin.fname -fill both -expand 1
  pack .rselwin.bf    -fill both -side bottom

  # Populate the report select window
  update_report_select

  # Finally, raise this window
  raise .rselwin

}

proc update_report_select {} {

  global rsel_sdv rsel_mi rsel_cu
  global rsel_l rsel_t rsel_c rsel_f rsel_a rsel_r
  global rsel_width rsel_wsel
  global rsel_fname cdd_name

  if {[winfo exists .rselwin] == 1} {

    # Create human-readable versions of these values
    switch $rsel_sdv {
      s { set sdv_name "Summary" }
      d { set sdv_name "Detailed" }
      v { set sdv_name "Verbose" }
    }
    switch -- $rsel_mi {
      ""   { set mi_name "Module" }
      "-i" { set mi_name "Instance" }
    }
    switch -- $rsel_cu {
      ""   { set cu_name "Uncovered" }
      "-c" { set cu_name "Covered" }
    }
    if {$rsel_l == "l"} {
      lappend metric_list "Line"
    }
    if {$rsel_t == "t"} {
      lappend metric_list "Toggle"
    }
    if {$rsel_c == "c"} {
      lappend metric_list "Logic"
    }
    if {$rsel_f == "f"} {
      lappend metric_list "FSM"
    }
    if {$rsel_a == "a"} {
      lappend metric_list "Assertion"
    }
    if {$rsel_r == "r"} {
      lappend metric_list "Race Conditions"
    }
    if {[llength metric_list] == 0} {
      set metric_list [list None]
    }
    if {$rsel_wsel == 1} {
      set width_name "$rsel_width characters"
    } else {
      set width_name "Preformatted"
    }

    # Set the labels with the appropriate values
    .rselwin.of.sdv_val    configure -text $sdv_name
    .rselwin.of.mi_val     configure -text $mi_name
    .rselwin.of.metric_val configure -text [join $metric_list ,]
    .rselwin.of.cu_val     configure -text $cu_name
    .rselwin.of.width_val  configure -text $width_name

  }

}

