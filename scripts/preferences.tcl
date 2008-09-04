#!/usr/bin/env wish

##################################
# Manages the preferences window #
##################################

# Global variables (eventually we will want to get/save these to a configuration file!)
set main_geometry           ""
set show_wizard             true
set save_gui_on_exit        false
set show_tooltips           true
set saved_gui               0
set uncov_fgColor           blue
set uncov_bgColor           yellow
set cov_fgColor             black
set cov_bgColor             white
set race_fgColor            white
set race_bgColor            blue
set line_low_limit          90
set toggle_low_limit        90
set memory_low_limit        90
set comb_low_limit          90
set fsm_low_limit           90
set assert_low_limit        90
set vlog_hl_mode            on
set vlog_hl_ppkeyword_color ForestGreen
set vlog_hl_keyword_color   purple
set vlog_hl_comment_color   blue
set vlog_hl_value_color     red
set vlog_hl_string_color    red
set vlog_hl_symbol_color    coral
set mod_inst_tl_width       ""
set mod_inst_tl_columns     {0 {Instance Name} 0 {Module Name} 0 {Hit} right 0 {Miss} right 0 {Excluded} right 0 {Total} right 0 {Hit %} right 0 {Index}}
set mod_inst_tl_init_hidden {1 0 0 0 0 0 0}
set rc_file_to_write        ""
set hl_mode                 0
set last_pref_index         -1
set exclude_reasons_enabled 1

# Create a list from 100 to 0
for {set i 100} {$i >= 0} {incr i -1} {
  lappend percent_range $i
}

proc read_coveredrc {} {

  global show_wizard save_gui_on_exit show_tooltips
  global uncov_fgColor uncov_bgColor
  global cov_fgColor   cov_bgColor
  global race_fgColor  race_bgColor
  global line_low_limit toggle_low_limit memory_low_limit comb_low_limit fsm_low_limit assert_low_limit
  global vlog_hl_mode vlog_hl_ppkeyword_color vlog_hl_keyword_color
  global vlog_hl_comment_color vlog_hl_string_color vlog_hl_value_color
  global vlog_hl_symbol_color
  global HOME USER_HOME rc_file_to_write
  global main_geometry toggle_geometry memory_geometry comb_geometry fsm_geometry assert_geometry
  global mod_inst_tl_columns mod_inst_type mod_inst_tl_init_hidden mod_inst_tl_width

  # Find the correct configuration file to read and eventually write
  if {[file exists ".coveredrc"] == 1} {
    set rc [open ".coveredrc" r]
    set rc_file_to_write ".coveredrc"
  } elseif {[file exists [file join $USER_HOME .coveredrc]] == 1} {
    set rc [open [file join $USER_HOME .coveredrc] r]
    set rc_file_to_write [file join $USER_HOME .coveredrc]
  } elseif {[file exists [file join $HOME .coveredrc]] == 1} {
    set rc [open [file join $HOME .coveredrc] r]
    set rc_file_to_write [file join $USER_HOME .coveredrc]
  } else {
    set rc -1
    set rc_file_to_write ""
  }

  if {$rc != -1} {

    seek $rc 0 start

    while {[eof $rc] == 0} {

      set line [gets $rc]

      # If the line is not a comment and contains an equal sign, parse it for information
      if {[string index $line 0] != "\#" && [string first "=" $line] != -1} {

        # Parse string here
        set line_elems [split $line =]
        set field      [string trim [lindex $line_elems 0]]
        set value      [string trim [lindex $line_elems 1]]

        if {$field == "ShowWizardOnStartup"} {
          set show_wizard $value
        } elseif {$field == "SaveGuiOnExit"} {
          set save_gui_on_exit $value
        } elseif {$field == "ShowTooltips"} {
          set show_tooltips $value
        } elseif {$field == "UncoveredForegroundColor"} {
          set uncov_fgColor $value
        } elseif {$field == "UncoveredBackgroundColor"} {
          set uncov_bgColor $value
        } elseif {$field == "CoveredForegroundColor"} {
          set cov_fgColor $value
        } elseif {$field == "CoveredBackgroundColor"} {
          set cov_bgColor $value
        } elseif {$field == "RaceConditionForegroundColor"} {
          set race_fgColor $value
        } elseif {$field == "RaceConditionBackgroundColor"} {
          set race_bgColor $value
        } elseif {$field == "AcceptableLinePercentage"} {
          set line_low_limit $value
        } elseif {$field == "AcceptableTogglePercentage"} {
          set toggle_low_limit $value
        } elseif {$field == "AcceptableMemoryPercentage"} {
          set memory_low_limit $value
        } elseif {$field == "AcceptableCombinationalLogicPercentage"} {
          set comb_low_limit $value
        } elseif {$field == "AcceptableFsmPercentage"} {
          set fsm_low_limit $value
        } elseif {$field == "AcceptableAssertionPercentage"} {
          set assert_low_limit $value
        } elseif {$field == "HighlightingMode"} {
          set vlog_hl_mode $value
        } elseif {$field == "HighlightPreprocessorKeywordColor"} {
          set vlog_hl_ppkeyword_color $value
        } elseif {$field == "HighlightKeywordColor"} {
          set vlog_hl_keyword_color $value
        } elseif {$field == "HighlightCommentColor"} {
          set vlog_hl_comment_color $value
        } elseif {$field == "HighlightValueColor"} {
          set vlog_hl_value_color $value
        } elseif {$field == "HighlightStringColor"} {
          set vlog_hl_string_color $value
        } elseif {$field == "HighlightSymbolColor"} {
          set vlog_hl_symbol_color $value

        # The following are GUI state-saved information -- only use this information if SaveGuiOnExit was set to true
        } elseif {$save_gui_on_exit == "true"} {
          if {$field == "MainWindowGeometry"} {
            set main_geometry $value
          } elseif {$field == "ToggleWindowGeometry"} {
            set toggle_geometry $value
          } elseif {$field == "MemoryWindowGeometry"} {
            set memory_geometry $value
          } elseif {$field == "CombWindowGeometry"} {
            set comb_geometry $value
          } elseif {$field == "FsmWindowGeometry"} {
            set fsm_geometry $value
          } elseif {$field == "AssertWindowGeometry"} {
            set assert_geometry $value
          } elseif {$field == "ModuleInstanceType"} {
            set mod_inst_type $value
          } elseif {$field == "ModuleInstanceTableListWidth"} {
            set mod_inst_tl_width $value
          } elseif {$field == "ModuleInstanceTableListColumns"} {
            set mod_inst_tl_columns [split $value :]
          } elseif {$field == "ModuleInstanceTableListInitHidden"} {
            set mod_inst_tl_init_hidden [split $value :]
          }
        }

      }
        
    }

    close $rc

  }

}

