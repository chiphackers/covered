# This proc will take in a file, the list of uncovered lines and the
# the corresponding uncovered lines will be highlighted in the window

# SPECIAL NOTE : In order to avoid multiple re-read of the same file, it
# is much better to read in a file at the very first time when the
# corresponding tree-node is selected. The file contents are kept as 
# part of the global filecontent hashtable.

set fileContent(0)        0
set file_name             0
set start_line            0
set end_line              0
set line_summary_total    0
set line_summary_hit      0
set toggle_summary_total  0
set toggle_summary_hit    0
set comb_summary_total    0
set comb_summary_hit      0
set fsm_summary_total     0
set fsm_summary_hit       0
set assert_summary_total  0
set assert_summary_hit    0
set curr_funit_name       0
set curr_funit_type       0

# TODO : 
# Suppose that a really large verilog file has a lot of lines uncovered.
# Obviously, if we use a list and lsearch then the time it will take to
# print the total listing will be a bit too-much. As of now, we are 
# using lsearch, but will certainly optimize later.

proc get_race_reason_from_start_line {start_line} {

  global race_reasons race_msgs race_lines

  set i 0
  while {[expr $i < [llength $race_lines]] && [expr [string compare [lindex [lindex $race_lines $i] 0] $start_line] == 0]} {
    incr i
  }

  return [lindex $race_msgs [lindex $race_reasons $i]]

}

proc create_race_tags {} {

  global race_type race_reasons race_lines
  global race_fgColor race_bgColor

  # Set race condition information
  if {[expr $race_type == 1] && [expr [llength $race_reasons] > 0]} {
    set cmd_enter ".bot.right.txt tag add race_enter"
    set cmd_leave ".bot.right.txt tag add race_leave"
    foreach entry $race_lines {
      set cmd_enter [concat $cmd_enter $entry]
      set cmd_leave [concat $cmd_leave $entry]
    }
    eval $cmd_enter
    eval $cmd_leave
    .bot.right.txt tag configure race_enter -foreground $race_fgColor -background $race_bgColor
    .bot.right.txt tag bind race_enter <Enter> {
      set curr_info   [.info cget -text]
      set curr_cursor [.bot.right.txt cget -cursor]
      .bot.right.txt configure -cursor question_arrow
      set reason [get_race_reason_from_start_line [lindex [.bot.right.txt tag prevrange race_enter {current + 1 chars}] 0]]
      .info configure -text "Race condition reason: $reason"
    }
    .bot.right.txt tag bind race_leave <Leave> {
      .bot.right.txt configure -cursor $curr_cursor
      .info          configure -text   $curr_info
    }
  }

}

proc process_funit_line_cov {} {

  global fileContent file_name start_line end_line
  global curr_funit_name curr_funit_type
  global line_summary_hit line_summary_total

  if {$curr_funit_name != 0} {

    tcl_func_get_filename $curr_funit_name $curr_funit_type

    if {[catch {set fileText $fileContent($file_name)}]} {
      if {[catch {set fp [open $file_name "r"]}]} {
        tk_messageBox -message "File $file_name Not Found!" \
                      -title "No File" -icon error
        return
      } 
      set fileText [read $fp]
      set fileContent($file_name) $fileText
      close $fp
    }

    # Get start and end line numbers of this functional unit
    set start_line 0
    set end_line   0
    tcl_func_get_funit_start_and_end $curr_funit_name $curr_funit_type

    # Get line summary information and display this now
    tcl_func_get_line_summary $curr_funit_name $curr_funit_type

    # If we have some uncovered values, enable the "next" pointer and menu item
    if {$line_summary_total != $line_summary_hit} {
      .bot.right.h.next configure -state normal
      .menubar.view.menu entryconfigure 2 -state normal
    } else {
      .bot.right.h.next configure -state disabled
      .menubar.view.menu entryconfigure 2 -state disabled
    }
    .bot.right.h.prev configure -state disabled
    .menubar.view.menu entryconfigure 3 -state disabled

    calc_and_display_line_cov

  }

}

