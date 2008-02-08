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

  # Add Mac OS X familiar shortcuts for the menubar
  if {[tk windowingsystem] eq "aqua"} {
    bind all <Command-o> {.menubar.file invoke 0}
    bind all <Command-s> {.menubar.file invoke 3}
    bind all <Command-w> {.menubar.file invoke 4}
  }

  bind all <Control-o> {.menubar.file invoke 0}
  bind all <Control-s> {.menubar.file invoke 3}
  bind all <Control-w> {.menubar.file invoke 4}
  bind all <Control-x> {.menubar.file invoke 8}

  bind all <Control-r> {.menubar.file.gen invoke 0}

  bind all <Control-n> {.menubar.view invoke 2}
  bind all <Control-p> {.menubar.view invoke 3}
  bind all <Control-c> {.menubar.view invoke 4}

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
    if {[open_files] != ""} {
      .menubar.file entryconfigure 1 -state normal
      .menubar.file entryconfigure 4 -state normal
      .menubar.file.gen entryconfigure 0 -state normal
      .menubar.view entryconfigure 0 -state normal
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
    if {[.menubar.file entrycget 3 -state] == "normal"} {
      set exit_status [tk_messageBox -message "Opened database has changed.  Would you like to save before closing?" \
                       -type yesnocancel -icon warning]
      if {$exit_status == "yes"} {
        .menubar.file invoke 3
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
    .menubar.file entryconfigure 1 -state disabled
    .menubar.file entryconfigure 3 -state disabled
    .menubar.file entryconfigure 4 -state disabled
    .menubar.file.gen entryconfigure 0 -state disabled
    .menubar.view entryconfigure 0 -state disabled
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

  # We don't need the exit function if we are running the Aqua version
  if {[tk windowingsystem] ne "aqua"} {
    # FILE - entry 7
    $tfm add separator
    # FILE - entry 8
    $tfm add command -label Exit -accelerator "Ctrl-x" -underline 1 -command {
      if {[.menubar.file entrycget 3 -state] == "normal"} {
        set exit_status [tk_messageBox -message "Opened database has changed.  Would you like to save before exiting?" \
                                       -type yesnocancel -icon warning]
        if {$exit_status == "yes"} {
          .menubar.file invoke 3
        } elseif {$exit_status == "cancel"} {
          return
        }
      }
      exit
    }
  }

  # Configure the report option
  set report [menu $mb.report -tearoff false]
  $mb add cascade -label "Report" -menu $report

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
  set mod_inst_type  "module"

  # Configure the color options
  set m [menu $mb.view -tearoff false]
  $mb add cascade -label "View" -menu $m

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
  $m add command -label "Preferences..." -underline 0 -command {
    create_preferences -1
  }

  # Configure the help option
  set thm [menu $mb.help -tearoff false]
  $mb add cascade -label "Help" -menu $thm

  # Add Manual and About information
  $thm add command -label "Manual" -state disabled -underline 0 -command {
    help_show_manual "contents"
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
        .menubar.file entryconfigure 3 -state normal
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
  global rsel_l rsel_t rsel_m rsel_c rsel_f rsel_a rsel_r
  global rsel_width rsel_wsel rsel_sup
  global rsel_fname cdd_name

  toplevel .rselwin
  wm title .rselwin "Create ASCII report"
  wm resizable .rselwin 0 0
  wm geometry .rselwin =550x250

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
  label .rselwin.of.sup_lbl    -anchor w -text "Suppress Empty Modules:"
  label .rselwin.of.sup_val    -anchor w -text ""
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
  grid .rselwin.of.sup_lbl     -row 6 -column 0 -sticky news -padx 12
  grid .rselwin.of.sup_val     -row 6 -column 1 -columnspan 2 -sticky news

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
    if {$rsel_wsel == 0} { set w "" } else { set w "-w $rsel_width" }
    if {$rsel_mi  == "None"} { set mi  "" } else { set mi  $rsel_mi }
    if {$rsel_cu  == "None"} { set cu  "" } else { set cu  $rsel_cu }
    if {$rsel_sup == "None"} { set sup "" } else { set sup $rsel_sup }
    if {$rsel_l   == "None"} { set l   "" } else { set l   $rsel_l }
    if {$rsel_t   == "None"} { set t   "" } else { set t   $rsel_t }
    if {$rsel_m   == "None"} { set m   "" } else { set m   $rsel_m }
    if {$rsel_c   == "None"} { set c   "" } else { set c   $rsel_c }
    if {$rsel_f   == "None"} { set f   "" } else { set f   $rsel_f }
    if {$rsel_a   == "None"} { set a   "" } else { set a   $rsel_a }
    if {$rsel_r   == "None"} { set r   "" } else { set r   $rsel_r }
    set cmd "-d $rsel_sdv $mi $cu -m $l$t$m$c$f$a$r $w -o $rsel_fname $sup $cdd_name"
    puts "cmd: $cmd"
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
  global rsel_l rsel_t rsel_m rsel_c rsel_f rsel_a rsel_r
  global rsel_width rsel_wsel rsel_sup
  global rsel_fname cdd_name

  if {[winfo exists .rselwin] == 1} {

    # Create human-readable versions of these values
    switch $rsel_sdv {
      s { set sdv_name "Summary" }
      d { set sdv_name "Detailed" }
      v { set sdv_name "Verbose" }
    }
    switch -- $rsel_mi {
      "None" { set mi_name "Module" }
      "-i"   { set mi_name "Instance" }
    }
    switch -- $rsel_cu {
      "None" { set cu_name "Uncovered" }
      "-c"   { set cu_name "Covered" }
    }
    if {$rsel_l == "l"} {
      lappend metric_list "Line"
    }
    if {$rsel_t == "t"} {
      lappend metric_list "Toggle"
    }
    if {$rsel_m == "m"} {
      lappend metric_list "Memory"
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
    if {$rsel_sup == ""} {
      set sup_name "No"
    } else {
      set sup_name "Yes"
    }

    # Set the labels with the appropriate values
    .rselwin.of.sdv_val    configure -text $sdv_name
    .rselwin.of.mi_val     configure -text $mi_name
    .rselwin.of.metric_val configure -text [join $metric_list ,]
    .rselwin.of.cu_val     configure -text $cu_name
    .rselwin.of.width_val  configure -text $width_name
    .rselwin.of.sup_val    configure -text $sup_name

  }

}