proc write_coveredrc {exiting} {

  global show_wizard save_gui_on_exit show_tooltips
  global uncov_fgColor uncov_bgColor
  global cov_fgColor   cov_bgColor
  global race_fgColor  race_bgColor
  global line_low_limit toggle_low_limit memory_low_limit comb_low_limit fsm_low_limit assert_low_limit
  global vlog_hl_mode vlog_hl_ppkeyword_color vlog_hl_keyword_color
  global vlog_hl_comment_color vlog_hl_string_color vlog_hl_value_color
  global vlog_hl_symbol_color
  global rc_file_to_write
  global mod_inst_tl_columns mod_inst_type tableColHide
  global main_geometry toggle_geometry memory_geometry comb_geometry fsm_geometry assert_geometry

  if {$rc_file_to_write != ""} {

    set rc [open $rc_file_to_write w]

    puts $rc "# Covered GUI Configuration File"
    puts $rc "#--------------------------------"
    puts $rc "# All variable assignments below are in the form of \"variable = value\""
    puts $rc "# where whitespace must be present between the variable, the \"=\" character"
    puts $rc "# and the value.  Comment lines start with the \"#\" character.\n"

    puts $rc "# If set to true, causes the wizard window to be displayed on GUI startup."
    puts $rc "# If set to false, the wizard window is suppressed.\n"

    puts $rc "ShowWizardOnStartup = $show_wizard\n"

    puts $rc "# If set to true, causes various GUI elements to be saved when exiting the application"
    puts $rc "# which will be used when the application is restarted at a later time.  Set to false"
    puts $rc "# to cause the Covered GUI to use the default values.\n"

    puts $rc "SaveGuiOnExit = $save_gui_on_exit\n"

    puts $rc "# If set to true, causes tooltips to be used for many widgets within the GUI.  If set"
    puts $rc "# to false, causes all tooltip support to be disabled."

    puts $rc "ShowTooltips = $show_tooltips\n"

    puts $rc "# Sets the foreground color for all source code that is found"
    puts $rc "# to be uncovered during simulation.  The value can be any legal color"
    puts $rc "# value accepted by Tcl.\n"

    puts $rc "UncoveredForegroundColor = $uncov_fgColor\n"

    puts $rc "# Sets the background color for all source code that is found"
    puts $rc "# to be uncovered during simulation.  The value can be any legal color"
    puts $rc "# value accepted by Tcl.\n"

    puts $rc "UncoveredBackgroundColor = $uncov_bgColor\n"

    puts $rc "# Sets the foreground color for all source code that is found"
    puts $rc "# to be covered during simulation.  The value can be any legal color value"
    puts $rc "# accepted by Tcl.\n"

    puts $rc "CoveredForegroundColor = $cov_fgColor\n"

    puts $rc "# Sets the background color for all source code that is found"
    puts $rc "# to be covered during simulation.  The value can be any legal color value."
    puts $rc "# accepted by Tcl.\n"

    puts $rc "CoveredBackgroundColor = $cov_bgColor\n"

    puts $rc "# Sets the foreground color for all source code that has been detected as"
    puts $rc "# containing a race condition situation.  This code is not analyzed by Covered."
    puts $rc "# The value can be any legal color value accepted by Tcl.\n"

    puts $rc "RaceConditionForegroundColor = $race_fgColor\n"

    puts $rc "# Sets the background color for all source code that has been detected as"
    puts $rc "# containing a race condition situation.  This code is not analyzed by Covered."
    puts $rc "# The value can be any legal color value accepted by Tcl.\n"

    puts $rc "RaceConditionBackgroundColor = $race_bgColor\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved a line"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the line coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableLinePercentage = $line_low_limit\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved a toggle"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the toggle coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableTogglePercentage = $toggle_low_limit\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved a memory"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the memory coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableMemoryPercentage = $memory_low_limit\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved a combinational"
    puts $rc "# logic coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the combinational logic coverage can possibly be"
    puts $rc "# deemed \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableCombinationalLogicPercentage = $comb_low_limit\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved an FSM state/arc"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the FSM coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableFsmPercentage = $fsm_low_limit\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved an assertion"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the assertion coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableAssertionPercentage = $assert_low_limit\n"

    puts $rc "# Causes syntax highlighting to be turned on or off.  This value should either be"
    puts $rc "# 'on' or 'off'.\n"

    puts $rc "HighlightingMode = $vlog_hl_mode\n"

    puts $rc "# When the syntax highlighting mode is on, causes the preprocessor directive keywords"
    puts $rc "# to be highlighted with the specified color.  The value may be any color value that"
    puts $rc "# is acceptable to Tk.\n"

    puts $rc "HighlightPreprocessorKeywordColor = $vlog_hl_ppkeyword_color\n"

    puts $rc "# When the syntax highlighting mode is on, causes the Verilog keywords to be highlighted"
    puts $rc "# with the specified color.  The value may be any color value that is acceptable to Tk.\n"

    puts $rc "HighlightKeywordColor = $vlog_hl_keyword_color\n"

    puts $rc "# When the syntax highlighting mode is on, causes any single or multi-line comments to"
    puts $rc "# be highlighted with the specified color.  The value may be any color value that is"
    puts $rc "# acceptable to Tk.\n"

    puts $rc "HighlightCommentColor = $vlog_hl_comment_color\n"

    puts $rc "# When the syntax highlighting mode is on, causes any defined, binary, octal, decimal or"
    puts $rc "# hexidecimal value to be highlighted with the specified color.  The value may be any"
    puts $rc "# color value that is acceptable to Tk.\n"

    puts $rc "HighlightValueColor = $vlog_hl_value_color\n"

    puts $rc "# When the syntax highlighting mode is on, causes any quoted string to be highlighted with"
    puts $rc "# the specified color.  The value may be any color value that is acceptable to Tk.\n"

    puts $rc "HighlightStringColor = $vlog_hl_string_color\n"

    puts $rc "# When the syntax highlighting mode is on, causes any Verilog symbol to be highlighted with"
    puts $rc "# the specified color.  The value may be any color value that is acceptable to Tk.\n"

    puts $rc "HighlightSymbolColor = $vlog_hl_symbol_color\n"

    puts $rc "# THE FOLLOWING LINES ARE FOR STORING GUI STATE INFORMATION -- DO NOT MODIFY LINES BELOW THIS COMMENT!\n"

    if {$save_gui_on_exit == true && $exiting == 1} {

      for {set i 0} {$i < 7} {incr i} {
        lappend tl_init_hidden [.bot.left.tl columncget $i -hide]
      }

      puts $rc "MainWindowGeometry                = [winfo geometry .]"
      puts $rc "ToggleWindowGeometry              = $toggle_geometry"
      puts $rc "MemoryWindowGeometry              = $memory_geometry"
      puts $rc "CombWindowGeometry                = $comb_geometry"
      puts $rc "FsmWindowGeometry                 = $fsm_geometry"
      puts $rc "AssertWindowGeometry              = $assert_geometry"
      puts $rc "ModuleInstanceType                = $mod_inst_type"
      puts $rc "ModuleInstanceTableListWidth      = [winfo width .bot.left]"
      puts $rc "ModuleInstanceTableListColumns    = [join [.bot.left.tl cget -columns] :]"
      puts $rc "ModuleInstanceTableListInitHidden = [join $tl_init_hidden :]"

    }

    flush $rc
    close $rc

  }

}