proc calc_and_display_line_cov {} {

  global cov_type uncov_type mod_inst_type funit_names funit_types
  global uncovered_lines covered_lines race_lines race_reasons
  global curr_funit_name curr_funit_type start_line

  if {$curr_funit_name != 0} {

    # Get list of uncovered/covered lines
    set uncovered_lines 0
    set covered_lines   0
    set race_lines      ""
    set race_reasons    ""
    tcl_func_collect_uncovered_lines $curr_funit_name $curr_funit_type
    tcl_func_collect_covered_lines   $curr_funit_name $curr_funit_type
    tcl_func_collect_race_lines      $curr_funit_name $curr_funit_type $start_line

    display_line_cov

  }

}

proc display_line_cov {} {

  global fileContent file_name
  global uncov_fgColor uncov_bgColor
  global cov_fgColor cov_bgColor
  global uncovered_lines covered_lines race_lines
  global uncov_type cov_type
  global start_line end_line
  global line_summary_total line_summary_hit
  global curr_funit_name curr_funit_type

  if {$curr_funit_name != 0} {

    # Populate information bar
    if {$file_name != 0} {
      .info configure -text "Filename: $file_name"
    }

    .bot.right.txt tag configure uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
    .bot.right.txt tag configure cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor

    # Allow us to write to the text box
    .bot.right.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.right.txt delete 1.0 end

    set contents [split $fileContent($file_name) \n]
    set linecount 1

    if {$end_line != 0} {

      # First, populate the summary information
      cov_display_summary $line_summary_hit $line_summary_total

      # Next, populate text box with file contents including highlights for covered/uncovered lines
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%3s  %7u  %s} "   " $linecount [append phrase "\n"]]
          if {[expr $uncov_type == 1] && [expr [lsearch $uncovered_lines $linecount] != -1]} {
            .bot.right.txt insert end $line uncov_colorMap
          } elseif {[expr $cov_type == 1] && [expr [lsearch $covered_lines $linecount] != -1]} {
            .bot.right.txt insert end $line cov_colorMap
          } else {
            .bot.right.txt insert end $line
          }
        }
        incr linecount
      }

      # Create race condition tags
      create_race_tags

    }

    # Now cause the text box to be read-only again
    .bot.right.txt configure -state disabled

  }

  return

}

proc process_funit_toggle_cov {} {

  global fileContent file_name start_line end_line
  global curr_funit_name curr_funit_type
  global toggle_summary_hit toggle_summary_total

  if {$curr_funit_name != 0} {

    tcl_func_get_filename $curr_funit_name $curr_funit_type

    if {[catch {set fileText $fileContent($file_name)}]} {
      if {[catch {set fp [open $file_name "r"]}]} {
        tk_messageBox -message "File $file_name Not Found!" \
                      -title "No File" -icon error
        return
      } 
      set fileText [read $fp]
      set fileContent($file_name) $fileText
      close $fp
    }

    # Get start and end line numbers of this functional unit
    set start_line 0
    set end_line   0
    tcl_func_get_funit_start_and_end $curr_funit_name $curr_funit_type

    # Get line summary information and display this now
    tcl_func_get_toggle_summary $curr_funit_name $curr_funit_type

    # If we have some uncovered values, enable the "next" pointer
    if {$toggle_summary_total != $toggle_summary_hit} {
      .bot.right.h.next configure -state normal
    } else {
      .bot.right.h.next configure -state disabled
    }
    .bot.right.h.prev configure -state disabled

    calc_and_display_toggle_cov

  }

}

proc calc_and_display_toggle_cov {} {

  global cov_type uncov_type mod_inst_type
  global uncovered_toggles covered_toggles race_toggles
  global curr_funit_name curr_funit_type start_line

  if {$curr_funit_name != 0} {

    # Get list of uncovered/covered lines
    set uncovered_toggles ""
    set covered_toggles   ""
    tcl_func_collect_uncovered_toggles $curr_funit_name $curr_funit_type $start_line
    tcl_func_collect_covered_toggles   $curr_funit_name $curr_funit_type $start_line

    display_toggle_cov

  }

}

