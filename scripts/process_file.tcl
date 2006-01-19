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
set curr_funit_name       0
set curr_funit_type       0

# TODO : 
# Suppose that a really large verilog file has a lot of lines uncovered.
# Obviously, if we use a list and lsearch then the time it will take to
# print the total listing will be a bit too-much. As of now, we are 
# using lsearch, but will certainly optimize later.

proc create_race_tags {} {

  global race_type race_reasons race_lines

  # Set race condition information
  if {[expr $race_type == 1] && [expr [llength $race_reasons] > 0]} {
    set cmd_enter ".bot.right.txt tag add race_enter"
    set cmd_leave ".bot.right.txt tag add race_leave"
    foreach entry $race_lines {
      set cmd_enter [concat $cmd_enter "$entry.0" "$entry.end"]
      set cmd_leave [concat $cmd_leave "$entry.0" "$entry.end"]
    }
    eval $cmd_enter
    eval $cmd_leave
    .bot.right.txt tag bind race_enter <Enter> {
      set curr_info   [.info cget -text]
      set curr_cursor [.bot.right.txt cget -cursor]
      .bot.right.txt configure -cursor question_arrow
      set curr_line [lindex [split [.bot.right.txt index {current + 1 chars}] .] 0]
      set reason    [lindex $race_msgs [lindex $race_reasons [lsearch $race_lines $curr_line]]]
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

    calc_and_display_line_cov

  }

}

proc calc_and_display_line_cov {} {

  global cov_type uncov_type race_type mod_inst_type funit_names funit_types
  global uncovered_lines covered_lines race_lines
  global curr_funit_name curr_funit_type

  if {$curr_funit_name != 0} {

    # Get list of uncovered/covered lines
    set uncovered_lines 0
    set covered_lines   0
    set race_lines      0
    tcl_func_collect_uncovered_lines $curr_funit_name $curr_funit_type
    tcl_func_collect_covered_lines   $curr_funit_name $curr_funit_type
    tcl_func_collect_race_lines      $curr_funit_name $curr_funit_type

    display_line_cov

  }

}

proc display_line_cov {} {

  global fileContent file_name
  global uncov_fgColor uncov_bgColor
  global cov_fgColor cov_bgColor
  global race_fgColor race_bgColor
  global uncovered_lines covered_lines race_lines
  global uncov_type cov_type race_type
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
    .bot.right.txt tag configure race_colorMap  -foreground $race_fgColor  -background $race_bgColor

    # Allow us to write to the text box
    .bot.right.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.right.txt delete 1.0 end

    set contents [split $fileContent($file_name) \n]
    set linecount 1

    if {$end_line != 0} {

      # First, populate the summary information
      .covbox.ht configure -text "$line_summary_hit"
      .covbox.tt configure -text "$line_summary_total"

      # Next, populate text box with file contents including highlights for covered/uncovered lines
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%7d  %s} $linecount [append phrase "\n"]]
          if {[expr $uncov_type == 1] && [expr [lsearch $uncovered_lines $linecount] != -1]} {
            .bot.right.txt insert end $line uncov_colorMap
          } elseif {[expr $cov_type == 1] && [expr [lsearch $covered_lines $linecount] != -1]} {
            .bot.right.txt insert end $line cov_colorMap
          } elseif {[expr $race_type == 1] && [expr [lsearch $race_lines $linecount] != -1]} {
            .bot.right.txt insert end $line race_colorMap
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
      .covbox.ht configure -text "$toggle_summary_hit"
      .covbox.tt configure -text "$toggle_summary_total"

      # Next, populate text box with file contents including highlights for covered/uncovered lines
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%7d  %s} $linecount [append phrase "\n"]]
          .bot.right.txt insert end $line
        }
        incr linecount
      }

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
          set range [.bot.right.txt tag prevrange uncov_button {current + 1 chars}]
          create_toggle_window $curr_funit_name $curr_funit_type [string trim [lindex [split [.bot.right.txt get [lindex $range 0] [lindex $range 1]] "\["] 0]]
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

    calc_and_display_comb_cov

  }

} 

proc calc_and_display_comb_cov {} {

  global cov_type uncov_type race_type mod_inst_type
  global uncovered_combs covered_combs race_lines
  global curr_funit_name curr_funit_type start_line

  if {$curr_funit_name != 0} {

    # Get list of uncovered/covered combinational logic 
    set uncovered_combs ""
    set covered_combs   ""
    set race_lines      ""
    tcl_func_collect_combs $curr_funit_name $curr_funit_type $start_line
    tcl_func_collect_race_lines $curr_funit_name $curr_funit_type

    display_comb_cov

  }

}

proc display_comb_cov {} {
 
  global fileContent file_name
  global uncov_fgColor uncov_bgColor
  global cov_fgColor cov_bgColor
  global race_fgColor race_bgColor
  global uncovered_combs covered_combs race_lines
  global uncov_type cov_type race_type
  global start_line end_line
  global comb_summary_total comb_summary_hit
  global cov_rb mod_inst_type
  global curr_funit_name curr_funit_type

  if {$curr_funit_name != 0} {

    # Populate information bar
    .info configure -text "Filename: $file_name"

    .bot.right.txt tag configure uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
    .bot.right.txt tag configure cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor
    .bot.right.txt tag configure race_colorMap  -foreground $race_fgColor  -background $race_bgColor

    # Allow us to write to the text box
    .bot.right.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.right.txt delete 1.0 end

    set contents [split $fileContent($file_name) \n]
    set linecount 1

    if {$end_line != 0} {

      # First, populate the summary information
      .covbox.ht configure -text "$comb_summary_hit"
      .covbox.tt configure -text "$comb_summary_total"

      # Next, populate text box with file contents including highlights for covered/uncovered lines
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%7d  %s} $linecount [append phrase "\n"]]
          if {[expr $race_type == 1] && [expr [lsearch $race_lines $linecount] != -1]} {
            .bot.right.txt insert end $line race_colorMap
          } else {
            .bot.right.txt insert end $line
          }
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
          set all_ranges [.bot.right.txt tag ranges uncov_highlight]
          set my_range   [.bot.right.txt tag prevrange uncov_highlight {current + 1 chars}]
          set index [expr [lsearch -exact $all_ranges [lindex $my_range 0]] / 2]
          set expr_id [lindex [lindex $uncovered_combs $index] 2]
          create_comb_window $curr_funit_name $curr_funit_type $expr_id
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

  global start_line end_line

  display_fsm_cov

}

proc display_fsm_cov {} {

  global fgColor bgColor

  # Configure text area
  .bot.right.txt tag configure colorMap -foreground $fgColor -background $bgColor

  # Clear the text-box before any insertion is being made
  .bot.right.txt delete 1.0 end

}
