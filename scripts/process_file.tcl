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
set toggle_summary_hit01  0
set toggle_summary_hit10  0
set comb_summary_total    0
set comb_summary_hit      0
set curr_mod_name         0

# TODO : 
# Suppose that a really large verilog file has a lot of lines uncovered.
# Obviously, if we use a list and lsearch then the time it will take to
# print the total listing will be a bit too-much. As of now, we are 
# using lsearch, but will certainly optimize later.

proc process_module_line_cov {} {

  global fileContent file_name start_line end_line
  global curr_mod_name

  if {$curr_mod_name != 0} {

    tcl_func_get_filename $curr_mod_name

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

    # Get start and end line numbers of this module
    set start_line 0
    set end_line   0
    tcl_func_get_module_start_and_end $curr_mod_name

    # Get line summary information and display this now
    tcl_func_get_line_summary $curr_mod_name

    calc_and_display_line_cov

  }

}

proc calc_and_display_line_cov {} {

  global cov_type uncov_type mod_inst_type mod_list
  global uncovered_lines covered_lines
  global curr_mod_name

  if {$curr_mod_name != 0} {

    # Get list of uncovered/covered lines
    set uncovered_lines 0
    set covered_lines   0
    tcl_func_collect_uncovered_lines $curr_mod_name
    tcl_func_collect_covered_lines   $curr_mod_name

    display_line_cov

  }

}

proc display_line_cov {} {

  global fileContent file_name
  global uncov_fgColor uncov_bgColor cov_fgColor cov_bgColor
  global uncovered_lines covered_lines uncov_type cov_type
  global start_line end_line
  global line_summary_total line_summary_hit

  # Populate information bar
  if {$file_name != 0} {
    .bot.info configure -text "Filename: $file_name"
  }

  .bot.txt tag configure uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
  .bot.txt tag configure cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor

  # Allow us to write to the text box
  .bot.txt configure -state normal

  # Clear the text-box before any insertion is being made
  .bot.txt delete 1.0 end

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
          .bot.txt insert end $line uncov_colorMap
        } elseif {[expr $cov_type == 1] && [expr [lsearch $covered_lines $linecount] != -1]} {
          .bot.txt insert end $line cov_colorMap
        } else {
          .bot.txt insert end $line
        }
      }
      incr linecount
    }

  }

  # Now cause the text box to be read-only again
  .bot.txt configure -state disabled

  return

}

proc process_module_toggle_cov {} {

  global fileContent file_name start_line end_line
  global curr_mod_name

  if {$curr_mod_name != 0} {

    tcl_func_get_filename $curr_mod_name

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

    # Get start and end line numbers of this module
    set start_line 0
    set end_line   0
    tcl_func_get_module_start_and_end $curr_mod_name

    # Get line summary information and display this now
    tcl_func_get_toggle_summary $curr_mod_name

    calc_and_display_toggle_cov

  }

}

proc calc_and_display_toggle_cov {} {

  global cov_type uncov_type mod_inst_type mod_list
  global uncovered_toggles covered_toggles
  global curr_mod_name start_line
  global toggle_summary_hit toggle_summary_total

  if {$curr_mod_name != 0} {

    # Get list of uncovered/covered lines
    set uncovered_toggles ""
    set covered_toggles   ""
    tcl_func_collect_uncovered_toggles $curr_mod_name $start_line
    tcl_func_collect_covered_toggles   $curr_mod_name $start_line

    # Calculate toggle hit and total values
    if {[llength $covered_toggles] == 0} {
      set toggle_summary_hit 0
      if {[llength $uncovered_toggles] == 0} {
        set toggle_summary_total 0
      } else {
        set toggle_summary_total [llength $uncovered_toggles]
      }
    } else {
      set toggle_summary_hit   [llength $covered_toggles]
      set toggle_summary_total [expr $toggle_summary_hit + [llength $uncovered_toggles]]
    }

    display_toggle_cov

  }

}

