# This proc will take in a file, the list of uncovered lines and the
# the corresponding uncovered lines will be highlighted in the window

# SPECIAL NOTE : In order to avoid multiple re-read of the same file, it
# is much better to read in a file at the very first time when the
# corresponding tree-node is selected. The file contents are kept as 
# part of the global filecontent hashtable.

set fileContent(0)     0
set file_name          0
set start_line         0
set end_line           0
set line_summary_total 0
set line_summary_hit   0

# TODO : 
# Suppose that a really large verilog file has a lot of lines uncovered.
# Obviously, if we use a list and lsearch then the time it will take to
# print the total listing will be a bit too-much. As of now, we are 
# using lsearch, but will certainly optimize later.

proc process_module_line_cov {mod_name textArea} {

  global fileContent file_name start_line end_line

  tcl_func_get_filename $mod_name

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
  tcl_func_get_module_start_and_end $mod_name

  # Get line summary information and display this now
  tcl_func_get_line_summary $mod_name

  calc_and_display_line_cov

}

proc calc_and_display_line_cov {} {

  global cov_type uncov_type mod_inst_type mod_list
  global uncovered_lines covered_lines

  set index [.bot.l curselection]

  if {$index != ""} {

    if {$mod_inst_type == "instance"} {
      set mod_name [lindex $mod_list $index]
    } else {
      set mod_name [.bot.l get $index]
    }

    # Get list of uncovered/covered lines
    if {$uncov_type == 1} {
      set uncovered_lines 0
      tcl_func_collect_uncovered_lines $mod_name
    }
    if {$cov_type == 1} {
      set covered_lines 0
      tcl_func_collect_covered_lines $mod_name
    }

    display_line_cov

  }

}

proc display_line_cov {} {

  global fileContent file_name
  global uncov_fgColor uncov_bgColor cov_fgColor cov_bgColor
  global uncovered_lines covered_lines uncov_type cov_type
  global start_line end_line
  global line_summary_total line_summary_hit

  .bot.txt tag configure uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
  .bot.txt tag configure cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor

  # Clear the text-box before any insertion is being made
  .bot.txt delete 1.0 end

  set contents [split $fileContent($file_name) \n]
  set linecount 1

  if {$end_line != 0} {

    # First, populate the summary information
    .covbox.ht configure -text "$line_summary_hit"
    .covbox.tt configure -text "$line_summary_total"

    # Write header information
    .bot.txt insert end "Line #   Verilog Source\n-------  --------------\n"
  
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

  return

}

proc process_module_toggle_cov {mod_name textArea} {

  global start_line end_line

  display_toggle_cov .bot.txt $start_line $end_line

}

proc display_toggle_cov {textArea start end} {

  global fgColor bgColor

  # Configure text area
  $textArea tag configure colorMap -foreground $fgColor -background $bgColor

  # Clear the text-box before any insertion is being made
  $textArea delete 1.0 end

}
 
proc process_module_comb_cov {mod_name textArea} {

  global start_line end_line

  display_comb_cov .bot.txt $start_line $end_line

} 

proc display_comb_cov {textArea start end} {
 
  global fgColor bgColor

  # Configure text area
  $textArea tag configure colorMap -foreground $fgColor -background $bgColor

  # Clear the text-box before any insertion is being made
  $textArea delete 1.0 end

}

proc process_module_fsm_cov {mod_name textArea} {

  global start_line end_line

  display_fsm_cov .bot.txt $start_line $end_line

}

proc display_fsm_cov {textArea start end} {

  global fgColor bgColor

  # Configure text area
  $textArea tag configure colorMap -foreground $fgColor -background $bgColor

  # Clear the text-box before any insertion is being made
  $textArea delete 1.0 end

}