proc create_preferences {start_index} {

  global show_wizard      tmp_show_wizard
  global save_gui_on_exit tmp_save_gui_on_exit
  global show_tooltips    tmp_show_tooltips
  global cov_fgColor   cov_bgColor   tmp_cov_fgColor   tmp_cov_bgColor
  global uncov_fgColor uncov_bgColor tmp_uncov_fgColor tmp_uncov_bgColor
  global race_fgColor  race_bgColor  tmp_race_fgColor  tmp_race_bgColor
  global line_low_limit          tmp_line_low_limit
  global toggle_low_limit        tmp_toggle_low_limit
  global memory_low_limit        tmp_memory_low_limit
  global comb_low_limit          tmp_comb_low_limit
  global fsm_low_limit           tmp_fsm_low_limit
  global assert_low_limit        tmp_assert_low_limit
  global vlog_hl_mode            tmp_vlog_hl_mode
  global vlog_hl_ppkeyword_color tmp_vlog_hl_ppkeyword_color
  global vlog_hl_keyword_color   tmp_vlog_hl_keyword_color
  global vlog_hl_comment_color   tmp_vlog_hl_comment_color
  global vlog_hl_value_color     tmp_vlog_hl_value_color
  global vlog_hl_string_color    tmp_vlog_hl_string_color
  global vlog_hl_symbol_color    tmp_vlog_hl_symbol_color
  global hl_mode last_pref_index

  # Now create the window and set the grab to this window
  if {[winfo exists .prefwin] == 0} {

    # Initialize all temporary preference values
    set tmp_show_wizard             $show_wizard
    set tmp_save_gui_on_exit        $save_gui_on_exit
    set tmp_show_tooltips           $show_tooltips
    set tmp_cov_fgColor             $cov_fgColor
    set tmp_cov_bgColor             $cov_bgColor
    set tmp_uncov_fgColor           $uncov_fgColor
    set tmp_uncov_bgColor           $uncov_bgColor
    set tmp_race_fgColor            $race_fgColor
    set tmp_race_bgColor            $race_bgColor
    set tmp_line_low_limit          $line_low_limit
    set tmp_toggle_low_limit        $toggle_low_limit
    set tmp_memory_low_limit        $memory_low_limit
    set tmp_comb_low_limit          $comb_low_limit
    set tmp_fsm_low_limit           $fsm_low_limit
    set tmp_assert_low_limit        $assert_low_limit
    set tmp_vlog_hl_mode            $vlog_hl_mode
    set tmp_vlog_hl_ppkeyword_color $vlog_hl_ppkeyword_color
    set tmp_vlog_hl_keyword_color   $vlog_hl_keyword_color
    set tmp_vlog_hl_comment_color   $vlog_hl_comment_color
    set tmp_vlog_hl_value_color     $vlog_hl_value_color
    set tmp_vlog_hl_string_color    $vlog_hl_string_color
    set tmp_vlog_hl_symbol_color    $vlog_hl_symbol_color

    # Specify that there was no last index selected
    set last_pref_index -1

    # Create new window
    toplevel .prefwin
    wm title .prefwin "Covered - Preferences"
    # wm resizable .prefwin 0 0
    wm geometry .prefwin =600x450

    # Create listbox frame
    frame .prefwin.lbf -relief raised -borderwidth 1
    label .prefwin.lbf.l -text "Option Categories"
    listbox .prefwin.lbf.lb
    bind .prefwin.lbf.lb <<ListboxSelect>> populate_pref

    # Populate listbox with various options
    .prefwin.lbf.lb insert end "General"
    .prefwin.lbf.lb insert end "Colors"
    .prefwin.lbf.lb insert end "Coverage Goals"
    .prefwin.lbf.lb insert end "Syntax Highlighting"

    pack .prefwin.lbf.l  -pady 4
    pack .prefwin.lbf.lb -fill y -expand 1

    # Create preference frame
    frame .prefwin.pf -relief raised -borderwidth 1
    frame .prefwin.pf.f
    label .prefwin.pf.f.l -text "- Select an option category on the left to view/edit.\n\n- Click the \"Apply button\" to apply all preference options\nto the rest of the GUI.\n\n- Click the \"OK\" button to apply and save the changes to\nthe Covered configuration file and exit this window.\n\n- Click the \"Cancel\" button to forget all preference changes\nmade the exit this window.\n\n- Click the \"Help\" button to get help for the shown option window." -justify left
    pack .prefwin.pf.f.l -fill y -padx 8 -pady 10 
    pack .prefwin.pf.f -fill both

    # Create button frame
    frame .prefwin.bf -relief raised -borderwidth 1
    button .prefwin.bf.apply -width 10 -text "Apply" -command {
      apply_preferences
    }
    button .prefwin.bf.ok -width 10 -text "OK" -command {
      if {[apply_preferences] == 1} {
        write_coveredrc 0
      }
      destroy .prefwin
    }
    button .prefwin.bf.cancel -width 10 -text "Cancel" -command {
      destroy .prefwin
    }
    help_button .prefwin.bf.help ""
    .prefwin.bf.help configure -command {
      switch [.prefwin.lbf.lb curselection] {
        0 { help_show_manual chapter.gui.preferences "section.gui.pref.general" }
        1 { help_show_manual chapter.gui.preferences "section.gui.pref.color" }
        2 { help_show_manual chapter.gui.preferences "section.gui.pref.goals" }
        3 { help_show_manual chapter.gui.preferences "section.gui.pref.syntax" }
        default { help_show_manual chapter.gui.preferences "" }
      }
    }
    pack .prefwin.bf.help   -side right -pady 4
    pack .prefwin.bf.cancel -side right -padx 8 -pady 4
    pack .prefwin.bf.ok     -side right -padx 8 -pady 4
    pack .prefwin.bf.apply  -side right -padx 8 -pady 4

    # Pack frames
    grid rowconfigure    .prefwin 0 -weight 1
    grid columnconfigure .prefwin 1 -weight 1
    grid .prefwin.lbf -row 0 -column 0 -sticky news
    grid .prefwin.pf  -row 0 -column 1 -sticky news
    grid .prefwin.bf  -row 1 -column 0 -columnspan 2 -sticky news

  }

  # If the user specified a starting index value, activate it.
  if {[expr $start_index >= 0] && [expr $start_index <= 3]} {
    .prefwin.lbf.lb selection set $start_index
    populate_pref
  }

  # Bring the window to the top
  raise .prefwin

}

