#!/usr/bin/env wish

##################################
# Manages the preferences window #
##################################

# Global variables (eventually we will want to get/save these to a configuration file!)
set uncov_fgColor    blue
set uncov_bgColor    yellow
set cov_fgColor      black
set cov_bgColor      white
set race_fgColor     white
set race_bgColor     blue
set line_low_limit   90
set toggle_low_limit 90
set comb_low_limit   90
set fsm_low_limit    90
set rc_file_to_write ""

# Create a list from 100 to 0
for {set i 100} {$i >= 0} {incr i -1} {
  lappend percent_range $i
}

proc read_coveredrc {} {

  global uncov_fgColor uncov_bgColor
  global cov_fgColor   cov_bgColor
  global race_fgColor  race_bgColor
  global line_low_limit toggle_low_limit comb_low_limit fsm_low_limit
  global HOME USER_HOME rc_file_to_write

  puts "$HOME/scripts/.coveredrc"

  # Find the correct configuration file to read and eventually write
  if {[file exists ".coveredrc"] == 1} {
    set rc [open ".coveredrc" r]
    set rc_file_to_write ".coveredrc"
    puts "Reading from configuration file .coveredrc"
  } elseif {[file exists "$USER_HOME/.coveredrc"] == 1} {
    set rc [open "$USER_HOME/.coveredrc" r]
    set rc_file_to_write "$USER_HOME/.coveredrc"
    puts "Reading from configuration file $USER_HOME/.coveredrc"
  } elseif {[file exists "$HOME/scripts/.coveredrc"] == 1} {
    set rc [open "$HOME/scripts/.coveredrc" r]
    set rc_file_to_write "$USER_HOME/.coveredrc"
    puts "Reading from configuration file $HOME/scripts/.coveredrc"
  } else {
    set rc -1
    set rc_file_to_write ""
    puts "Unable to find configuration file to read"
  }

  if {$rc != -1} {

    seek $rc 0 start

    while {[eof $rc] == 0} {

      # Parse string here
      set line_elems [split [gets $rc]]

      if {[lindex $line_elems 1] == "="} {

        set field [lindex $line_elems 0]
        set value [lindex $line_elems 2]

        if {$field == "UncoveredForegroundColor"} {
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
        } elseif {$field == "AcceptableCombinationalLogicPercentage"} {
          set comb_low_limit $value
        } elseif {$field == "AcceptableFsmPercentage"} {
          set fsm_low_limit $value
        }

      }
        
    }

    close $rc

  }

}