proc display_toggle_cov {} {

  global fileContent file_name
  global uncov_fgColor uncov_bgColor
  global cov_fgColor cov_bgColor
  global uncovered_toggles covered_toggles
  global uncov_type cov_type
  global start_line end_line
  global toggle_summary_total toggle_summary_hit
  global cov_rb mod_inst_type
  global toggle01_verbose toggle10_verbose toggle_width
  global curr_funit_name curr_funit_type

  if {$curr_funit_name != 0} {

    # Populate information bar
    .info configure -text "Filename: $file_name"

    # Allow us to write to the text box
    .bot.right.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.right.txt delete 1.0 end

    set contents [split $fileContent($file_name) \n]
    set linecount 1

    if {$end_line != 0} {

      # First, populate the summary information
      cov_display_summary $toggle_summary_hit $toggle_summary_total

      # Next, populate text box with file contents
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%3s  %7u  %s} "   " $linecount [append phrase "\n"]]
          .bot.right.txt insert end $line
        }
        incr linecount
      }

      # Create race condition tags
      create_race_tags

      # Finally, set toggle information
      if {[expr $uncov_type == 1] && [expr [llength $uncovered_toggles] > 0]} {
        set cmd_enter  ".bot.right.txt tag add uncov_enter"
        set cmd_button ".bot.right.txt tag add uncov_button"
        set cmd_leave  ".bot.right.txt tag add uncov_leave"
        foreach entry $uncovered_toggles {
          set cmd_enter  [concat $cmd_enter  $entry]
          set cmd_button [concat $cmd_button $entry]
          set cmd_leave  [concat $cmd_leave  $entry]
        }
        eval $cmd_enter
        eval $cmd_button
        eval $cmd_leave
        .bot.right.txt tag configure uncov_button -underline true -foreground $uncov_fgColor -background $uncov_bgColor
        .bot.right.txt tag bind uncov_enter <Enter> {
          set curr_cursor [.bot.right.txt cget -cursor]
          set curr_info   [.info cget -text]
          .bot.right.txt configure -cursor hand2
          .info configure -text "Click left button for detailed toggle coverage information"
        }
        .bot.right.txt tag bind uncov_leave <Leave> {
          .bot.right.txt configure -cursor $curr_cursor
          .info configure -text $curr_info
        }
        .bot.right.txt tag bind uncov_button <ButtonPress-1> {
          display_toggle current
        }
      } 

      if {[expr $cov_type == 1] && [expr [llength $covered_toggles] > 0]} {
        set cmd_cov ".bot.right.txt tag add cov_highlight"
        foreach entry $covered_toggles {
          set cmd_cov [concat $cmd_cov $entry]
        }
        eval $cmd_cov
        .bot.right.txt tag configure cov_highlight -foreground $cov_fgColor -background $cov_bgColor
      }

    }

    # Now cause the text box to be read-only again
    .bot.right.txt configure -state disabled

  }

  return

}
 
proc process_funit_comb_cov {} {

  global fileContent file_name start_line end_line
  global curr_funit_name curr_funit_type
  global comb_summary_total comb_summary_hit

  if {$curr_funit_name != 0} {

    tcl_func_get_filename $curr_funit_name $curr_funit_type

    if {[catch {set fileText $fileContent($file_name)}]} {
      if {[catch {set fp [open $file_name "r"]}]} {
        tk_messageBox -message "File $file_name Not Found!" \
                      -title "No File" -icon error
        return
      }
      set fileText [read $fp]
      set fileContent($file_name) $fileText
      close $fp
    }

    # Get start and end line numbers of this functional unit
    set start_line 0
    set end_line   0
    tcl_func_get_funit_start_and_end $curr_funit_name $curr_funit_type

    # Get line summary information and display this now
    tcl_func_get_comb_summary $curr_funit_name $curr_funit_type

    # If we have found uncovered combinations in this module, enable the next button
    if {$comb_summary_total != $comb_summary_hit} {
      .bot.right.h.next configure -state normal
    } else {
      .bot.right.h.next configure -state disabled
    }
    .bot.right.h.prev configure -state disabled

    calc_and_display_comb_cov

  }

} 