# Causes the state of the syntax color widgets to get set according to the current hl_mode value.
proc synchronize_syntax_widgets {hl_mode} {
  
  if {$hl_mode == "on"} {
    .prefwin.pf.f.ppcl configure -state normal
    .prefwin.pf.f.ppcb configure -state normal
    .prefwin.pf.f.pcl  configure -state normal
    .prefwin.pf.f.pcb  configure -state normal
    .prefwin.pf.f.ccl  configure -state normal
    .prefwin.pf.f.ccb  configure -state normal
    .prefwin.pf.f.vcl  configure -state normal
    .prefwin.pf.f.vcb  configure -state normal
    .prefwin.pf.f.stcl configure -state normal
    .prefwin.pf.f.stcb configure -state normal
    .prefwin.pf.f.sycl configure -state normal
    .prefwin.pf.f.sycb configure -state normal
  } else {
    .prefwin.pf.f.ppcl configure -state disabled
    .prefwin.pf.f.ppcb configure -state disabled
    .prefwin.pf.f.pcl  configure -state disabled
    .prefwin.pf.f.pcb  configure -state disabled
    .prefwin.pf.f.ccl  configure -state disabled
    .prefwin.pf.f.ccb  configure -state disabled
    .prefwin.pf.f.vcl  configure -state disabled
    .prefwin.pf.f.vcb  configure -state disabled
    .prefwin.pf.f.stcl configure -state disabled
    .prefwin.pf.f.stcb configure -state disabled
    .prefwin.pf.f.sycl configure -state disabled
    .prefwin.pf.f.sycb configure -state disabled
  }
        
}

