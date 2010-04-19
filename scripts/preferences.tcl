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

##################################
# Manages the preferences window #
##################################

# Preference variables
array set preferences {
  main_geometry           ""
  show_wizard             true
  save_gui_on_exit        false
  show_tooltips           true
  ttk_style               "clam"
  enable_animations       true
  uncov_fgColor           blue
  uncov_bgColor           yellow
  cov_fgColor             black
  cov_bgColor             white
  race_fgColor            white
  race_bgColor            blue
  line_low_limit          90
  toggle_low_limit        90
  memory_low_limit        90
  comb_low_limit          90
  fsm_low_limit           90
  assert_low_limit        90
  vlog_hl_mode            on
  vlog_hl_ppkeyword_color ForestGreen
  vlog_hl_keyword_color   purple
  vlog_hl_comment_color   blue
  vlog_hl_value_color     red
  vlog_hl_string_color    red
  vlog_hl_symbol_color    coral
  exclude_reasons_enabled 1
  exclude_reasons         {}
  exclude_resolution      "first"
}

# Other global variables
set saved_gui             0
set rc_file_to_write      ""
set hl_mode               0
set last_pref_index       -1

# If we are on Mac OS X, set the default style to aqua
if {[tk windowingsystem] eq "aqua"} {
  set $preferences(ttk_style) "aqua"
}

# Create a list from 100 to 0
for {set i 100} {$i >= 0} {incr i -1} {
  lappend percent_range $i
}

proc read_coveredrc {} {

  global preferences
  global HOME USER_HOME rc_file_to_write

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
          set preferences(show_wizard) $value
        } elseif {$field == "SaveGuiOnExit"} {
          set preferences(save_gui_on_exit) $value
        } elseif {$field == "ShowTooltips"} {
          set preferences(show_tooltips) $value
        } elseif {$field == "TtkStyle"} {
          set preferences(ttk_style) $value
        } elseif {$field == "EnableAnimations"} {
          set preferences(enable_animations) $value
        } elseif {$field == "UncoveredForegroundColor"} {
          set preferences(uncov_fgColor) $value
        } elseif {$field == "UncoveredBackgroundColor"} {
          set preferences(uncov_bgColor) $value
        } elseif {$field == "CoveredForegroundColor"} {
          set preferences(cov_fgColor) $value
        } elseif {$field == "CoveredBackgroundColor"} {
          set preferences(cov_bgColor) $value
        } elseif {$field == "RaceConditionForegroundColor"} {
          set preferences(race_fgColor) $value
        } elseif {$field == "RaceConditionBackgroundColor"} {
          set preferences(race_bgColor) $value
        } elseif {$field == "AcceptableLinePercentage"} {
          set preferences(line_low_limit) $value
        } elseif {$field == "AcceptableTogglePercentage"} {
          set preferences(toggle_low_limit) $value
        } elseif {$field == "AcceptableMemoryPercentage"} {
          set preferences(memory_low_limit) $value
        } elseif {$field == "AcceptableCombinationalLogicPercentage"} {
          set preferences(comb_low_limit) $value
        } elseif {$field == "AcceptableFsmPercentage"} {
          set preferences(fsm_low_limit) $value
        } elseif {$field == "AcceptableAssertionPercentage"} {
          set preferences(assert_low_limit) $value
        } elseif {$field == "HighlightingMode"} {
          set preferences(vlog_hl_mode) $value
        } elseif {$field == "HighlightPreprocessorKeywordColor"} {
          set preferences(vlog_hl_ppkeyword_color) $value
        } elseif {$field == "HighlightKeywordColor"} {
          set preferences(vlog_hl_keyword_color) $value
        } elseif {$field == "HighlightCommentColor"} {
          set preferences(vlog_hl_comment_color) $value
        } elseif {$field == "HighlightValueColor"} {
          set preferences(vlog_hl_value_color) $value
        } elseif {$field == "HighlightStringColor"} {
          set preferences(vlog_hl_string_color) $value
        } elseif {$field == "HighlightSymbolColor"} {
          set preferences(vlog_hl_symbol_color) $value
        } elseif {$field == "EnableExclusionReasons"} {
          set preferences(exclude_reasons_enabled) $value
        } elseif {$field == "ExclusionReasons"} {
          set preferences(exclude_reasons) [split $value :]
        } elseif {$field == "ExclusionReasonConflictResolution"} {
          set preferences(exclude_resolution) $value

        # The following are GUI state-saved information -- only use this information if SaveGuiOnExit was set to true
        } elseif {$preferences(save_gui_on_exit) == "true"} {
          if {$field == "MainWindowGeometry"} {
            set preferences(main_geometry) $value
          } elseif {$field == "ToggleWindowGeometry"} {
            set preferences(toggle_geometry) $value
          } elseif {$field == "MemoryWindowGeometry"} {
            set preferences(memory_geometry) $value
          } elseif {$field == "CombWindowGeometry"} {
            set preferences(comb_geometry) $value
          } elseif {$field == "FsmWindowGeometry"} {
            set preferences(fsm_geometry) $value
          } elseif {$field == "AssertWindowGeometry"} {
            set preferences(assert_geometry) $value
          }
        }

      }
        
    }

    close $rc

  }

}