proc calc_and_display_comb_cov {} {

  global cov_type uncov_type mod_inst_type
  global uncovered_combs covered_combs race_lines race_reasons
  global curr_funit_name curr_funit_type start_line

  if {$curr_funit_name != 0} {

    # Get list of uncovered/covered combinational logic 
    set uncovered_combs ""
    set covered_combs   ""
    set race_lines      ""
    set race_reasons    ""
    tcl_func_collect_combs $curr_funit_name $curr_funit_type $start_line
    tcl_func_collect_race_lines $curr_funit_name $curr_funit_type $start_line

    display_comb_cov

  }

}

proc display_comb_cov {} {
 
  global fileContent file_name
  global uncov_fgColor uncov_bgColor
  global cov_fgColor cov_bgColor
  global uncovered_combs covered_combs race_lines
  global uncov_type cov_type
  global start_line end_line
  global comb_summary_total comb_summary_hit
  global cov_rb mod_inst_type
  global curr_funit_name curr_funit_type

  if {$curr_funit_name != 0} {

    # Populate information bar
    .info configure -text "Filename: $file_name"

    # Allow us to write to the text box
    .bot.right.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.right.txt delete 1.0 end

    set contents [split $fileContent($file_name) \n]
    set linecount 1

    if {$end_line != 0} {

      # First, populate the summary information
      cov_display_summary $comb_summary_hit $comb_summary_total

      # Next, populate text box with file contents including highlights for covered/uncovered lines
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%3s  %7u  %s} "   " $linecount [append phrase "\n"]]
          .bot.right.txt insert end $line
        }
        incr linecount
      }

      # Create race condition tags
      create_race_tags

      # Finally, set combinational logic information
      if {[expr $uncov_type == 1] && [expr [llength $uncovered_combs] > 0]} {
        set cmd_enter     ".bot.right.txt tag add uncov_enter"
        set cmd_button    ".bot.right.txt tag add uncov_button"
        set cmd_leave     ".bot.right.txt tag add uncov_leave"
        set cmd_highlight ".bot.right.txt tag add uncov_highlight"
        foreach entry $uncovered_combs {
          set cmd_highlight [concat $cmd_highlight [lindex $entry 0] [lindex $entry 1]]
          set sline [lindex [split [lindex $entry 0] .] 0]
          set eline [lindex [split [lindex $entry 1] .] 0]
          if {$sline != $eline} {
            set cmd_enter  [concat $cmd_enter  [lindex $entry 0] "$sline.end"]
            set cmd_button [concat $cmd_button [lindex $entry 0] "$sline.end"]
            set cmd_leave  [concat $cmd_leave  [lindex $entry 0] "$sline.end"]
            for {set i [expr $sline + 1]} {$i <= $eline} {incr i} {
              set line       [.bot.right.txt get "$i.7" end]
              set line_diff  [expr [expr [string length $line] - [string length [string trimleft $line]]] + 7]
              if {$i == $eline} {
                set cmd_enter  [concat $cmd_enter  "$i.$line_diff" [lindex $entry 1]]
                set cmd_button [concat $cmd_button "$i.$line_diff" [lindex $entry 1]]
                set cmd_leave  [concat $cmd_leave  "$i.$line_diff" [lindex $entry 1]]
              } else {
                set cmd_enter  [concat $cmd_enter  "$i.$line_diff" "$i.end"]
                set cmd_button [concat $cmd_button "$i.$line_diff" "$i.end"]
                set cmd_leave  [concat $cmd_leave  "$i.$line_diff" "$i.end"]
              }
            }
          } else {
            set cmd_enter  [concat $cmd_enter  [lindex $entry 0] [lindex $entry 1]]
            set cmd_button [concat $cmd_button [lindex $entry 0] [lindex $entry 1]]
            set cmd_leave  [concat $cmd_leave  [lindex $entry 0] [lindex $entry 1]]
          }
        }
        eval $cmd_enter
        eval $cmd_button
        eval $cmd_leave
        eval $cmd_highlight
        .bot.right.txt tag configure uncov_highlight -foreground $uncov_fgColor -background $uncov_bgColor
        .bot.right.txt tag configure uncov_button -underline true
        .bot.right.txt tag bind uncov_enter <Enter> {
          set curr_cursor [.bot.right.txt cget -cursor]
          set curr_info   [.info cget -text]
          .bot.right.txt configure -cursor hand2
          .info configure -text "Click left button for detailed combinational logic coverage information" 
        }
        .bot.right.txt tag bind uncov_leave <Leave> {
          .bot.right.txt configure -cursor $curr_cursor
          .info configure -text $curr_info
        }
        .bot.right.txt tag bind uncov_button <ButtonPress-1> {
          display_comb current
        }
      }

      if {[expr $cov_type == 1] && [expr [llength $covered_combs] > 0]} {
        set cmd_cov ".bot.right.txt tag add cov_highlight"
        foreach entry $covered_combs {
          set cmd_cov [concat $cmd_cov $entry]
        }
        eval $cmd_cov
        .bot.right.txt tag configure cov_highlight -foreground $cov_fgColor -background $cov_bgColor
      }

    }

    # Now cause the text box to be read-only again
    .bot.right.txt configure -state disabled

  }

  return

}