# Checks to see if any of the global preferences have been modified in the preferences window.
# If changes have occurred, the displays are immediately updated to reflect these new values.
# A value of 0 is returned if no changes were found; otherwise, a value of 1 is returned.
proc apply_preferences {} {

  global show_wizard      tmp_show_wizard
  global save_gui_on_exit tmp_save_gui_on_exit
  global show_tooltips    tmp_show_tooltips
  global cov_fgColor      tmp_cov_fgColor
  global cov_bgColor      tmp_cov_bgColor
  global uncov_fgColor    tmp_uncov_fgColor
  global uncov_bgColor    tmp_uncov_bgColor
  global race_fgColor     tmp_race_fgColor
  global race_bgColor     tmp_race_bgColor
  global line_low_limit   tmp_line_low_limit
  global toggle_low_limit tmp_toggle_low_limit
  global memory_low_limit tmp_memory_low_limit
  global comb_low_limit   tmp_comb_low_limit
  global fsm_low_limit    tmp_fsm_low_limit
  global assert_low_limit tmp_assert_low_limit
  global cov_rb
  global vlog_hl_mode            tmp_vlog_hl_mode
  global vlog_hl_ppkeyword_color tmp_vlog_hl_ppkeyword_color
  global vlog_hl_keyword_color   tmp_vlog_hl_keyword_color
  global vlog_hl_comment_color   tmp_vlog_hl_comment_color
  global vlog_hl_value_color     tmp_vlog_hl_value_color
  global vlog_hl_string_color    tmp_vlog_hl_string_color
  global vlog_hl_symbol_color    tmp_vlog_hl_symbol_color

  # Save spinner values to temporary storage items
  save_spinners [.prefwin.lbf.lb curselection]

  set changed 0

  # Check for changes and update global preference variables accordingly
  if {$show_wizard != $tmp_show_wizard} {
    set show_wizard $tmp_show_wizard
    set changed 1
  }
  if {$save_gui_on_exit != $tmp_save_gui_on_exit} {
    set save_gui_on_exit $tmp_save_gui_on_exit
    set changed 1
  }
  if {$show_tooltips != $tmp_show_tooltips} {
    set show_tooltips $tmp_show_tooltips
    set changed 1
  }
  if {$cov_fgColor != $tmp_cov_fgColor} {
    set cov_fgColor $tmp_cov_fgColor
    set changed 1
  }
  if {$cov_bgColor != $tmp_cov_bgColor} {
    set cov_bgColor $tmp_cov_bgColor
    set changed 1
  }
  if {$uncov_fgColor != $tmp_uncov_fgColor} {
    set uncov_fgColor $tmp_uncov_fgColor
    set changed 1
  }
  if {$uncov_bgColor != $tmp_uncov_bgColor} {
    set uncov_bgColor $tmp_uncov_bgColor
    set changed 1
  }
  if {$race_fgColor != $tmp_race_fgColor} {
    set race_fgColor $tmp_race_fgColor
    set changed 1
  }
  if {$race_bgColor != $tmp_race_bgColor} {
    set race_bgColor $tmp_race_bgColor
    set changed 1
  }
  if {$line_low_limit != $tmp_line_low_limit} {
    set line_low_limit $tmp_line_low_limit
    set changed 1
  }
  if {$toggle_low_limit != $tmp_toggle_low_limit} {
    set toggle_low_limit $tmp_toggle_low_limit
    set changed 1
  }
  if {$memory_low_limit != $tmp_memory_low_limit} {
    set memory_low_limit $tmp_memory_low_limit
    set changed 1
  }
  if {$comb_low_limit != $tmp_comb_low_limit} {
    set comb_low_limit $tmp_comb_low_limit
    set changed 1
  }
  if {$fsm_low_limit != $tmp_fsm_low_limit} {
    set fsm_low_limit $tmp_fsm_low_limit
    set changed 1
  }
  if {$assert_low_limit != $tmp_assert_low_limit} {
    set assert_low_limit $tmp_assert_low_limit
    set changed 1
  }
  if {$vlog_hl_mode != $tmp_vlog_hl_mode} {
    set vlog_hl_mode $tmp_vlog_hl_mode
    set changed 1
  }
  if {$vlog_hl_ppkeyword_color != $tmp_vlog_hl_ppkeyword_color} {
    set vlog_hl_ppkeyword_color $tmp_vlog_hl_ppkeyword_color
    set changed 1
  }
  if {$vlog_hl_keyword_color != $tmp_vlog_hl_keyword_color} {
    set vlog_hl_keyword_color $tmp_vlog_hl_keyword_color
    set changed 1
  }
  if {$vlog_hl_comment_color != $tmp_vlog_hl_comment_color} {
    set vlog_hl_comment_color $tmp_vlog_hl_comment_color
    set changed 1
  }
  if {$vlog_hl_value_color != $tmp_vlog_hl_value_color} {
    set vlog_hl_value_color $tmp_vlog_hl_value_color
    set changed 1
  }
  if {$vlog_hl_string_color != $tmp_vlog_hl_string_color} {
    set vlog_hl_string_color $tmp_vlog_hl_string_color
    set changed 1
  }
  if {$vlog_hl_symbol_color != $tmp_vlog_hl_symbol_color} {
    set vlog_hl_symbol_color $tmp_vlog_hl_symbol_color
    set changed 1
  }

  # Update the display if necessary
  if {$changed == 1} {

    # Redisplay with new settings
    set text_x [.bot.right.txt xview]
    set text_y [.bot.right.txt yview]

    # Redisplay the information in the main file viewer
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

    # Redisplay the combinational logic window, if it exists
    if {[winfo exists .combwin] != 0} {
      display_comb_info
    }

    # Redisplay the FSM logic window, if it exists
    if {[winfo exists .fsmwin] != 0} {
      display_fsm_table
    }

    .bot.right.txt xview moveto [lindex $text_x 0]
    .bot.right.txt yview moveto [lindex $text_y 0]

    # Update the listbox
    populate_listbox

  }

  return $changed

}

proc pref_set_label_color {clabel type origcolor title} {

  # Get color from color chooser window
  set color [tk_chooseColor -initialcolor $origcolor -title $title]

  # If a color was choosen, set the buttons
  if {$color != ""} {

    # Set foreground or background colors on buttons
    if {$type == "fg"} {
      $clabel configure -fg $color
    } else {
      $clabel configure -bg $color
    }

    return $color

  # Otherwise, just return the original color
  } else {

    return $origcolor

  }

}