proc write_coveredrc {} {

  global uncov_fgColor uncov_bgColor
  global cov_fgColor   cov_bgColor
  global race_fgColor  race_bgColor
  global line_low_limit toggle_low_limit comb_low_limit fsm_low_limit
  global rc_file_to_write

  if {$rc_file_to_write != ""} {

    puts "Writing to configuration file $rc_file_to_write"

    set rc [open $rc_file_to_write w]

    puts $rc "# Covered GUI Configuration File"
    puts $rc "#--------------------------------"
    puts $rc "# All variable assignments below are in the form of \"variable = value\""
    puts $rc "# where whitespace must be present between the variable, the \"=\" character"
    puts $rc "# and the value.  Comment lines start with the \"#\" character."

    puts $rc "# Sets the foreground color for all source code that is found"
    puts $rc "# to be uncovered during simulation.  The value can be any legal color"
    puts $rc "# value accepted by Tcl."

    puts $rc "UncoveredForegroundColor = $uncov_fgColor"

    puts $rc "# Sets the background color for all source code that is found"
    puts $rc "# to be uncovered during simulation.  The value can be any legal color"
    puts $rc "# value accepted by Tcl.\n"

    puts $rc "UncoveredBackgroundColor = $uncov_bgColor"

    puts $rc "# Sets the foreground color for all source code that is found"
    puts $rc "# to be covered during simulation.  The value can be any legal color value"
    puts $rc "# accepted by Tcl.\n"

    puts $rc "CoveredForegroundColor = $cov_fgColor"

    puts $rc "# Sets the background color for all source code that is found"
    puts $rc "# to be covered during simulation.  The value can be any legal color value."
    puts $rc "# accepted by Tcl."

    puts $rc "CoveredBackgroundColor = $cov_bgColor"

    puts $rc "# Sets the foreground color for all source code that has been detected as"
    puts $rc "# containing a race condition situation.  This code is not analyzed by Covered."
    puts $rc "# The value can be any legal color value accepted by Tcl.\n"

    puts $rc "RaceConditionForegroundColor = $race_fgColor"

    puts $rc "# Sets the background color for all source code that has been detected as"
    puts $rc "# containing a race condition situation.  This code is not analyzed by Covered."
    puts $rc "# The value can be any legal color value accepted by Tcl.\n"

    puts $rc "RaceConditionBackgroundColor = $race_bgColor"

    puts $rc "# Causes the summary color for a module/instance that has achieved a line"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the line coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableLinePercentage = $line_low_limit"

    puts $rc "# Causes the summary color for a module/instance that has achieved a toggle"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the toggle coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableTogglePercentage = $toggle_low_limit"

    puts $rc "# Causes the summary color for a module/instance that has achieved a combinational"
    puts $rc "# logic coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the combinational logic coverage can possibly be"
    puts $rc "# deemed \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableCombinationalLogicPercentage = $comb_low_limit"

    puts $rc "# Causes the summary color for a module/instance that has achieved an FSM state/arc"
    puts $rc "# coverage percentage greater than or equal to this value (but not 100%) to be"
    puts $rc "# colored \"yellow\", indicating that the FSM coverage can possibly be deemed"
    puts $rc "# \"good enough\".  This value must be in the range of 0 - 100.\n"

    puts $rc "AcceptableFsmPercentage = $fsm_low_limit"

    flush $rc
    close $rc

  }

}