proc write_coveredrc {exiting} {

  global preferences
  global rc_file_to_write

  if {$rc_file_to_write != ""} {

    set rc [open $rc_file_to_write w]

    puts $rc "# Covered GUI Configuration File"
    puts $rc "#--------------------------------"
    puts $rc "# All variable assignments below are in the form of \"variable = value\""
    puts $rc "# where whitespace must be present between the variable, the \"=\" character"
    puts $rc "# and the value.  Comment lines start with the \"#\" character.\n"

    puts $rc "# If set to true, causes the wizard window to be displayed on GUI startup."
    puts $rc "# If set to false, the wizard window is suppressed.\n"

    puts $rc "ShowWizardOnStartup = $preferences(show_wizard)\n"

    puts $rc "# If set to true, causes various GUI elements to be saved when exiting the application"
    puts $rc "# which will be used when the application is restarted at a later time.  Set to false"
    puts $rc "# to cause the Covered GUI to use the default values.\n"

    puts $rc "SaveGuiOnExit = $preferences(save_gui_on_exit)\n"

    puts $rc "# If set to true, causes tooltips to be used for many widgets within the GUI.  If set"
    puts $rc "# to false, causes all tooltip support to be disabled."

    puts $rc "ShowTooltips = $preferences(show_tooltips)\n"

    puts $rc "# Specifies the ttk style type to use for the GUI.\n"

    puts $rc "TtkStyle = $preferences(ttk_style)\n"

    puts $rc "# Is set to true, enables a few of Covered's GUI animations.  If these animations"
    puts $rc "# don't work well or get in the way of your work, simply set this preference option"
    puts $rc "# to value of false.\n"

    puts $rc "EnableAnimations = $preferences(enable_animations)\n"

    puts $rc "# Sets the foreground color for all source code that is found"
    puts $rc "# to be uncovered during simulation.  The value can be any legal color"
    puts $rc "# value accepted by Tcl.\n"

    puts $rc "UncoveredForegroundColor = $preferences(uncov_fgColor)\n"

    puts $rc "# Sets the background color for all source code that is found"
    puts $rc "# to be uncovered during simulation.  The value can be any legal color"
    puts $rc "# value accepted by Tcl.\n"

    puts $rc "UncoveredBackgroundColor = $preferences(uncov_bgColor)\n"

    puts $rc "# Sets the foreground color for all source code that is found"
    puts $rc "# to be covered during simulation.  The value can be any legal color value"
    puts $rc "# accepted by Tcl.\n"

    puts $rc "CoveredForegroundColor = $preferences(cov_fgColor)\n"

    puts $rc "# Sets the background color for all source code that is found"
    puts $rc "# to be covered during simulation.  The value can be any legal color value."
    puts $rc "# accepted by Tcl.\n"

    puts $rc "CoveredBackgroundColor = $preferences(cov_bgColor)\n"

    puts $rc "# Sets the foreground color for all source code that has been detected as"
    puts $rc "# containing a race condition situation.  This code is not analyzed by Covered."
    puts $rc "# The value can be any legal color value accepted by Tcl.\n"

    puts $rc "RaceConditionForegroundColor = $preferences(race_fgColor)\n"

    puts $rc "# Sets the background color for all source code that has been detected as"
    puts $rc "# containing a race condition situation.  This code is not analyzed by Covered."
    puts $rc "# The value can be any legal color value accepted by Tcl.\n"

    puts $rc "RaceConditionBackgroundColor = $preferences(race_bgColor)\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved a line"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the line coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableLinePercentage = $preferences(line_low_limit)\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved a toggle"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the toggle coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableTogglePercentage = $preferences(toggle_low_limit)\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved a memory"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the memory coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableMemoryPercentage = $preferences(memory_low_limit)\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved a combinational"
    puts $rc "# logic coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the combinational logic coverage can possibly be"
    puts $rc "# deemed \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableCombinationalLogicPercentage = $preferences(comb_low_limit)\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved an FSM state/arc"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the FSM coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableFsmPercentage = $preferences(fsm_low_limit)\n"

    puts $rc "# Causes the summary color for a module/instance that has achieved an assertion"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the assertion coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableAssertionPercentage = $preferences(assert_low_limit)\n"

    puts $rc "# Causes syntax highlighting to be turned on or off.  This value should either be"
    puts $rc "# 'on' or 'off'.\n"

    puts $rc "HighlightingMode = $preferences(vlog_hl_mode)\n"

    puts $rc "# When the syntax highlighting mode is on, causes the preprocessor directive keywords"
    puts $rc "# to be highlighted with the specified color.  The value may be any color value that"
    puts $rc "# is acceptable to Tk.\n"

    puts $rc "HighlightPreprocessorKeywordColor = $preferences(vlog_hl_ppkeyword_color)\n"

    puts $rc "# When the syntax highlighting mode is on, causes the Verilog keywords to be highlighted"
    puts $rc "# with the specified color.  The value may be any color value that is acceptable to Tk.\n"

    puts $rc "HighlightKeywordColor = $preferences(vlog_hl_keyword_color)\n"

    puts $rc "# When the syntax highlighting mode is on, causes any single or multi-line comments to"
    puts $rc "# be highlighted with the specified color.  The value may be any color value that is"
    puts $rc "# acceptable to Tk.\n"

    puts $rc "HighlightCommentColor = $preferences(vlog_hl_comment_color)\n"

    puts $rc "# When the syntax highlighting mode is on, causes any defined, binary, octal, decimal or"
    puts $rc "# hexidecimal value to be highlighted with the specified color.  The value may be any"
    puts $rc "# color value that is acceptable to Tk.\n"

    puts $rc "HighlightValueColor = $preferences(vlog_hl_value_color)\n"

    puts $rc "# When the syntax highlighting mode is on, causes any quoted string to be highlighted with"
    puts $rc "# the specified color.  The value may be any color value that is acceptable to Tk.\n"

    puts $rc "HighlightStringColor = $preferences(vlog_hl_string_color)\n"

    puts $rc "# When the syntax highlighting mode is on, causes any Verilog symbol to be highlighted with"
    puts $rc "# the specified color.  The value may be any color value that is acceptable to Tk.\n"

    puts $rc "HighlightSymbolColor = $preferences(vlog_hl_symbol_color)\n"

    puts $rc "# If this value is set to true, a popup dialog box will be created whenever the user excludes a coverage"
    puts $rc "# point, allowing for the creation of a reason for the exclusion.  If this value is set to false, no popup"
    puts $rc "# box will be displayed when a coverage point is marked for exclusion.  The exclusion reason information"
    puts $rc "# is saved to the CDD (if specified from the GUI).\n"

    puts $rc "EnableExclusionReasons = $preferences(exclude_reasons_enabled)\n"

    puts $rc "# This string specifies default reasons for exclusion.  These reasons may be used wherever"
    puts $rc "# a coverage point is excluded (or the user may specify a unique reason).\n"

    puts $rc "ExclusionReasons = [join $preferences(exclude_reasons) :]\n"

    puts $rc "# This value specifies how to handle exclusion reason conflicts that arise when two or more CDD files"
    puts $rc "# are merged that contain mismatching exclusion reasons for identical coverage points.  The valid"
    puts $rc "# values are the following:"
    puts $rc "#   first - Use the exclusion reason from the first CDD file read in that contains the reason"
    puts $rc "#   last  - Use the exclusion reason from the last CDD file read in that contains the reason"
    puts $rc "#   all   - Merge all exclusion reasons into one" 
    puts $rc "#   new   - Use the newest exclusion reason specified"
    puts $rc "#   old   - Use the oldest exclusion reason specified\n"

    puts $rc "ExclusionReasonConflictResolution = $preferences(exclude_resolution)\n"

    puts $rc "# THE FOLLOWING LINES ARE FOR STORING GUI STATE INFORMATION -- DO NOT MODIFY LINES BELOW THIS COMMENT!\n"

    if {$preferences(save_gui_on_exit) == true && $exiting == 1} {

      puts $rc "MainWindowGeometry                = [winfo geometry .]"
      puts $rc "ToggleWindowGeometry              = $preferences(toggle_geometry)"
      puts $rc "MemoryWindowGeometry              = $preferences(memory_geometry)"
      puts $rc "CombWindowGeometry                = $preferences(comb_geometry)"
      puts $rc "FsmWindowGeometry                 = $preferences(fsm_geometry)"
      puts $rc "AssertWindowGeometry              = $preferences(assert_geometry)"

    }

    flush $rc
    close $rc

  }

}