proc percent_spinner {w showval} {

  global percent_range

  # If the Tk version is 8.5 or higher, the spinbox widget exists so make use of it
  if {[info tclversion] >= 8.4} {

    spinbox $w -from 0 -to 100 -increment 1.0 -width 3 -state readonly
    $w set $showval

    return $w

  } else {

    # Create up arrow
    set im(up) [image create bitmap -data "
      #define i_width 5\n#define i_height 3\nstatic char i_bits[] = {\n4,14,31}"]

    # Create down arrow
    set im(dn) [image create bitmap -data "
    #define i_width 5\n#define i_height 3\nstatic char i_bits[] = {\n31,14,4}"]

    # Create the spinner widgets
    frame $w
    listbox $w.l -height 1 -width 3 -listvar percent_range -relief flat
    $w.l configure -selectbackground [$w.l cget -bg] -selectforeground [$w.l cget -fg]
    frame $w.f
    button $w.f.1 -image $im(up) -width 10 -height 4 -command [list $w.l yview scroll -1 unit]
    button $w.f.2 -image $im(dn) -width 10 -height 4 -command [list $w.l yview scroll 1 unit]

    # Pack the widgets
    pack $w.f.1 $w.f.2
    pack $w.l $w.f -side left -fill y

    # Display current percentage value
    $w.l see [expr 100 - $showval]

    return $w.l

  }

}

proc populate_pref {} {

  global last_pref_index

  # Save spinner values before proceeding
  save_spinners $last_pref_index

  # Get the current selection
  set index [.prefwin.lbf.lb curselection]

  if {$index != $last_pref_index} {

    # Destroy the preference information pane
    destroy .prefwin.pf.f

    # Display the new preference information pane
    if {$index == 0} {
      create_general_pref
    } elseif {$index == 1} {
      create_color_pref
    } elseif {$index == 2} {
      create_cov_goal_pref
    } elseif {$index == 3} {
      create_syntax_pref
    }

    set last_pref_index $index

  }

}

proc create_general_pref {} {

  global tmp_show_wizard tmp_save_gui_on_exit tmp_show_tooltips

  # Create main frame
  labelframe .prefwin.pf.f -labelanchor nw -text "General Options" -pady 6 -padx 4

  # Create "Show Wizard" checkbutton
  checkbutton .prefwin.pf.f.wiz -text "Show wizard window on startup" -variable tmp_show_wizard -onvalue true -offvalue false -anchor w

  # Create checkbutton for saving GUI elements to configuration file upon exit
  checkbutton .prefwin.pf.f.save_gui_on_exit -text "Save state of GUI when exiting the application" -variable tmp_save_gui_on_exit -onvalue true -offvalue false -anchor w

  # Create checkbutton for displayint tooltips
  checkbutton .prefwin.pf.f.show_tooltips -text "Show tooltips" -variable tmp_show_tooltips -onvalue true -offvalue false -anchor w

  # Pack main frame
  grid .prefwin.pf.f.wiz              -row 0 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.save_gui_on_exit -row 1 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.show_tooltips    -row 2 -column 0 -sticky news -padx 4

  # Pack the frame
  pack .prefwin.pf.f -fill both

}

proc create_color_pref {} {

  global cov_fgColor   cov_bgColor   tmp_cov_fgColor   tmp_cov_bgColor
  global uncov_fgColor uncov_bgColor tmp_uncov_fgColor tmp_uncov_bgColor
  global race_fgColor  race_bgColor  tmp_race_fgColor  tmp_race_bgColor

  # Create main frame
  labelframe .prefwin.pf.f -labelanchor nw -text "Set Highlight Color (F=Change Foreground, B=Change Background)" -pady 6 -padx 4

  # Uncovered selectors
  button .prefwin.pf.f.ufb -text "F" -relief groove -command {
    set tmp_uncov_fgColor \
        [pref_set_label_color .prefwin.pf.f.ul fg $tmp_uncov_fgColor "Choose Uncovered Foreground"]
  }
  button .prefwin.pf.f.ubb -text "B" -relief groove -command {
    set tmp_uncov_bgColor \
        [pref_set_label_color .prefwin.pf.f.ul bg $tmp_uncov_bgColor "Choose Uncovered Background"]
  }
  label .prefwin.pf.f.ul -bg $tmp_uncov_bgColor -fg $tmp_uncov_fgColor -text "Uncovered Sample"

  # Covered selectors
  button .prefwin.pf.f.cfb -text "F" -relief groove -command {
    set tmp_cov_fgColor \
        [pref_set_label_color .prefwin.pf.f.cl fg $tmp_cov_fgColor "Choose Covered Foreground"]
  }
  button .prefwin.pf.f.cbb -text "B" -relief groove -command {
    set tmp_cov_bgColor \
        [pref_set_label_color .prefwin.pf.f.cl bg $tmp_cov_bgColor "Choose Covered Background"]
  }
  label .prefwin.pf.f.cl -bg $tmp_cov_bgColor -fg $tmp_cov_fgColor -text "Covered Sample"

  # Race selectors
  button .prefwin.pf.f.rfb -text "F" -relief groove -command {
    set tmp_race_fgColor \
        [pref_set_label_color .prefwin.pf.f.rl fg $tmp_race_fgColor "Choose Race Condition Foreground"]
  }
  button .prefwin.pf.f.rbb -text "B" -relief groove -command {
    set tmp_race_bgColor \
        [pref_set_label_color .prefwin.pf.f.rl bg $tmp_race_bgColor "Choose Race Condition Background"]
  }
  label .prefwin.pf.f.rl -bg $tmp_race_bgColor -fg $tmp_race_fgColor -text "Race Condition Sample"

  # Pack the color widgets into the color frame
  grid columnconfigure .prefwin.pf.f 3 -weight 1
  grid .prefwin.pf.f.ul  -row 0 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.ufb -row 0 -column 1 -sticky news -padx 4
  grid .prefwin.pf.f.ubb -row 0 -column 2 -sticky news -padx 4
  grid .prefwin.pf.f.cl  -row 1 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.cfb -row 1 -column 1 -sticky news -padx 4
  grid .prefwin.pf.f.cbb -row 1 -column 2 -sticky news -padx 4
  grid .prefwin.pf.f.rl  -row 2 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.rfb -row 2 -column 1 -sticky news -padx 4
  grid .prefwin.pf.f.rbb -row 2 -column 2 -sticky news -padx 4

  # Pack the frame
  pack .prefwin.pf.f -fill both

}

proc create_cov_goal_pref {} {

  global tmp_line_low_limit tmp_toggle_low_limit tmp_memory_low_limit tmp_comb_low_limit tmp_fsm_low_limit tmp_assert_low_limit

  # Create widgets
  labelframe .prefwin.pf.f -labelanchor nw -text "Set Acceptable Coverage Goals" -padx 4 -pady 6

  label .prefwin.pf.f.ll -anchor e -text "Line Coverage %:"
  percent_spinner .prefwin.pf.f.ls $tmp_line_low_limit

  label .prefwin.pf.f.tl -anchor e -text "Toggle Coverage %:"
  percent_spinner .prefwin.pf.f.ts $tmp_toggle_low_limit

  label .prefwin.pf.f.ml -anchor e -text "Memory Coverage %:"
  percent_spinner .prefwin.pf.f.ms $tmp_memory_low_limit

  label .prefwin.pf.f.cl -anchor e -text "Combinational Logic Coverage %:"
  percent_spinner .prefwin.pf.f.cs $tmp_comb_low_limit

  label .prefwin.pf.f.fl -anchor e -text "FSM State/Arc Coverage %:"
  percent_spinner .prefwin.pf.f.fs $tmp_fsm_low_limit

  label .prefwin.pf.f.al -anchor e -text "Assertion Coverage %:"
  percent_spinner .prefwin.pf.f.as $tmp_assert_low_limit

  # Pack widgets into grid
  grid columnconfigure .prefwin.pf.f 2 -weight 1
  grid .prefwin.pf.f.ll -row 0 -column 0 -sticky news
  grid .prefwin.pf.f.ls -row 0 -column 1 -sticky news
  grid .prefwin.pf.f.tl -row 1 -column 0 -sticky news
  grid .prefwin.pf.f.ts -row 1 -column 1 -sticky news
  grid .prefwin.pf.f.ml -row 2 -column 0 -sticky news
  grid .prefwin.pf.f.ms -row 2 -column 1 -sticky news
  grid .prefwin.pf.f.cl -row 3 -column 0 -sticky news
  grid .prefwin.pf.f.cs -row 3 -column 1 -sticky news
  grid .prefwin.pf.f.fl -row 4 -column 0 -sticky news
  grid .prefwin.pf.f.fs -row 4 -column 1 -sticky news
  grid .prefwin.pf.f.al -row 5 -column 0 -sticky news
  grid .prefwin.pf.f.as -row 5 -column 1 -sticky news

  # Pack the frame
  pack .prefwin.pf.f -fill both

}

proc create_syntax_pref {} {

  global vlog_hl_mode            tmp_vlog_hl_mode
  global vlog_hl_ppkeyword_color tmp_vlog_hl_ppkeyword_color
  global vlog_hl_keyword_color   tmp_vlog_hl_keyword_color
  global vlog_hl_comment_color   tmp_vlog_hl_comment_color
  global vlog_hl_value_color     tmp_vlog_hl_value_color
  global vlog_hl_string_color    tmp_vlog_hl_string_color
  global vlog_hl_symbol_color    tmp_vlog_hl_symbol_color
  global hl_mode

  # Create widgets
  labelframe .prefwin.pf.f -labelanchor nw -text "Set Syntax Highlighting Options" -padx 4 -pady 6

  checkbutton .prefwin.pf.f.mcb -variable tmp_vlog_hl_mode -onvalue on -offvalue off -anchor w \
                                  -text "Turn on syntax highlighting mode" -command {
    synchronize_syntax_widgets $tmp_vlog_hl_mode
  }

  label .prefwin.pf.f.ppcl -bg $vlog_hl_ppkeyword_color -anchor e -text "Preprocessor keyword highlight color:"
  button .prefwin.pf.f.ppcb -text "Change" -command {
    set tmp_vlog_hl_ppkeyword_color \
        [pref_set_label_color .prefwin.pf.f.ppcl bg $tmp_vlog_hl_ppkeyword_color "Choose Preprocessor Keyword Highlight Color"]
  }

  label .prefwin.pf.f.pcl -bg $vlog_hl_keyword_color -anchor e -text "Keyword highlight color:"
  button .prefwin.pf.f.pcb -text "Change" -command {
    set tmp_vlog_hl_keyword_color \
        [pref_set_label_color .prefwin.pf.f.pcl bg $tmp_vlog_hl_keyword_color "Choose Keyword Highlight Color"]
  }

  label .prefwin.pf.f.ccl -bg $vlog_hl_comment_color -anchor e -text "Comment highlight color:"
  button .prefwin.pf.f.ccb -text "Change" -command {
    set tmp_vlog_hl_comment_color \
        [pref_set_label_color .prefwin.pf.f.ccl bg $tmp_vlog_hl_comment_color "Choose Comment Highlight Color"]
  }

  label .prefwin.pf.f.vcl -bg $vlog_hl_value_color -anchor e -text "Value highlight color:"
  button .prefwin.pf.f.vcb -text "Change" -command {
    set tmp_vlog_hl_value_color \
        [pref_set_label_color .prefwin.pf.f.vcl bg $tmp_vlog_hl_value_color "Choose Value Highlight Color"]
  }

  label .prefwin.pf.f.stcl -bg $vlog_hl_string_color -anchor e -text "String highlight color:"
  button .prefwin.pf.f.stcb -text "Change" -command {
    set tmp_vlog_hl_string_color \
        [pref_set_label_color .prefwin.pf.f.stcl bg $tmp_vlog_hl_string_color "Choose String Highlight Color"]
  }

  label .prefwin.pf.f.sycl -bg $vlog_hl_symbol_color -anchor e -text "Symbol highlight color:"
  button .prefwin.pf.f.sycb -text "Change" -command {
    set tmp_vlog_hl_symbol_color \
        [pref_set_label_color .prefwin.pf.f.sycl bg $tmp_vlog_hl_symbol_color "Choose Symbol Highlight Color"]
  }

  grid columnconfigure .prefwin.pf.f 2 -weight 1
  grid .prefwin.pf.f.mcb  -row 0 -column 0 -sticky news -pady 4
  grid .prefwin.pf.f.ppcl -row 1 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.ppcb -row 1 -column 1 -sticky news -padx 4
  grid .prefwin.pf.f.pcl  -row 2 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.pcb  -row 2 -column 1 -sticky news -padx 4
  grid .prefwin.pf.f.ccl  -row 3 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.ccb  -row 3 -column 1 -sticky news -padx 4
  grid .prefwin.pf.f.vcl  -row 4 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.vcb  -row 4 -column 1 -sticky news -padx 4
  grid .prefwin.pf.f.stcl -row 5 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.stcb -row 5 -column 1 -sticky news -padx 4
  grid .prefwin.pf.f.sycl -row 6 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.sycb -row 6 -column 1 -sticky news -padx 4

  pack .prefwin.pf.f -fill both

  synchronize_syntax_widgets $tmp_vlog_hl_mode

}

# If the specified index is pointing to the coverage goals menu option, save off the limit
# values of each spinner box.
proc save_spinners {index} {

  global tmp_line_low_limit tmp_toggle_low_limit tmp_memory_low_limit tmp_comb_low_limit tmp_fsm_low_limit tmp_assert_low_limit

  if {$index == 2} {

    if {[info tclversion] >= 8.4} {
      set tmp_line_low_limit   [.prefwin.pf.f.ls get]
      set tmp_toggle_low_limit [.prefwin.pf.f.ts get]
      set tmp_memory_low_limit [.prefwin.pf.f.ms get]
      set tmp_comb_low_limit   [.prefwin.pf.f.cs get]
      set tmp_fsm_low_limit    [.prefwin.pf.f.fs get]
      set tmp_assert_low_limit [.prefwin.pf.f.as get]
    } else {
      set tmp_line_low_limit   [expr 100 - [.prefwin.pf.f.ls.l nearest 0]]
      set tmp_toggle_low_limit [expr 100 - [.prefwin.pf.f.ts.l nearest 0]]
      set tmp_memory_low_limit [expr 100 - [.prefwin.pf.f.ms.l nearest 0]]
      set tmp_comb_low_limit   [expr 100 - [.prefwin.pf.f.cs.l nearest 0]]
      set tmp_fsm_low_limit    [expr 100 - [.prefwin.pf.f.fs.l nearest 0]]
      set tmp_assert_low_limit [expr 100 - [.prefwin.pf.f.as.l nearest 0]]
    }

  }

}

# Function to verify the contents of an entry or spinbox entry (not currently used)
proc validInteger {win event X oldX min max} {

  # Make sure min<=max
  if {$min > $max} {
    set tmp $min; set min $max; set max $tmp
  }

  # Allow valid integers, empty strings, sign without number
  # Reject Octal numbers, but allow a single "0"
  # Which signes are allowed ?
  if {($min <= 0) && ($max >= 0)} {   ;# positive & negative sign
    set pattern {^[+-]?(()|0|([1-9][0-9]*))$}
  } elseif {$max < 0} {               ;# negative sign
    set pattern {^[-]?(()|0|([1-9][0-9]*))$}
  } else {                            ;# positive sign
    set pattern {^[+]?(()|0|([1-9][0-9]*))$}
  }

  # Weak integer checking: allow empty string, empty sign, reject octals
  set weakCheck [regexp $pattern $X]

  # if weak check fails, continue with old value
  if {! $weakCheck} {set X $oldX}

  # Strong integer checking with range
  set strongCheck [expr {[string is int -strict $X] && ($X >= $min) && ($X <= $max)}]

  switch $event {
    key {
      $win configure -bg [expr {$strongCheck ? "white" : "yellow"}]
      return $weakCheck
    }
    focusout {
      if {! $strongCheck} {$win configure -bg red}
        return $strongCheck
      }
    default {
      return 1
    }
  }

}

proc save_gui_elements {top this} {

  global save_gui_on_exit saved_gui

  if {[winfo toplevel $this] == $top && $save_gui_on_exit == true && $saved_gui == 0} {

    set saved_gui 1

    # Save the contents of any existing windows before writing the configuration information
    save_toggle_gui_elements 1
    save_memory_gui_elements 1
    save_comb_gui_elements 1
    save_fsm_gui_elements 1
    save_assert_gui_elements 1

    puts "Saving GUI information to configuration file..."

    # Write the information to the .coveredrc file
    write_coveredrc 1
  }

}