proc process_funit_fsm_cov {} {

  global fileContent file_name start_line end_line
  global curr_funit_name curr_funit_type
  global fsm_summary_hit fsm_summary_total

  if {$curr_funit_name != 0} {

    tcl_func_get_filename $curr_funit_name $curr_funit_type

    if {[catch {set fileText $fileContent($file_name)}]} {
      if {[catch {set fp [open $file_name "r"]}]} {
        tk_messageBox -message "File $file_name Not Found!" \
                      -title "No File" -icon error
        return
      }
      set fileText [read $fp]
      set fileContent($file_name) $fileText
      close $fp
    }

    # Get start and end line numbers of this functional unit
    set start_line 0
    set end_line   0
    tcl_func_get_funit_start_and_end $curr_funit_name $curr_funit_type

    # Get line summary information and display this now
    tcl_func_get_fsm_summary $curr_funit_name $curr_funit_type

    # If we have some uncovered values, enable the "next" pointer and menu item
    if {$fsm_summary_total != $fsm_summary_hit} {
      .bot.right.h.next configure -state normal
      .menubar.view.menu entryconfigure 2 -state normal
    } else {
      .bot.right.h.next configure -state disabled
      .menubar.view.menu entryconfigure 2 -state disabled
    }
    .bot.right.h.prev configure -state disabled
    .menubar.view.menu entryconfigure 3 -state disabled

    calc_and_display_fsm_cov

  }

}

proc calc_and_display_fsm_cov {} {

  global cov_type uncov_type mod_inst_type funit_names funit_types
  global uncovered_fsms covered_fsms race_lines race_reasons
  global curr_funit_name curr_funit_type start_line

  if {$curr_funit_name != 0} {

    # Get list of uncovered/covered FSMs
    set uncovered_fsms ""
    set covered_fsms   ""
    set race_lines     ""
    set race_reasons   ""
    tcl_func_collect_fsms       $curr_funit_name $curr_funit_type $start_line
    tcl_func_collect_race_lines $curr_funit_name $curr_funit_type $start_line

    display_fsm_cov

  }

}