proc create_preferences {start_index} {

  global preferences tmp_preferences
  global hl_mode last_pref_index

  # Now create the window and set the grab to this window
  if {[winfo exists .prefwin] == 0} {

    # Initialize all temporary preference values
    array set tmp_preferences [array get preferences]

    # Specify that there was no last index selected
    set last_pref_index -1

    # Create new window
    toplevel .prefwin
    wm title .prefwin "Covered - Preferences"
    # wm resizable .prefwin 0 0
    wm geometry .prefwin =600x500

    # Create listbox frame
    ttk::frame .prefwin.lbf -relief raised -borderwidth 1
    ttk::label .prefwin.lbf.l -text "Option Categories"
    listbox .prefwin.lbf.lb
    bind .prefwin.lbf.lb <<ListboxSelect>> populate_pref

    # Populate listbox with various options
    .prefwin.lbf.lb insert end "General"
    .prefwin.lbf.lb insert end "Colors"
    .prefwin.lbf.lb insert end "Coverage Goals"
    .prefwin.lbf.lb insert end "Syntax Highlighting"
    .prefwin.lbf.lb insert end "Exclusions"
    .prefwin.lbf.lb insert end "Merging"

    pack .prefwin.lbf.l  -pady 4
    pack .prefwin.lbf.lb -fill y -expand 1

    # Create preference frame
    ttk::frame .prefwin.pf -relief raised -borderwidth 1
    ttk::frame .prefwin.pf.f
    ttk::label .prefwin.pf.f.l -text "- Select an option category on the left to view/edit.\n\n- Click the \"Apply button\" to apply all preference options\nto the rest of the GUI.\n\n- Click the \"OK\" button to apply and save the changes to\nthe Covered configuration file and exit this window.\n\n- Click the \"Cancel\" button to forget all preference changes\nmade the exit this window.\n\n- Click the \"Help\" button to get help for the shown option window." -justify left
    pack .prefwin.pf.f.l -fill y -padx 8 -pady 10 
    pack .prefwin.pf.f -fill both

    # Create button frame
    ttk::frame .prefwin.bf -relief raised -borderwidth 1
    ttk::button .prefwin.bf.apply -width 10 -text "Apply" -command {
      apply_preferences
    }
    ttk::button .prefwin.bf.ok -width 10 -text "OK" -command {
      if {[apply_preferences] == 1} {
        write_coveredrc 0
      }
      destroy .prefwin
    }
    ttk::button .prefwin.bf.cancel -width 10 -text "Cancel" -command {
      destroy .prefwin
    }
    help_button .prefwin.bf.help ""
    bind .prefwin.bf.help <Button-1> {
      switch [.prefwin.lbf.lb curselection] {
        0 { help_show_manual chapter.gui.preferences "section.gui.pref.general" }
        1 { help_show_manual chapter.gui.preferences "section.gui.pref.color" }
        2 { help_show_manual chapter.gui.preferences "section.gui.pref.goals" }
        3 { help_show_manual chapter.gui.preferences "section.gui.pref.syntax" }
        4 { help_show_manual chapter.gui.preferences "section.gui.pref.exclusions" }
        5 { help_show_manual chapter.gui.preferences "section.gui.pref.merging" }
        default { help_show_manual chapter.gui.preferences "" }
      }
    }
    pack .prefwin.bf.help   -side right -padx 4 -pady 4
    pack .prefwin.bf.cancel -side right -padx 4 -pady 4
    pack .prefwin.bf.ok     -side right -padx 4 -pady 4
    pack .prefwin.bf.apply  -side right -padx 4 -pady 4

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

  global preferences tmp_preferences
  global cov_rb metric_src

  # Save spinner values to temporary storage items
  save_spinners [.prefwin.lbf.lb curselection]

  set changed 0

  # Check for changes and update global preference variables accordingly
  foreach pref [array names preferences] {   
    if {$preferences($pref) != $tmp_preferences($pref)} {
      set preferences($pref) $tmp_preferences($pref)
      set changed 1
      if {$pref eq "ttk_style"} {
        ttk::style theme use $preferences(ttk_style)
      }
    }
  }

  # Update the display if necessary
  if {$changed == 1} {

    # Redisplay the information in the main file viewer
    if {$cov_rb == "Line"} {
      set text_x [$metric_src(line).txt xview]
      set text_y [$metric_src(line).txt yview]
      display_line_cov
      $metric_src(line).txt xview moveto [lindex $text_x 0]
      $metric_src(line).txt yview moveto [lindex $text_y 0]
    } elseif {$cov_rb == "Toggle"} {
      set text_x [$metric_src(toggle).txt xview]
      set text_y [$metric_src(toggle).txt yview]
      display_toggle_cov
      $metric_src(toggle).txt xview moveto [lindex $text_x 0]
      $metric_src(toggle).txt yview moveto [lindex $text_y 0]
    } elseif {$cov_rb == "Memory"} {
      set text_x [$metric_src(memory).txt xview]
      set text_y [$metric_src(memory).txt yview]
      display_memory_cov
      $metric_src(memory).txt xview moveto [lindex $text_x 0]
      $metric_src(memory).txt yview moveto [lindex $text_y 0]
    } elseif {$cov_rb == "Logic"} {
      set text_x [$metric_src(comb).txt xview]
      set text_y [$metric_src(comb).txt yview]
      display_comb_cov
      $metric_src(comb).txt xview moveto [lindex $text_x 0]
      $metric_src(comb).txt yview moveto [lindex $text_y 0]
    } elseif {$cov_rb == "FSM"} {
      set text_x [$metric_src(fsm).txt xview]
      set text_y [$metric_src(fsm).txt yview]
      display_fsm_cov
      $metric_src(fsm).txt xview moveto [lindex $text_x 0]
      $metric_src(fsm).txt yview moveto [lindex $text_y 0]
    } elseif {$cov_rb == "Assert"} {
      set text_x [$metric_src(assert).txt xview]
      set text_y [$metric_src(assert).txt yview]
      display_assert_cov
      $metric_src(assert).txt xview moveto [lindex $text_x 0]
      $metric_src(assert).txt yview moveto [lindex $text_y 0]
    }

    # Redisplay the combinational logic window, if it exists
    if {[winfo exists .combwin] != 0} {
      display_comb_info
    }

    # Redisplay the FSM logic window, if it exists
    if {[winfo exists .fsmwin] != 0} {
      display_fsm_table
    }

    # Update the treeview
    populate_treeview

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
    } elseif {$index == 4} {
      create_exclusion_pref
    } elseif {$index == 5} {
      create_merging_pref
    }

    set last_pref_index $index

  }

}