proc create_preferences {} {

  global cov_fgColor   cov_bgColor   tmp_cov_fgColor   tmp_cov_bgColor
  global uncov_fgColor uncov_bgColor tmp_uncov_fgColor tmp_uncov_bgColor
  global race_fgColor  race_bgColor  tmp_race_fgColor  tmp_race_bgColor
  global line_low_limit   tmp_line_low_limit
  global toggle_low_limit tmp_toggle_low_limit
  global comb_low_limit   tmp_comb_low_limit
  global fsm_low_limit    tmp_fsm_low_limit

  # Now create the window and set the grab to this window
  if {[winfo exists .prefwin] == 0} {

    # Setup common color widget values
    set brelief "groove"
    set bpadx   8
    set bpady   4

    # Initialize all temporary preference values
    set tmp_cov_fgColor      $cov_fgColor
    set tmp_cov_bgColor      $cov_bgColor
    set tmp_uncov_fgColor    $uncov_fgColor
    set tmp_uncov_bgColor    $uncov_bgColor
    set tmp_race_fgColor     $race_fgColor
    set tmp_race_bgColor     $race_bgColor

    # Create new window
    toplevel .prefwin
    wm title .prefwin "Covered - Preferences"
    wm resizable .prefwin 0 0

    ###############################
    # Create color selection area #
    ###############################

      frame .prefwin.c -relief raised -borderwidth 1
      label .prefwin.c.l -anchor w -text "Select Highlight Color"

      # Uncovered selectors
      button .prefwin.c.ufb -bg $uncov_bgColor -fg $uncov_fgColor -activebackground $uncov_bgColor -activeforeground $uncov_fgColor \
             -text "Change Uncovered Foreground" -relief $brelief -command {
        set tmp_uncov_fgColor [pref_set_button_color .prefwin.c.ufb .prefwin.c.ubb fg $tmp_uncov_fgColor "Choose Uncovered Foreground"]
      }
      button .prefwin.c.ubb -bg $uncov_bgColor -fg $uncov_fgColor -activebackground $uncov_bgColor -activeforeground $uncov_fgColor \
             -text "Change Uncovered Background" -relief $brelief -command {
        set tmp_uncov_bgColor [pref_set_button_color .prefwin.c.ufb .prefwin.c.ubb bg $tmp_uncov_bgColor "Choose Uncovered Background"]
      }

      # Covered selectors
      button .prefwin.c.cfb -bg $cov_bgColor -fg $cov_fgColor -activebackground $cov_bgColor -activeforeground $cov_fgColor \
             -text "Change Covered Foreground" -relief $brelief -command {
        set tmp_cov_fgColor [pref_set_button_color .prefwin.c.cfb .prefwin.c.cbb fg $tmp_cov_fgColor "Choose Covered Foreground"]
      }
      button .prefwin.c.cbb -bg $cov_bgColor -fg $cov_fgColor -activebackground $cov_bgColor -activeforeground $cov_fgColor \
             -text "Change Covered Background" -relief $brelief -command {
        set tmp_cov_bgColor [pref_set_button_color .prefwin.c.cfb .prefwin.c.cbb bg $tmp_cov_bgColor "Choose Covered Background"]
      }

      # Race selectors
      button .prefwin.c.rfb -bg $race_bgColor -fg $race_fgColor -activebackground $race_bgColor -activeforeground $race_fgColor \
             -text "Change Race Condition Foreground" -relief $brelief -command {
        set tmp_race_fgColor [pref_set_button_color .prefwin.c.rfb .prefwin.c.rbb fg $tmp_race_fgColor "Choose Race Condition Foreground"]
      }
      button .prefwin.c.rbb -bg $race_bgColor -fg $race_fgColor -activebackground $race_bgColor -activeforeground $race_fgColor \
             -text "Change Race Condition Background" -relief $brelief -command {
        set tmp_race_bgColor [pref_set_button_color .prefwin.c.rfb .prefwin.c.rbb bg $tmp_race_bgColor "Choose Race Condition Background"]
      } 

      # Pack the color widgets into the color frame
      grid .prefwin.c.l   -row 0 -column 0 -sticky news -pady 4
      grid .prefwin.c.ufb -row 1 -column 0 -sticky news -padx $bpadx -pady $bpady
      grid .prefwin.c.ubb -row 1 -column 1 -sticky news -padx $bpadx -pady $bpady
      grid .prefwin.c.cfb -row 2 -column 0 -sticky news -padx $bpadx -pady $bpady
      grid .prefwin.c.cbb -row 2 -column 1 -sticky news -padx $bpadx -pady $bpady
      grid .prefwin.c.rfb -row 3 -column 0 -sticky news -padx $bpadx -pady $bpady
      grid .prefwin.c.rbb -row 3 -column 1 -sticky news -padx $bpadx -pady $bpady

    #########################
    # Create hit limit area #
    #########################

      # Create widgets
      frame .prefwin.limits -relief raised -borderwidth 1
      label .prefwin.limits.l -anchor w -text "Set Acceptable Coverage Goals"

      label .prefwin.limits.ll -anchor e -text "Line Coverage %:"
      percent_spinner .prefwin.limits.ls $line_low_limit

      label .prefwin.limits.tl -anchor e -text "Toggle Coverage %:"
      percent_spinner .prefwin.limits.ts $toggle_low_limit

      label .prefwin.limits.cl -anchor e -text "Combinational Logic Coverage %:"
      percent_spinner .prefwin.limits.cs $comb_low_limit

      label .prefwin.limits.fl -anchor e -text "FSM State/Arc Coverage %:"
      percent_spinner .prefwin.limits.fs $fsm_low_limit
     
      # Pack widgets into grid
      grid columnconfigure .prefwin.limits 2 -weight 1
      grid .prefwin.limits.l  -row 0 -column 0 -sticky news -pady 4
      grid .prefwin.limits.ll -row 1 -column 0 -sticky news
      grid .prefwin.limits.ls -row 1 -column 1 -sticky news
      grid .prefwin.limits.tl -row 2 -column 0 -sticky news
      grid .prefwin.limits.ts -row 2 -column 1 -sticky news
      grid .prefwin.limits.cl -row 3 -column 0 -sticky news
      grid .prefwin.limits.cs -row 3 -column 1 -sticky news
      grid .prefwin.limits.fl -row 4 -column 0 -sticky news
      grid .prefwin.limits.fs -row 4 -column 1 -sticky news

    ############################
    # Create OK/Cancel buttons #
    ############################

      frame .prefwin.bbar -relief raised -borderwidth 1

      button .prefwin.bbar.ok -width 10 -text "OK" -command {

        set changed 0

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
        if {$line_low_limit != [expr 100 - [.prefwin.limits.ls.l nearest 0]]} {
          set line_low_limit [expr 100 - [.prefwin.limits.ls.l nearest 0]]
          set changed 1
        }
        if {$toggle_low_limit != [expr 100 - [.prefwin.limits.ts.l nearest 0]]} {
          set toggle_low_limit [expr 100 - [.prefwin.limits.ts.l nearest 0]]
          set changed 1
        }
        if {$comb_low_limit != [expr 100 - [.prefwin.limits.cs.l nearest 0]]} {
          set comb_low_limit [expr 100 - [.prefwin.limits.cs.l nearest 0]]
          set changed 1
        }
        if {$fsm_low_limit != [expr 100 - [.prefwin.limits.fs.l nearest 0]]} {
          set fsm_low_limit    [expr 100 - [.prefwin.limits.fs.l nearest 0]]
          set changed 1
        }

        destroy .prefwin

        if {$changed == 1} {

          # Write configuration file with new values
          write_coveredrc

          # Redisplay with new settings
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

          if {[.menubar.tools.menu entrycget 0 -state] != "disabled"} {
            create_summary
          }

        }

      }

      button .prefwin.bbar.cancel -width 10 -text "Cancel" -command {
        destroy .prefwin
      }

      pack .prefwin.bbar.cancel -side right -padx 8 -pady 4
      pack .prefwin.bbar.ok     -side right -padx 8 -pady 4

    ####################################
    # Pack all widgets into the window #
    ####################################

      pack .prefwin.c      -fill x -anchor w
      pack .prefwin.limits -fill x -anchor w
      pack .prefwin.bbar   -fill x -side bottom

  }

  # Bring the window to the top
  raise .prefwin

}