proc display_fsm_cov {} {

  global uncov_fgColor uncov_bgColor
  global cov_fgColor cov_bgColor
  global curr_funit_name file_name fileContent
  global fsm_summary_hit fsm_summary_total uncov_type cov_type
  global covered_fsms uncovered_fsms
  global start_line end_line

  if {$curr_funit_name != 0} {

    # Populate information bar
    if {$file_name != 0} {
      .info configure -text "Filename: $file_name"
    }

    # Allow us to write to the text box
    .bot.right.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.right.txt delete 1.0 end

    set contents [split $fileContent($file_name) \n]
    set linecount 1

    if {$end_line != 0} {

      # First, populate the summary information
      cov_display_summary $fsm_summary_hit $fsm_summary_total

      # Next, populate text box with file contents including highlights for covered/uncovered lines
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%3s  %7u  %s} "   " $linecount [append phrase "\n"]]
          .bot.right.txt insert end $line
        }
        incr linecount
      }

      # Create race condition tags
      create_race_tags

      # Finally, set FSM information
      if {[expr $uncov_type == 1] && [expr [llength $uncovered_fsms] > 0]} {
        set cmd_enter  ".bot.right.txt tag add uncov_enter"
        set cmd_button ".bot.right.txt tag add uncov_button"
        set cmd_leave  ".bot.right.txt tag add uncov_leave"
        foreach entry $uncovered_fsms {
          set cmd_enter  [concat $cmd_enter  [lindex $entry 0] [lindex $entry 1]]
          set cmd_button [concat $cmd_button [lindex $entry 0] [lindex $entry 1]]
          set cmd_leave  [concat $cmd_leave  [lindex $entry 0] [lindex $entry 1]]
        }
        eval $cmd_enter
        eval $cmd_button
        eval $cmd_leave
        .bot.right.txt tag configure uncov_button -underline true -foreground $uncov_fgColor -background $uncov_bgColor
        .bot.right.txt tag bind uncov_enter <Enter> {
          set curr_cursor [.bot.right.txt cget -cursor]
          set curr_info   [.info cget -text]
          .bot.right.txt configure -cursor hand2
          .info configure -text "Click left button for detailed FSM coverage information"
        }
        .bot.right.txt tag bind uncov_leave <Leave> {
          .bot.right.txt configure -cursor $curr_cursor
          .info configure -text $curr_info
        }
        .bot.right.txt tag bind uncov_button <ButtonPress-1> {
          display_fsm current
        }
      }

      if {[expr $cov_type == 1] && [expr [llength $covered_fsms] > 0]} {
        set cmd_cov ".bot.right.txt tag add cov_highlight"
        foreach entry $covered_fsms {
          set cmd_cov [concat $cmd_cov [lindex $entry 0] [lindex $entry 1]]
        }
        eval $cmd_cov
        .bot.right.txt tag configure cov_highlight -foreground $cov_fgColor -background $cov_bgColor
      }

    }

    # Now cause the text box to be read-only again
    .bot.right.txt configure -state disabled

  }

}

proc process_funit_assert_cov {} {

  global fileContent file_name start_line end_line
  global curr_funit_name curr_funit_type
  global assert_summary_hit assert_summary_total

  if {$curr_funit_name != 0} {

    tcl_func_get_filename $curr_funit_name $curr_funit_type

    if {[catch {set fileText $fileContent($file_name)}]} {
      if {[catch {set fp [open $file_name "r"]}]} {
        tk_messageBox -message "File $file_name Not Found!" \
                      -title "No File" -icon error
        return
      }
      set fileText [read $fp]
      set fileContent($file_name) $fileText
      close $fp
    }

    # Get start and end line numbers of this functional unit
    set start_line 0
    set end_line   0
    tcl_func_get_funit_start_and_end $curr_funit_name $curr_funit_type

    # Get assertion summary information and display this now
    tcl_func_get_assert_summary $curr_funit_name $curr_funit_type

    # If we have some uncovered values, enable the "next" pointer and menu item
    if {$assert_summary_total != $assert_summary_hit} {
      .bot.right.h.next configure -state normal
      .menubar.view.menu entryconfigure 2 -state normal
    } else {
      .bot.right.h.next configure -state disabled
      .menubar.view.menu entryconfigure 2 -state disabled
    }
    .bot.right.h.prev configure -state disabled
    .menubar.view.menu entryconfigure 3 -state disabled

    calc_and_display_assert_cov

  }

}