proc update_ttk_style {args} {

  global tmp_preferences ttk_style

  set tmp_preferences(ttk_style) $ttk_style

}

proc create_general_pref {} {

  global tmp_preferences ttk_style

  # Create main frame
  ttk::labelframe .prefwin.pf.f -labelanchor nw -text "General Options"

  # Create "Show Wizard" checkbutton
  ttk::checkbutton .prefwin.pf.f.wiz -text "Show wizard window on startup" -variable tmp_preferences(show_wizard) -onvalue true -offvalue false

  # Create checkbutton for saving GUI elements to configuration file upon exit
  ttk::checkbutton .prefwin.pf.f.save_gui_on_exit -text "Save state of GUI when exiting the application" \
                                                  -variable tmp_preferences(save_gui_on_exit) -onvalue true -offvalue false

  # Create checkbutton for displaying tooltips
  ttk::checkbutton .prefwin.pf.f.show_tooltips -text "Show tooltips" -variable tmp_preferences(show_tooltips) -onvalue true -offvalue false

  # Create checkbutton for enabling animations
  ttk::checkbutton .prefwin.pf.f.enable_anims -text "Enable GUI animations" -variable tmp_preferences(enable_animations) -onvalue true -offvalue false

  # Create ttk style frame
  ttk::frame     .prefwin.pf.f.tf
  ttk::label     .prefwin.pf.f.tf.l -text "Select a GUI theme:"
  eval ttk_optionMenu .prefwin.pf.f.tf.om ttk_style [ttk::style theme names]
  trace add variable ttk_style write update_ttk_style

  pack .prefwin.pf.f.tf.l  -side left -padx 4
  pack .prefwin.pf.f.tf.om -side left -padx 4

  # Pack main frame
  grid .prefwin.pf.f.wiz              -row 0 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.save_gui_on_exit -row 1 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.show_tooltips    -row 2 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.enable_anims     -row 3 -column 0 -sticky news -padx 4
  grid .prefwin.pf.f.tf               -row 4 -column 0 -sticky news -padx 4

  # Pack the frame
  pack .prefwin.pf.f -fill both

}