proc pref_set_button_color {fbutton bbutton type origcolor title} {

  # Get color from color chooser window
  set color [tk_chooseColor -initialcolor $origcolor -title $title]

  # If a color was choosen, set the buttons
  if {$color != ""} {

    # Set foreground or background colors on buttons
    if {$type == "fg"} {
      $fbutton configure -fg $color -activeforeground $color
      $bbutton configure -fg $color -activeforeground $color
    } else {
      $fbutton configure -bg $color -activebackground $color
      $bbutton configure -bg $color -activebackground $color
    }

    return $color

  # Otherwise, just return the original color
  } else {

    return $origcolor

  }

}

proc percent_spinner {w showval} {

  global percent_range

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

#######################################################################
# All code below here is just for testing in a stand-alone environment
#######################################################################

proc display_settings {title} {

  global cov_fgColor   cov_bgColor
  global uncov_fgColor uncov_bgColor
  global race_fgColor  race_bgColor
  global line_low_limit toggle_low_limit comb_low_limit fsm_low_limit

  puts "\n$title"
  puts "Uncovered Foreground: $uncov_fgColor"
  puts "Uncovered Background: $uncov_bgColor"
  puts "Covered Foreground:   $cov_fgColor"
  puts "Covered Background:   $cov_bgColor"
  puts "Race Foreground:      $race_fgColor"
  puts "Race Background:      $race_bgColor"
  puts "Line Lower Limit:     $line_low_limit"
  puts "Toggle Lower Limit:   $toggle_low_limit"
  puts "Comb Lower Limit:     $comb_low_limit"
  puts "FSM Lower Limit:      $fsm_low_limit"

}