proc display_toggle_cov {} {

  global fileContent file_name
  global uncov_fgColor uncov_bgColor cov_fgColor cov_bgColor
  global uncovered_toggles covered_toggles
  global uncov_type cov_type
  global start_line end_line
  global toggle_summary_total toggle_summary_hit
  global cov_rb mod_inst_type mod_list
  global toggle01_verbose toggle10_verbose toggle_width
  global curr_mod_name

  if {$curr_mod_name != 0} {

    # Populate information bar
    .bot.info configure -text "Filename: $file_name"

    .bot.txt tag configure uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
    .bot.txt tag configure cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor

    # Allow us to write to the text box
    .bot.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.txt delete 1.0 end

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
          .bot.txt insert end $line
        }
        incr linecount
      }

      # Finally, set toggle information
      if {[expr $uncov_type == 1] && [expr [llength $uncovered_toggles] > 0]} {
        set cmd_enter  ".bot.txt tag add uncov_enter"
        set cmd_button ".bot.txt tag add uncov_button"
        set cmd_leave  ".bot.txt tag add uncov_leave"
        foreach entry $uncovered_toggles {
          set cmd_enter  [concat $cmd_enter  $entry]
          set cmd_button [concat $cmd_button $entry]
          set cmd_leave  [concat $cmd_leave  $entry]
        }
        eval $cmd_enter
        eval $cmd_button
        eval $cmd_leave
        .bot.txt tag configure uncov_button -underline true -foreground $uncov_fgColor -background $uncov_bgColor
        .bot.txt tag bind uncov_enter <Enter> {
          set curr_cursor [.bot.txt cget -cursor]
          .bot.txt configure -cursor hand2
        }
        .bot.txt tag bind uncov_leave <Leave> {
          .bot.txt configure -cursor $curr_cursor
        }
        .bot.txt tag bind uncov_button <ButtonPress-1> {
          set range [.bot.txt tag prevrange uncov_button {current + 1 chars}]
          create_toggle_window $curr_mod_name [string trim [lindex [split [.bot.txt get [lindex $range 0] [lindex $range 1]] "\["] 0]]
        }
      } 

      if {[expr $cov_type == 1] && [expr [llength $covered_toggles] > 0]} {
        set cmd_cov ".bot.txt tag add cov_highlight"
        foreach entry $covered_toggles {
          set cmd_cov [concat $cmd_cov $entry]
        }
        eval $cmd_cov
        .bot.txt tag configure cov_highlight -foreground $cov_fgColor -background $cov_bgColor
      }

    }

  }

  # Now cause the text box to be read-only again
  .bot.txt configure -state disabled

  return

}
 
proc process_module_comb_cov {} {

  global fileContent file_name start_line end_line
  global curr_mod_name

  if {$curr_mod_name != 0} {

    tcl_func_get_filename $curr_mod_name

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

    # Get start and end line numbers of this module
    set start_line 0
    set end_line   0
    tcl_func_get_module_start_and_end $curr_mod_name

    # Get line summary information and display this now
    tcl_func_get_comb_summary $curr_mod_name

    calc_and_display_comb_cov

  }

} 

proc calc_and_display_comb_cov {} {

  global cov_type uncov_type mod_inst_type mod_list
  global uncovered_combs covered_combs
  global curr_mod_name start_line
  global comb_summary_hit comb_summary_total

  if {$curr_mod_name != 0} {

    # Get list of uncovered/covered combinational logic 
    set uncovered_combs ""
    set covered_combs   ""
    tcl_func_collect_combs $curr_mod_name $start_line

    # Calculate combinational logic hit and total values
    if {[llength $covered_combs] == 0} {
      set comb_summary_hit 0
      if {[llength $uncovered_combs] == 0} {
        set comb_summary_total 0
      } else {
        set comb_summary_total [llength $uncovered_combs]
      }
    } else {
      set comb_summary_hit   [llength $covered_combs]
      set comb_summary_total [expr $comb_summary_hit + [llength $uncovered_combs]]
    }

    display_comb_cov

  }

}