proc create_color_pref {} {

  global tmp_preferences

  # Create main frame
  ttk::labelframe .prefwin.pf.f -labelanchor nw -text "Set Highlight Color (F=Change Foreground, B=Change Background)"

  # Uncovered selectors
  ttk::button .prefwin.pf.f.ufb -text "F" -command {
    set tmp_preferences(uncov_fgColor) \
        [pref_set_label_color .prefwin.pf.f.ul fg $tmp_preferences(uncov_fgColor) "Choose Uncovered Foreground"]
  }
  ttk::button .prefwin.pf.f.ubb -text "B" -command {
    set tmp_preferences(uncov_bgColor) \
        [pref_set_label_color .prefwin.pf.f.ul bg $tmp_preferences(uncov_bgColor) "Choose Uncovered Background"]
  }
  ttk::label .prefwin.pf.f.ul -background $tmp_preferences(uncov_bgColor) -foreground $tmp_preferences(uncov_fgColor) -text "Uncovered Sample"

  # Covered selectors
  ttk::button .prefwin.pf.f.cfb -text "F" -command {
    set tmp_preferences(cov_fgColor) \
        [pref_set_label_color .prefwin.pf.f.cl fg $tmp_preferences(cov_fgColor) "Choose Covered Foreground"]
  }
  ttk::button .prefwin.pf.f.cbb -text "B" -command {
    set tmp_preferences(cov_bgColor) \
        [pref_set_label_color .prefwin.pf.f.cl bg $tmp_preferences(cov_bgColor) "Choose Covered Background"]
  }
  ttk::label .prefwin.pf.f.cl -background $tmp_preferences(cov_bgColor) -foreground $tmp_preferences(cov_fgColor) -text "Covered Sample"

  # Race selectors
  ttk::button .prefwin.pf.f.rfb -text "F" -command {
    set tmp_preferences(race_fgColor) \
        [pref_set_label_color .prefwin.pf.f.rl fg $tmp_preferences(race_fgColor) "Choose Race Condition Foreground"]
  }
  ttk::button .prefwin.pf.f.rbb -text "B" -command {
    set tmp_preferences(race_bgColor) \
        [pref_set_label_color .prefwin.pf.f.rl bg $tmp_preferences(race_bgColor) "Choose Race Condition Background"]
  }
  ttk::label .prefwin.pf.f.rl -background $tmp_preferences(race_bgColor) -foreground $tmp_preferences(race_fgColor) -text "Race Condition Sample"

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

  global tmp_preferences

  # Create widgets
  ttk::labelframe .prefwin.pf.f -labelanchor nw -text "Set Acceptable Coverage Goals"

  ttk::label .prefwin.pf.f.ll -anchor e -text "Line Coverage %:"
  percent_spinner .prefwin.pf.f.ls $tmp_preferences(line_low_limit)

  ttk::label .prefwin.pf.f.tl -anchor e -text "Toggle Coverage %:"
  percent_spinner .prefwin.pf.f.ts $tmp_preferences(toggle_low_limit)

  ttk::label .prefwin.pf.f.ml -anchor e -text "Memory Coverage %:"
  percent_spinner .prefwin.pf.f.ms $tmp_preferences(memory_low_limit)

  ttk::label .prefwin.pf.f.cl -anchor e -text "Combinational Logic Coverage %:"
  percent_spinner .prefwin.pf.f.cs $tmp_preferences(comb_low_limit)

  ttk::label .prefwin.pf.f.fl -anchor e -text "FSM State/Arc Coverage %:"
  percent_spinner .prefwin.pf.f.fs $tmp_preferences(fsm_low_limit)

  ttk::label .prefwin.pf.f.al -anchor e -text "Assertion Coverage %:"
  percent_spinner .prefwin.pf.f.as $tmp_preferences(assert_low_limit)

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

  global tmp_preferences
  global hl_mode

  # Create widgets
  ttk::labelframe .prefwin.pf.f -labelanchor nw -text "Set Syntax Highlighting Options"

  ttk::checkbutton .prefwin.pf.f.mcb -variable tmp_preferences(vlog_hl_mode) -onvalue on -offvalue off \
                                  -text "Turn on syntax highlighting mode" -command {
    synchronize_syntax_widgets $tmp_preferences(vlog_hl_mode)
  }

  ttk::label .prefwin.pf.f.ppcl -background $preferences(vlog_hl_ppkeyword_color) -anchor e -text "Preprocessor keyword highlight color:"
  ttk::button .prefwin.pf.f.ppcb -text "Change" -command {
    set tmp_vlog_hl_ppkeyword_color \
        [pref_set_label_color .prefwin.pf.f.ppcl bg $tmp_preferences(vlog_hl_ppkeyword_color) "Choose Preprocessor Keyword Highlight Color"]
  }

  ttk::label .prefwin.pf.f.pcl -background $preferences(vlog_hl_keyword_color) -anchor e -text "Keyword highlight color:"
  ttk::button .prefwin.pf.f.pcb -text "Change" -command {
    set tmp_vlog_hl_keyword_color \
        [pref_set_label_color .prefwin.pf.f.pcl bg $tmp_preferences(vlog_hl_keyword_color) "Choose Keyword Highlight Color"]
  }

  ttk::label .prefwin.pf.f.ccl -background $preferences(vlog_hl_comment_color) -anchor e -text "Comment highlight color:"
  ttk::button .prefwin.pf.f.ccb -text "Change" -command {
    set tmp_vlog_hl_comment_color \
        [pref_set_label_color .prefwin.pf.f.ccl bg $tmp_preferences(vlog_hl_comment_color) "Choose Comment Highlight Color"]
  }

  ttk::label .prefwin.pf.f.vcl -background $preferences(vlog_hl_value_color) -anchor e -text "Value highlight color:"
  ttk::button .prefwin.pf.f.vcb -text "Change" -command {
    set tmp_vlog_hl_value_color \
        [pref_set_label_color .prefwin.pf.f.vcl bg $tmp_preferences(vlog_hl_value_color) "Choose Value Highlight Color"]
  }

  ttk::label .prefwin.pf.f.stcl -background $preferences(vlog_hl_string_color) -anchor e -text "String highlight color:"
  ttk::button .prefwin.pf.f.stcb -text "Change" -command {
    set tmp_vlog_hl_string_color \
        [pref_set_label_color .prefwin.pf.f.stcl bg $tmp_preferences(vlog_hl_string_color) "Choose String Highlight Color"]
  }

  ttk::label .prefwin.pf.f.sycl -background $preferences(vlog_hl_symbol_color) -anchor e -text "Symbol highlight color:"
  ttk::button .prefwin.pf.f.sycb -text "Change" -command {
    set tmp_vlog_hl_symbol_color \
        [pref_set_label_color .prefwin.pf.f.sycl bg $tmp_preferences(vlog_hl_symbol_color) "Choose Symbol Highlight Color"]
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

  synchronize_syntax_widgets $tmp_preferences(vlog_hl_mode)

}

proc store_exclude_reasons {} {

  global tmp_preferences

  set tmp_preferences(exclude_reasons) [.prefwin.pf.f.elf.lf.tl get 0 end]

}

proc create_exclusion_pref {} {

  global HOME tablelistopts
  global tmp_preferences

  # Create widgets
  ttk::frame .prefwin.pf.f

  # Create checkbutton for turning exclusion reason support on/off
  ttk::labelframe  .prefwin.pf.f.ecf    -text "Exclusion Options"
  ttk::checkbutton .prefwin.pf.f.ecf.cb -text "Enable exclusion reason support when items are excluded" \
                                        -variable tmp_preferences(exclude_reasons_enabled)
  pack .prefwin.pf.f.ecf.cb -fill x -padx 3 -pady 3

  # Create labelframe for creating general reasons for exclusion
  ttk::labelframe .prefwin.pf.f.elf -labelanchor nw -text "Create General Exclusion Reasons"

  # Create tablelist frame
  ttk::frame .prefwin.pf.f.elf.lf
  tablelist::tablelist .prefwin.pf.f.elf.lf.tl -columns "0 {Exclusion Reason}" -stretch all \
    -xscrollcommand {.prefwin.pf.f.elf.lf.hb set} -yscrollcommand {.prefwin.pf.f.elf.lf.vb set} -movablerows 1 -selectmode single
  foreach {key value} [array get tablelistopts] {
    .prefwin.pf.f.elf.lf.tl configure -$key $value
  }
  bind .prefwin.pf.f.elf.lf.tl <<ListboxSelect>> {
    set row [.prefwin.pf.f.elf.lf.tl curselection]
    if {$row != ""} {
      .prefwin.pf.f.elf.tf.t   delete 1.0 end
      .prefwin.pf.f.elf.tf.t   insert end [%W getcells [list $row,0]]
      .prefwin.pf.f.elf.bf.add configure -state normal
      .prefwin.pf.f.elf.bf.del configure -state normal
    }
  }
  ttk::scrollbar .prefwin.pf.f.elf.lf.hb -command {.prefwin.pf.f.elf.lf.tl xview} -orient horizontal
  ttk::scrollbar .prefwin.pf.f.elf.lf.vb -command {.prefwin.pf.f.elf.lf.tl yview}
  grid rowconfigure    .prefwin.pf.f.elf.lf 0 -weight 1
  grid columnconfigure .prefwin.pf.f.elf.lf 0 -weight 1
  grid .prefwin.pf.f.elf.lf.tl -row 0 -column 0 -sticky news
  grid .prefwin.pf.f.elf.lf.vb -row 0 -column 1 -sticky ns
  grid .prefwin.pf.f.elf.lf.hb -row 1 -column 0 -sticky ew

  # Create button frame
  ttk::frame  .prefwin.pf.f.elf.bf
  set plus [image create bitmap -data "#define plus_width 18\n#define plus_height 18\nstatic unsigned char plus_bits[] = {\n0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0xf0, 0x3f, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};"]
  ttk::button .prefwin.pf.f.elf.bf.add -image $plus -state disabled -command {
    .prefwin.pf.f.elf.lf.tl  insert end [string trim [string map {\n { } \r { } \t { }} [list [.prefwin.pf.f.elf.tf.t get 1.0 end]]]]
    .prefwin.pf.f.elf.tf.t   delete 1.0 end
    .prefwin.pf.f.elf.bf.add configure -state disabled
    store_exclude_reasons
  }
  set_balloon .prefwin.pf.f.elf.bf.add "Adds the reason to the list"
  set minus [image create bitmap -data "#define minus_width 18\n#define minus_height 18\nstatic unsigned char minus_bits[] = {\n0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};"]
  ttk::button .prefwin.pf.f.elf.bf.del -image $minus -state disabled -command {
    .prefwin.pf.f.elf.lf.tl  delete [.prefwin.pf.f.elf.lf.tl curselection]
    .prefwin.pf.f.elf.tf.t   delete 1.0 end
    .prefwin.pf.f.elf.bf.add configure -state disabled
    .prefwin.pf.f.elf.bf.del configure -state disabled
    store_exclude_reasons
  }
  set_balloon .prefwin.pf.f.elf.bf.del "Removes the selected reason from the list"
  pack .prefwin.pf.f.elf.bf.add -side left -padx 3 -pady 3
  pack .prefwin.pf.f.elf.bf.del -side left -padx 3 -pady 3

  # Create text frame
  ttk::frame .prefwin.pf.f.elf.tf
  text       .prefwin.pf.f.elf.tf.t -wrap word -yscrollcommand {.prefwin.pf.f.elf.tf.vb set} -height 6
  bind .prefwin.pf.f.elf.tf.t <KeyRelease> {
    if {[%W count -chars 1.0 end] < 2} {
      .prefwin.pf.f.elf.bf.add configure -state disabled
    } else {
      .prefwin.pf.f.elf.bf.add configure -state normal
    }
  }
  ttk::scrollbar .prefwin.pf.f.elf.tf.vb -command {.prefwin.pf.f.elf.tf.t yview}
  grid columnconfigure .prefwin.pf.f.elf.tf 0 -weight 1
  grid .prefwin.pf.f.elf.tf.t  -row 0 -column 0 -sticky news
  grid .prefwin.pf.f.elf.tf.vb -row 0 -column 1 -sticky ns

  # Pack the tablelist, button and text frames
  pack .prefwin.pf.f.elf.lf -fill both -expand yes
  pack .prefwin.pf.f.elf.bf -fill both
  pack .prefwin.pf.f.elf.tf -fill both

  # Pack the labelframe
  pack .prefwin.pf.f.ecf -fill x
  pack .prefwin.pf.f.elf -fill both -expand yes

  # Pack the entire exclusion reason window
  pack .prefwin.pf.f -fill both

  # Insert the values into the tablelist
  foreach exclude_reason $tmp_preferences(exclude_reasons) {
    .prefwin.pf.f.elf.lf.tl insert end $exclude_reason
  }

}

proc create_merging_pref {} {

  global tmp_preferences

  # Create widgets
  ttk::frame .prefwin.pf.f

  # Create the exclusion reason conflict resolution frame
  ttk::labelframe .prefwin.pf.f.ecr -text "Exclusion Reason Conflict Resolution"
  ttk::radiobutton .prefwin.pf.f.ecr.first -text "Use the first reason"  -variable tmp_preferences(exclude_resolution) -value first
  ttk::radiobutton .prefwin.pf.f.ecr.last  -text "Use the last reason"   -variable tmp_preferences(exclude_resolution) -value last
  ttk::radiobutton .prefwin.pf.f.ecr.new   -text "Use the newest reason" -variable tmp_preferences(exclude_resolution) -value new
  ttk::radiobutton .prefwin.pf.f.ecr.old   -text "Use the oldest reason" -variable tmp_preferences(exclude_resolution) -value old
  ttk::radiobutton .prefwin.pf.f.ecr.all   -text "Merge all reasons"     -variable tmp_preferences(exclude_resolution) -value all

  pack .prefwin.pf.f.ecr.first -anchor w -padx 3 -pady 3
  pack .prefwin.pf.f.ecr.last  -anchor w -padx 3 -pady 3
  pack .prefwin.pf.f.ecr.new   -anchor w -padx 3 -pady 3
  pack .prefwin.pf.f.ecr.old   -anchor w -padx 3 -pady 3
  pack .prefwin.pf.f.ecr.all   -anchor w -padx 3 -pady 3

  # Pack the labelframes
  pack .prefwin.pf.f.ecr -fill both -expand yes

  # Pack the entire preference frame
  pack .prefwin.pf.f -fill both

}

# If the specified index is pointing to the coverage goals menu option, save off the limit
# values of each spinner box.
proc save_spinners {index} {

  global tmp_preferences

  if {$index == 2} {

    if {[info tclversion] >= 8.4} {
      set tmp_preferences(line_low_limit)   [.prefwin.pf.f.ls get]
      set tmp_preferences(toggle_low_limit) [.prefwin.pf.f.ts get]
      set tmp_preferences(memory_low_limit) [.prefwin.pf.f.ms get]
      set tmp_preferences(comb_low_limit)   [.prefwin.pf.f.cs get]
      set tmp_preferences(fsm_low_limit)    [.prefwin.pf.f.fs get]
      set tmp_preferences(assert_low_limit) [.prefwin.pf.f.as get]
    } else {
      set tmp_preferences(line_low_limit)   [expr 100 - [.prefwin.pf.f.ls.l nearest 0]]
      set tmp_preferences(toggle_low_limit) [expr 100 - [.prefwin.pf.f.ts.l nearest 0]]
      set tmp_preferences(memory_low_limit) [expr 100 - [.prefwin.pf.f.ms.l nearest 0]]
      set tmp_preferences(comb_low_limit)   [expr 100 - [.prefwin.pf.f.cs.l nearest 0]]
      set tmp_preferences(fsm_low_limit)    [expr 100 - [.prefwin.pf.f.fs.l nearest 0]]
      set tmp_preferences(assert_low_limit) [expr 100 - [.prefwin.pf.f.as.l nearest 0]]
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

  global preferences saved_gui

  if {[winfo toplevel $this] == $top && $preferences(save_gui_on_exit) == true && $saved_gui == 0} {

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