proc calc_and_display_assert_cov {} {

  global cov_type uncov_type mod_inst_type funit_names funit_types
  global uncovered_asserts covered_asserts race_lines race_reasons
  global curr_funit_name curr_funit_type start_line

  if {$curr_funit_name != 0} {

    # Get list of uncovered/covered assertions
    set uncovered_asserts ""
    set covered_asserts   ""
    set race_lines        ""
    set race_reasons      ""
    tcl_func_collect_assertions $curr_funit_name $curr_funit_type
    tcl_func_collect_race_lines $curr_funit_name $curr_funit_type $start_line

    display_assert_cov

  }
}

proc display_assert_cov {} {

  global uncov_fgColor uncov_bgColor
  global cov_fgColor cov_bgColor
  global curr_funit_name file_name fileContent
  global assert_summary_hit assert_summary_total uncov_type cov_type
  global covered_asserts uncovered_asserts
  global start_line end_line

  if {$curr_funit_name != 0} {

    # Populate information bar
    if {$file_name != 0} {
      .info configure -text "Filename: $file_name"
    }

    # Allow us to write to the text box
    .bot.right.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.right.txt delete 1.0 end

    set contents [split $fileContent($file_name) \n]
    set linecount 1

    if {$end_line != 0} {

      # First, populate the summary information
      cov_display_summary $assert_summary_hit $assert_summary_total

      # Next, populate text box with file contents including highlights for covered/uncovered lines
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%3s  %7u  %s} "   " $linecount [append phrase "\n"]]
          .bot.right.txt insert end $line
        }
        incr linecount
      }

      # Create race condition tags
      create_race_tags

      # Finally, set assertion information
      if {[expr $uncov_type == 1] && [expr [llength $uncovered_asserts] > 0]} {
        set cmd_enter  ".bot.right.txt tag add uncov_enter"
        set cmd_button ".bot.right.txt tag add uncov_button"
        set cmd_leave  ".bot.right.txt tag add uncov_leave"
        foreach entry $uncovered_asserts {
          set match_str ""
          append match_str {[^a-zA-Z0-9_]} $entry {[^a-zA-Z0-9_]}
          set start_index [.bot.right.txt index "[.bot.right.txt search -count matching_chars -regexp $match_str 1.0] + 1 chars"]
          set end_index   [.bot.right.txt index "$start_index + [expr $matching_chars - 2] chars"]
          set cmd_enter  [concat $cmd_enter  $start_index $end_index]
          set cmd_button [concat $cmd_button $start_index $end_index]
          set cmd_leave  [concat $cmd_leave  $start_index $end_index]
        }
        eval $cmd_enter
        eval $cmd_button
        eval $cmd_leave
        .bot.right.txt tag configure uncov_button -underline true -foreground $uncov_fgColor -background $uncov_bgColor
        .bot.right.txt tag bind uncov_enter <Enter> {
          set curr_cursor [.bot.right.txt cget -cursor]
          set curr_info   [.info cget -text]
          .bot.right.txt configure -cursor hand2
          .info configure -text "Click left button for detailed assertion coverage information"
        }
        .bot.right.txt tag bind uncov_leave <Leave> {
          .bot.right.txt configure -cursor $curr_cursor
          .info configure -text $curr_info
        }
        .bot.right.txt tag bind uncov_button <ButtonPress-1> {
          display_assert current
        }
      }

      if {[expr $cov_type == 1] && [expr [llength $covered_asserts] > 0]} {
        set cmd_cov ".bot.right.txt tag add cov_highlight"
        foreach entry $covered_asserts {
          set match_str ""
          append match_str {[^a-zA-Z0-9_]} $entry {[^a-zA-Z0-9_]}
          set start_index [.bot.right.txt index "[.bot.right.txt search -count matching_chars -regexp $match_str 1.0] + 1 chars"]
          set end_index   [.bot.right.txt index "$start_index + [expr $matching_chars - 2] chars"]
          set cmd_cov [concat $cmd_cov $start_index $end_index]
        }
        eval $cmd_cov
        .bot.right.txt tag configure cov_highlight -foreground $cov_fgColor -background $cov_bgColor
      }

    }

    # Now cause the text box to be read-only again
    .bot.right.txt configure -state disabled

  }

}