proc display_comb_cov {} {
 
  global fileContent file_name
  global uncov_fgColor uncov_bgColor cov_fgColor cov_bgColor
  global uncovered_combs covered_combs
  global uncov_type cov_type
  global start_line end_line
  global comb_summary_total comb_summary_hit
  global cov_rb mod_inst_type mod_list
  global curr_mod_name

  if {$curr_mod_name != 0} {

    # Populate information bar
    .bot.info configure -text "Filename: $file_name"

    .bot.txt tag configure uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
    .bot.txt tag configure cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor

    # Allow us to write to the text box
    .bot.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.txt delete 1.0 end

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
          .bot.txt insert end $line
        }
        incr linecount
      }

      # Finally, set combinational logic information
      if {[expr $uncov_type == 1] && [expr [llength $uncovered_combs] > 0]} {
        set cmd_enter     ".bot.txt tag add uncov_enter"
        set cmd_button    ".bot.txt tag add uncov_button"
        set cmd_leave     ".bot.txt tag add uncov_leave"
        set cmd_highlight ".bot.txt tag add uncov_highlight"
        foreach entry $uncovered_combs {
          set cmd_highlight [concat $cmd_highlight [lindex $entry 0] [lindex $entry 1]]
          set start_line    [lindex [split [lindex $entry 0] .] 0]
          set end_line      [lindex [split [lindex $entry 1] .] 0]
          if {$start_line != $end_line} {
            set cmd_enter  [concat $cmd_enter  [lindex $entry 0] "$start_line.end"]
            set cmd_button [concat $cmd_button [lindex $entry 0] "$start_line.end"]
            set cmd_leave  [concat $cmd_leave  [lindex $entry 0] "$start_line.end"]
            for {set i [expr $start_line + 1]} {$i <= $end_line} {incr i} {
              set line       [.bot.txt get "$i.7" end]
              set line_diff  [expr [expr [string length $line] - [string length [string trimleft $line]]] + 7]
              if {$i == $end_line} {
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
        .bot.txt tag configure uncov_highlight -foreground $uncov_fgColor -background $uncov_bgColor
        .bot.txt tag configure uncov_button -underline true
        .bot.txt tag bind uncov_enter <Enter> {
          set curr_cursor [.bot.txt cget -cursor]
          .bot.txt configure -cursor hand2
        }
        .bot.txt tag bind uncov_leave <Leave> {
          .bot.txt configure -cursor $curr_cursor
        }
        .bot.txt tag bind uncov_button <ButtonPress-1> {
          set all_ranges [.bot.txt tag ranges uncov_highlight]
          set my_range   [.bot.txt tag prevrange uncov_highlight {current + 1 chars}]
          set index [expr [lsearch -exact $all_ranges [lindex $my_range 0]] / 2]
          set expr_id [lindex [lindex $uncovered_combs $index] 2]
          create_comb_window $curr_mod_name $expr_id
        }
      }

      if {[expr $cov_type == 1] && [expr [llength $covered_combs] > 0]} {
        set cmd_cov ".bot.txt tag add cov_highlight"
        foreach entry $covered_combs {
          set cmd_cov [concat $cmd_cov $entry]
        }
        eval $cmd_cov
        .bot.txt tag configure cov_highlight -foreground $cov_fgColor -background $cov_bgColor
      }

    }

  }

  # Now cause the text box to be read-only again
  .bot.txt configure -state disabled

  return

}

proc process_module_fsm_cov {} {

  global start_line end_line

  display_fsm_cov

}

proc display_fsm_cov {} {

  global fgColor bgColor

  # Configure text area
  .bot.txt tag configure colorMap -foreground $fgColor -background $bgColor

  # Clear the text-box before any insertion is being made
  .bot.txt delete 1.0 end

}
