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

set rank_img_checked   [image create photo -file [file join $HOME scripts checked.gif]]
set rank_img_unchecked [image create photo -file [file join $HOME scripts unchecked.gif]]

proc create_rank_cdds {} {

  global rankgen_sel rankgen_fname rank_view rank_sname rank_rname rank_req_check_cnt
  global cdd_files

  if {[winfo exists .rankwin] == 0} {

    toplevel .rankwin
    wm title .rankwin "Create CDD Coverage Ranking"
    wm transient .rankwin .

    # Create labelframe that will hold the contents
    panedwindow .rankwin.p

    # Initialize global variables
    set rankgen_sel        "options"
    set rankgen_fname      ""
    set rank_view          0
    set rank_sname         ""
    set rank_rname         ""
    set rank_req_check_cnt 0

    # Add panes
    .rankwin.p add [create_rank_cdds_source  .rankwin.p.source] -width 750 -height 500
    .rankwin.p add [create_rank_cdds_options .rankwin.p.opts]   -width 750 -height 500 -hide true
    .rankwin.p add [create_rank_cdds_files   .rankwin.p.files]  -width 750 -height 500 -hide true
    .rankwin.p add [create_rank_cdds_output  .rankwin.p.output] -width 750 -height 500 -hide true

    # Allow currently opened CDD files to be ranked, if they exist
    if {[llength $cdd_files] > 0} {
      .rankwin.p.files.f.b.addcur configure -state normal
    }

    # Pack the panedwindow
    pack .rankwin.p -fill both -expand yes

  }

  # Finally, raise this window
  raise .rankwin

}

proc read_rank_option_file {w fname} {

  global rank_filename rank_rname
  global weight_line weight_toggle weight_memory weight_comb weight_fsm weight_assert
  global weight_line_num weight_toggle_num weight_memory_num weight_comb_num weight_fsm_num weight_assert_num
  global names_only rank_verbose
  global rank_img_checked rank_img_unchecked rank_req_check_cnt

  if {[catch {set fp [open $fname "r"]}]} {
    tk_messageBox -message "File $fname Not Found!" -title "No File" -icon error
  }

  set contents [join [list [read $fp]]]

  for {set i 0} {$i<[llength $contents]} {incr i} { 

    if {[lindex $contents $i] eq "-o"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set rank_filename [lindex $contents $i]
        handle_rank_cdds_filename .rankwin.p.opts
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-v"} {
      set rank_verbose 1

    } elseif {[lindex $contents $i] eq "-required-list"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set rank_rname [lindex $contents $i]
        if {[file isfile $rank_rname] == 1} {
          set fp     [open $rank_rname "r"]
          set fnames [join [list [read $fp]]]
          foreach fname $fnames {
            set index [lsearch -index 1 [.rankwin.p.files.f.t.lb get 0 end] $fname]
            if {$index == -1} {
              .rankwin.p.files.f.t.lb insert end [list 1 $fname]
              .rankwin.p.files.f.t.lb cellconfigure end,required -image $rank_img_checked
            } else {
              .rankwin.p.files.f.t.lb cellconfigure $index,required -image $rank_img_checked
            }
            incr rank_req_check_cnt
          }
          close $fp
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-required-cdd"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[file isfile [lindex $contents $i]] == 1} {
          set index [lsearch -index 1 [.rankwin.p.files.f.t.lb get 0 end] [lindex $contents $i]]
          if {$index == -1} {
            .rankwin.p.files.f.t.lb insert end [list 1 [lindex $contents $i]]
            .rankwin.p.files.f.t.lb cellconfigure end,required -image $rank_img_checked
          } else {
            .rankwin.p.files.f.t.lb cellconfigure $index,required -image $rank_img_checked
          }
          incr rank_req_check_cnt
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-names-only"} {
      set names_only 1
   
    } elseif {[lindex $contents $i] eq "-weight-line"} {
      incr i
      set value [lindex $contents $i]
      if {[string index $value 0] ne "-"} {
        if {[string is integer $value] && $value >= 0} {
          if {$value == 0} {
            set weight_line     0
          } else {
            set weight_line     1
            set weight_line_num $value
          }
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-weight-toggle"} {
      incr i
      set value [lindex $contents $i]
      if {[string index $value 0] ne "-"} {
        if {[string is integer $value] && $value >= 0} {
          if {$value == 0} {
            set weight_toggle     0
          } else {
            set weight_toggle     1
            set weight_toggle_num $value
          }
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-weight-memory"} {
      incr i
      set value [lindex $contents $i]
      if {[string index $value 0] ne "-"} {
        if {[string is integer $value] && $value >= 0} {
          if {$value == 0} {
            set weight_memory     0
          } else {
            set weight_memory     1
            set weight_memory_num $value
          }
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-weight-comb"} {
      incr i
      set value [lindex $contents $i]
      if {[string index $value 0] ne "-"} {
        if {[string is integer $value] && $value >= 0} {
          if {$value == 0} {
            set weight_comb     0
          } else {
            set weight_comb     1
            set weight_comb_num $value
          }
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-weight-fsm"} {
      incr i
      set value [lindex $contents $i]
      if {[string index $value 0] ne "-"} {
        if {[string is integer $value] && $value >= 0} {
          if {$value == 0} {
            set weight_fsm     0
          } else {
            set weight_fsm     1
            set weight_fsm_num $value
          }
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-weight-assert"} {
      incr i
      set value [lindex $contents $i]
      if {[string index $value 0] ne "-"} {
        if {[string is integer $value] && $value >= 0} {
          if {$value == 0} {
            set weight_assert     0
          } else {
            set weight_assert     1
            set weight_assert_num $value
          }
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-f"} {
      incr i
      if {[string index $value 0] ne "-"} {
        if {[file isfile [lindex $contents $i]] == 1} {
          read_rank_option_file $w [lindex $contents $i]
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[file isfile [lindex $contents $i]] == 1} {
      .rankwin.p.files.f.t.lb insert end [list 0 [lindex $contents $i]]
      .rankwin.p.files.f.t.lb cellconfigure end,required -image $rank_img_unchecked

    }

  }

  close $fp

  # Update the CDD file number label
  handle_rank_cdds_num_files .rankwin.p.files

}

proc setup_cdd_rank_options {w} {

  global rankgen_fname rank_filename
  global weight_line weight_toggle weight_memory weight_comb weight_fsm weight_assert
  global weight_line_num weight_toggle_num weight_memory_num weight_comb_num weight_fsm_num weight_assert_num
  global names_only rank_verbose rank_req_check_cnt

  if {$rankgen_fname ne ""} {

    # Perform global variable initialization here
    set rank_filename      ""
    set weight_line        1
    set weight_toggle      1
    set weight_memory      1
    set weight_comb        1
    set weight_fsm         1
    set weight_assert      1
    set weight_line_num    1
    set weight_toggle_num  1
    set weight_memory_num  1
    set weight_comb_num    1
    set weight_fsm_num     1
    set weight_assert_num  1
    set names_only         0
    set rank_verbose       0
    set rank_req_check_cnt 0

    # Update the corresponding entry widget state to normal
    .rankwin.p.opts.wf.e_l configure -state normal
    .rankwin.p.opts.wf.e_t configure -state normal
    .rankwin.p.opts.wf.e_m configure -state normal
    .rankwin.p.opts.wf.e_c configure -state normal
    .rankwin.p.opts.wf.e_f configure -state normal
    .rankwin.p.opts.wf.e_a configure -state normal

    read_rank_option_file $w $rankgen_fname

  } else {

    # Otherwise, set global variables to desired default values
    set rank_filename      ""
    set weight_line        1
    set weight_toggle      1
    set weight_memory      1
    set weight_comb        1
    set weight_fsm         1
    set weight_assert      1
    set weight_line_num    1
    set weight_toggle_num  1
    set weight_memory_num  1
    set weight_comb_num    1
    set weight_fsm_num     1
    set weight_assert_num  1
    set names_only         0
    set rank_verbose       0
    set rank_req_check_cnt 0

    # Update the corresponding entry widget state to normal
    .rankwin.p.opts.wf.e_l configure -state normal
    .rankwin.p.opts.wf.e_t configure -state normal
    .rankwin.p.opts.wf.e_m configure -state normal
    .rankwin.p.opts.wf.e_c configure -state normal
    .rankwin.p.opts.wf.e_f configure -state normal
    .rankwin.p.opts.wf.e_a configure -state normal

  }

  # Update the weight widgets
  handle_weight_entry_state .rankwin.p.opts.wf.cb_l

}

proc create_rank_cmd_options {} {

  global rank_filename rank_rname
  global weight_line weight_toggle weight_memory weight_comb weight_fsm weight_assert
  global weight_line_num weight_toggle_num weight_memory_num weight_comb_num weight_fsm_num weight_assert_num
  global names_only rank_verbose

  set args {}

  if {$rank_filename ne ""} {
    lappend args "-o $rank_filename"
  }

  if {$rank_rname ne ""} {
    lappend args "-required-list $rank_rname"
  } else {
    foreach row [.rankwin.p.files.f.t.lb get 0 end] {
      if {[lindex $row 0] == 1} {
        lappend args "-required-cdd [lindex $row 1]"
      }
    }
  }

  if {$names_only == 1} {
    lappend args "-names-only"
  }

  if {$rank_verbose == 1} {
    lappend args "-v"
  }

  if {$weight_line == 0} {
    lappend args "-weight-line 0"
  } elseif {$weight_line_num > 1} {
    lappend args "-weight-line $weight_line_num"
  }
  if {$weight_toggle == 0} {
    lappend args "-weight-toggle 0"
  } elseif {$weight_toggle_num > 1} {
    lappend args "-weight-toggle $weight_toggle_num"
  }
  if {$weight_memory == 0} {
    lappend args "-weight-memory 0"
  } elseif {$weight_memory_num > 1} {
    lappend args "-weight-memory $weight_memory_num"
  }
  if {$weight_comb == 0} {
    lappend args "-weight-comb 0"
  } elseif {$weight_comb_num > 1} {
    lappend args "-weight-comb $weight_comb_num"
  }
  if {$weight_fsm == 0} {
    lappend args "-weight-fsm 0"
  } elseif {$weight_fsm_num > 1} {
    lappend args "-weight-fsm $weight_fsm_num"
  }
  if {$weight_assert == 0} {
    lappend args "-weight-assert 0"
  } elseif {$weight_assert_num > 1} {
    lappend args "-weight-assert $weight_assert_num"
  }

  foreach row [.rankwin.p.files.f.t.lb get 0 end] {
    if {[lindex $row 0] == 0} {
      lappend args [lindex $row 1]
    }
  }

  return $args
  
}

proc handle_rank_cdds_source {w {fname ""} {use_fname 0}} {

  global rankgen_sel rankgen_fname

  if {$use_fname == 0} {
    set fname $rankgen_fname
  }

  if {$rankgen_sel eq "options"} {
    $w.f.fc.e configure -state disabled
    $w.f.fc.b configure -state disabled
    $w.bf.next configure -state normal
  } else {
    $w.f.fc.e configure -state normal
    $w.f.fc.b configure -state normal
    if {$fname eq "" || [file isfile $fname] == 0} {
      $w.bf.next configure -state disabled
    } else {
      $w.bf.next configure -state normal
    }
  }

  return 1

}

proc create_rank_cdds_source {w} {

  global rankgen_sel rankgen_fname

  # Create the upper widget frame for this pane
  ttk::frame $w

  # Create upper widgets
  ttk::frame $w.f
  ttk::frame $w.f.fu
  ttk::frame $w.f.fc
  ttk::frame $w.f.fl
  ttk::radiobutton $w.f.fc.rb_opts -text "Create CDD ranking by interactively selecting options" -variable rankgen_sel -value "options" \
    -command "handle_rank_cdds_source $w"
  ttk::radiobutton $w.f.fc.rb_file -text "Create CDD ranking by using option file" -variable rankgen_sel -value "file" \
    -command "handle_rank_cdds_source $w"
  ttk::entry  $w.f.fc.e -state disabled -textvariable rankgen_fname -validate all -validatecommand "handle_rank_cdds_source $w %P 1"
  ttk::button $w.f.fc.b -text "Browse..." -state disabled -command {
    set fname [tk_getOpenFile -title "Select a Rank Command Option File" -parent .rankwin]
    if {$fname ne ""} {
      set rankgen_fname $fname
    }
  }
  grid columnconfigure $w.f.fc 1 -weight 1
  grid $w.f.fc.rb_opts -row 0 -column 0 -columnspan 3 -sticky news -pady 10
  grid $w.f.fc.rb_file -row 1 -column 0 -sticky news -pady 10
  grid $w.f.fc.e       -row 1 -column 1 -sticky news -pady 10
  grid $w.f.fc.b       -row 1 -column 2 -sticky news -pady 10

  pack $w.f.fu -fill both -expand 1
  pack $w.f.fc -fill x
  pack $w.f.fl -fill both -expand 1

  # Create button frame
  ttk::frame $w.bf
  help_button $w.bf.help chapter.gui.rank section.gui.rank.select
  ttk::button $w.bf.cancel -width 10 -text "Cancel" -command "destroy [winfo toplevel $w]"
  ttk::button $w.bf.next   -width 10 -text "Next" -command "
    setup_cdd_rank_options $w
    goto_next_pane $w
  "
  pack $w.bf.help   -side right -padx 4 -pady 4
  pack $w.bf.cancel -side right -padx 4 -pady 4
  pack $w.bf.next   -side right -padx 4 -pady 4

  # Pack top-level frames
  pack $w.f  -fill both -expand yes
  pack $w.bf -side bottom -fill x

  # Set the input focus on the first checkbutton
  focus $w.f.fc.rb_opts

  return $w

}

proc handle_weight_entry_state {w} {

  global weight_line weight_toggle weight_memory weight_comb weight_fsm weight_assert

  set weight_type [string index [winfo name $w] end]
  set weight 0

  if {$weight_type eq "l"} {
    set weight $weight_line
  } elseif {$weight_type eq "t"} {
    set weight $weight_toggle
  } elseif {$weight_type eq "m"} {
    set weight $weight_memory
  } elseif {$weight_type eq "c"} {
    set weight $weight_comb
  } elseif {$weight_type eq "f"} {
    set weight $weight_fsm
  } elseif {$weight_type eq "a"} {
    set weight $weight_assert
  }

  if {$weight == 0} {
    $w configure -state disabled
  } else {
    $w configure -state normal
  }

}

proc check_weight_value {num} {

  if {[string is integer $num] && $num > 0} {
    return 1
  } else {
    return 0
  }

}

proc handle_rank_cdds_filename {w} {

  global rank_filename

  if {$rank_filename eq ""} {
    $w.bf.next configure -state disabled
  } else {
    $w.bf.next configure -state normal
  }

  return 1

}

proc handle_rank_cdds_filename_browse {w} {

  global rank_filename

  set fname [tk_getSaveFile -initialfile $rank_filename -parent [winfo toplevel $w]]

  if {$fname ne ""} {
    set rank_filename $fname
  }

}

proc create_rank_cdds_options {w} {

  global rank_filename
  global weight_line weight_toggle weight_memory weight_comb weight_fsm weight_assert
  global weight_line_num weight_toggle_num weight_memory_num weight_comb_num weight_fsm_num weight_assert_num
  global names_only rank_verbose

  # Create top-most frame
  ttk::frame $w

  # Create output filename frame
  ttk::frame  $w.ff
  ttk::label  $w.ff.l -text "Output ranking report name:"
  ttk::entry  $w.ff.e -textvariable rank_filename -validate all
  ttk::button $w.ff.b -text "Browse..." -command "handle_rank_cdds_filename_browse $w"
  pack   $w.ff.l -side left  -padx 3 -pady 3
  pack   $w.ff.e -side left  -padx 3 -pady 3 -fill x -expand 1
  pack   $w.ff.b -side right -padx 3 -pady 3

  # Create weight selection frame
  ttk::labelframe $w.wf -text "Coverage metric weighting"
  ttk::checkbutton $w.wf.cb_l -text "Consider line coverage with weight:"                -variable weight_line   -command "handle_weight_entry_state $w.wf.e_l"
  ttk::checkbutton $w.wf.cb_t -text "Consider toggle coverage with weight:"              -variable weight_toggle -command "handle_weight_entry_state $w.wf.e_t"
  ttk::checkbutton $w.wf.cb_m -text "Consider memory coverage with weight:"              -variable weight_memory -command "handle_weight_entry_state $w.wf.e_m"
  ttk::checkbutton $w.wf.cb_c -text "Consider combinational logic coverage with weight:" -variable weight_comb   -command "handle_weight_entry_state $w.wf.e_c"
  ttk::checkbutton $w.wf.cb_f -text "Consider FSM coverage with weight:"                 -variable weight_fsm    -command "handle_weight_entry_state $w.wf.e_f"
  ttk::checkbutton $w.wf.cb_a -text "Consider assertion coverage with weight:"           -variable weight_assert -command "handle_weight_entry_state $w.wf.e_a"
  ttk::entry       $w.wf.e_l  -textvariable weight_line_num   -width 10 -state disabled -validate all -validatecommand { check_weight_value $weight_line_num }
  ttk::entry       $w.wf.e_t  -textvariable weight_toggle_num -width 10 -state disabled -validate all -validatecommand { check_weight_value $weight_toggle_num }
  ttk::entry       $w.wf.e_m  -textvariable weight_memory_num -width 10 -state disabled -validate all -validatecommand { check_weight_value $weight_memory_num }
  ttk::entry       $w.wf.e_c  -textvariable weight_comb_num   -width 10 -state disabled -validate all -validatecommand { check_weight_value $weight_comb_num }
  ttk::entry       $w.wf.e_f  -textvariable weight_fsm_num    -width 10 -state disabled -validate all -validatecommand { check_weight_value $weight_fsm_num }
  ttk::entry       $w.wf.e_a  -textvariable weight_assert_num -width 10 -state disabled -validate all -validatecommand { check_weight_value $weight_assert_num }

  grid $w.wf.cb_l -row 0 -column 0 -sticky news -padx 3 -pady 3
  grid $w.wf.e_l  -row 0 -column 1 -sticky news -padx 3 -pady 3
  grid $w.wf.cb_t -row 1 -column 0 -sticky news -padx 3 -pady 3
  grid $w.wf.e_t  -row 1 -column 1 -sticky news -padx 3 -pady 3
  grid $w.wf.cb_m -row 2 -column 0 -sticky news -padx 3 -pady 3
  grid $w.wf.e_m  -row 2 -column 1 -sticky news -padx 3 -pady 3
  grid $w.wf.cb_c -row 3 -column 0 -sticky news -padx 3 -pady 3
  grid $w.wf.e_c  -row 3 -column 1 -sticky news -padx 3 -pady 3
  grid $w.wf.cb_f -row 4 -column 0 -sticky news -padx 3 -pady 3
  grid $w.wf.e_f  -row 4 -column 1 -sticky news -padx 3 -pady 3
  grid $w.wf.cb_a -row 5 -column 0 -sticky news -padx 3 -pady 3
  grid $w.wf.e_a  -row 5 -column 1 -sticky news -padx 3 -pady 3

  # Create names-only frame
  ttk::frame $w.nof
  ttk::checkbutton $w.nof.cb -text "Generate ranking report displaying only the names of the CDD files in the order they should be run" -variable names_only
  pack $w.nof.cb -side left -fill x

  # Create verbose frame
  ttk::frame $w.verbose
  ttk::checkbutton $w.verbose.cb -text "Display verbose information when running rank command" -variable rank_verbose
  pack $w.verbose.cb -side left -fill x

  # Create button frame
  ttk::frame  $w.bf
  help_button $w.bf.help chapter.gui.rank section.gui.rank.options
  ttk::button $w.bf.cancel -width 10 -text "Cancel" -command "destroy [winfo toplevel $w]"
  ttk::button $w.bf.next   -width 10 -text "Next"   -command "goto_next_pane $w"
  ttk::button $w.bf.prev   -width 10 -text "Back"   -command "goto_prev_pane $w"
  pack   $w.bf.help   -side right -padx 4 -pady 4
  pack   $w.bf.cancel -side right -padx 4 -pady 4
  pack   $w.bf.next   -side right -padx 4 -pady 4
  pack   $w.bf.prev   -side left  -padx 4 -pady 4

  $w.ff.e configure -validatecommand "handle_rank_cdds_filename $w"

  # Pack top-level frames
  pack $w.ff      -padx 3 -pady 3 -fill x
  pack $w.wf      -padx 3 -pady 3 -fill x
  pack $w.nof     -padx 3 -pady 3 -fill x
  pack $w.verbose -padx 3 -pady 3 -fill x
  pack $w.bf      -side bottom -padx 3 -pady 3 -fill x

  # Set the input focus on the first window
  focus $w.ff.e

  return $w

}

proc handle_rank_cdds_num_files {w} {

  global rank_req_check_cnt

  $w.m.l configure -text [format "%10d CDD Files are currently included for ranking" [$w.f.t.lb size]]

  # If we have two or more CDD files, we can rank them
  if {[$w.f.t.lb size] > 1} {
    $w.bf.generate configure -state normal
  } else {
    $w.bf.generate configure -state disabled
  }

  # Set the state of the "save required" button
  if {$rank_req_check_cnt == 0} {
    $w.save.b2 configure -state disabled
  } elseif {$rank_req_check_cnt == 1} {
    $w.save.b2 configure -state normal
  }

}

proc handle_rank_cdds_add_req_cdd {w} {

  global rank_img_checked rank_req_check_cnt

  set fnames [tk_getOpenFile -parent [winfo toplevel $w] -multiple 1 -title "Select Required CDD"]

  foreach fname $fnames {
    set index [lsearch -index 1 [$w.f.t.lb get 0 end] $fname]
    if {$index == -1} {
      $w.f.t.lb insert end [list 1 $fname]
      $w.f.t.lb cellconfigure end,required -image $rank_img_checked
    } else {
      $w.f.t.lb cellconfigure $index,required -image $rank_img_checked
    }
    incr rank_req_check_cnt
  }

  handle_rank_cdds_num_files $w

}
  
proc handle_rank_cdds_add_req_list {w} {

  global rank_img_checked rank_req_check_cnt

  set fname [tk_getOpenFile -parent [winfo toplevel $w] -title "Select File Containing a List of Required CDD Files to Include"]

  if {[catch {set fp [open $fname "r"]}]} {
    tk_messageBox -message "File $fname Not Found!" -title "No File" -icon error
  }

  set contents [join [list [read $fp]]]

  foreach fname $contents {
    set index [lsearch -index 1 [$w.f.t.lb get 0 end] $fname]
    if {$index == -1} {
      $w.f.t.lb insert end [list 1 $fname]
      $w.f.t.lb cellconfigure end,required -image $rank_img_checked
    } else {
      $w.f.t.lb cellconfigure $index,required -image $rank_img_checked
    }
    incr rank_req_check_cnt
  }

  close $fp

  handle_rank_cdds_num_files $w

}

proc handle_rank_cdds_add_files {w} {

  global rank_img_unchecked

  set fnames [tk_getOpenFile -filetypes {{{Code Coverage Database} {.cdd}}} -parent [winfo toplevel $w] -multiple 1 -title "Select CDD File(s) to Rank"]

  foreach fname $fnames {
    if {[lsearch -index 1 [$w.f.t.lb get 0 end] $fname] == -1} {
      $w.f.t.lb insert end [list 0 $fname]
      $w.f.t.lb cellconfigure end,required -image $rank_img_unchecked
    }
  }

  handle_rank_cdds_num_files $w

}

proc handle_rank_cdds_add_dir {w} {

  global rank_img_unchecked

  set dname [tk_chooseDirectory -mustexist 1 -parent [winfo toplevel $w] -title "Select Directory Containing CDD File(s) to Rank"]

  if {$dname ne ""} {
    foreach fname [glob -directory $dname *.cdd] {
      if {[lsearch -index 1 [$w.f.t.lb get 0 end] $fname] == -1} {
        $w.f.t.lb insert end [list 0 $fname]
        $w.f.t.lb cellconfigure end,required -image $rank_img_unchecked
      }
    }
  }

  handle_rank_cdds_num_files $w

}

proc handle_rank_cdds_add_curr {w} {

  global cdd_files rank_img_unchecked

  foreach fname $cdd_files {
    if {[lsearch -index 1 [$w.f.t.lb get 0 end] $fname] == -1} {
      $w.f.t.lb insert end [list 0 $fname]
      $w.f.t.lb cellconfigure end,required -image $rank_img_unchecked
    }
  }

  handle_rank_cdds_num_files $w

}

proc handle_rank_cdds_delete_files {w} {

  $w.f.t.lb delete [$w.f.t.lb curselection]

  handle_rank_cdds_num_files $w

}

proc generate_rank_cdd_file {w} {

  global COVERED dump_vpi_none

  # Create the argument list
  set cmdline [join [create_rank_cmd_options]]

  # Advance to the output window
  goto_next_pane $w

  # Open and clear the output contained in the given textbox
  .rankwin.p.output.f.t configure -state normal
  .rankwin.p.output.f.t delete 1.0 end

  # Generate the specified CDD file, grabbing the output so that we can display it
  set fp [open "|$COVERED rank $cmdline" "r+"]

  while {[gets $fp line] >= 0} {
    if {[string range $line 0 23] eq "Dynamic memory allocated"} {
      .rankwin.p.output.view.b configure -state normal
    }
    .rankwin.p.output.f.t insert end "$line\n"
  }

  close $fp

  # Disable the output textbox state
  .rankwin.p.output.f.t configure -state disabled

}

proc empty_string {val} {

  return ""

}

proc rank_files_edit_end_cmd {tbl row col text} {

  global rank_img_checked rank_img_unchecked rank_req_check_cnt

  switch [$tbl columncget $col -name] {

    required {
      if {$text == 0} {
        $tbl cellconfigure $row,$col -image $rank_img_unchecked
        incr rank_req_check_cnt -1
        if {$rank_req_check_cnt == 0} {
          .rankwin.p.files.save.b2 configure -state disabled
        }
      } else {
        $tbl cellconfigure $row,$col -image $rank_img_checked
        incr rank_req_check_cnt
        .rankwin.p.files.save.b2 configure -state normal
      }
    }

  }

  return $text

}

proc save_required_cdds_to_file {w} {

  global rank_rname

  set rank_rname [tk_getSaveFile -title "Save Required CDD Files..." -initialfile $rank_rname -parent .rankwin]

  if {$rank_rname ne ""} {
    if {[catch {set fp [open $rank_rname "w"]}]} {
      tk_messageBox -message "File $rank_rname Not Writable!" -title "No File" -icon error
    }
    foreach row [$w.f.t.lb get 0 end] {
      if {[lindex $row 0] == 1} {
        puts $fp [lindex $row 1]
      }
    }
    close $fp
  }

}

proc create_rank_cdds_files {w} {

  global rank_view tablelistopts

  # Create top-most frame
  ttk::frame $w

  # Create filename frame
  ttk::frame     $w.f
  ttk::frame     $w.f.t
  tablelist::tablelist $w.f.t.lb -columns {0 "Required" center 0 "CDD Filename"} -selectmode extended \
    -xscrollcommand "$w.f.t.hb set" -yscrollcommand "$w.f.t.vb set" -stretch {1} -movablerows 1 \
    -editendcommand rank_files_edit_end_cmd
  foreach {key value} [array get tablelistopts] {
    $w.f.t.lb configure -$key $value
  }
  $w.f.t.lb columnconfigure 0 -name required -editable 1 -editwindow checkbutton -formatcommand empty_string
  ttk::scrollbar $w.f.t.hb -orient horizontal -command "$w.f.t.lb xview" -takefocus 0
  ttk::scrollbar $w.f.t.vb -command "$w.f.t.lb yview" -takefocus 0
  grid rowconfigure    $w.f.t 0 -weight 1
  grid columnconfigure $w.f.t 0 -weight 1
  grid $w.f.t.lb -row 0 -column 0 -sticky news
  grid $w.f.t.vb -row 0 -column 1 -sticky ns
  grid $w.f.t.hb -row 1 -column 0 -sticky ew

  # Create top button frame
  ttk::frame  $w.f.b
  ttk::button $w.f.b.addfile -text "Add CDD File(s)"             -command "handle_rank_cdds_add_files $w"
  ttk::button $w.f.b.adddir  -text "Add CDDs from Directory"     -command "handle_rank_cdds_add_dir $w"
  ttk::button $w.f.b.addcur  -text "Add Currently Opened"        -command "handle_rank_cdds_add_curr $w"     -state disabled
  ttk::button $w.f.b.addreq  -text "Add Required CDD File(s)"    -command "handle_rank_cdds_add_req_cdd $w"
  ttk::button $w.f.b.addreqs -text "Add Required CDDs from List" -command "handle_rank_cdds_add_req_list $w"
  ttk::button $w.f.b.delete  -text "Delete"                      -command "handle_rank_cdds_delete_files $w" -state disabled
  pack   $w.f.b.addfile -fill x -padx 3 -pady 3
  pack   $w.f.b.adddir  -fill x -padx 3 -pady 3
  pack   $w.f.b.addcur  -fill x -padx 3 -pady 3
  pack   $w.f.b.addreq  -fill x -padx 3 -pady 3
  pack   $w.f.b.addreqs -fill x -padx 3 -pady 3
  pack   $w.f.b.delete  -fill x -padx 3 -pady 3

  pack $w.f.t -side left  -fill both -expand 1
  pack $w.f.b -side right -fill y

  # Create miscellaneous frame
  ttk::frame $w.m
  ttk::label $w.m.l
  pack $w.m.l -side left -pady 4

  # Create save frame
  ttk::frame  $w.save
  ttk::button $w.save.b1 -text "Save Options to File..." -command {
    set rank_sname [tk_getSaveFile -title "Save Rank Command Options to File..." -initialfile $rank_sname -parent .rankwin]
    if {$rank_sname ne ""} {
      if {[catch {set fp [open $rank_sname "w"]}]} {
        tk_messageBox -message "File $rank_sname Not Writable!" -title "No File" -icon error
      }
      foreach line [create_rank_cmd_options] {
        puts $fp $line
      }
      close $fp
    }
  }
  ttk::button $w.save.b2 -text "Save Required CDDs to File..." -state disabled -command "save_required_cdds_to_file $w"
  pack $w.save.b1 -side left  -pady 4
  pack $w.save.b2 -side right -pady 4

  # Create button frame
  ttk::frame  $w.bf
  help_button $w.bf.help chapter.gui.rank section.gui.rank.files
  ttk::button $w.bf.cancel   -width 10 -text "Cancel"   -command "destroy [winfo toplevel $w]"
  ttk::button $w.bf.generate -width 10 -text "Generate" -state disabled -command "generate_rank_cdd_file $w"
  ttk::button $w.bf.prev     -width 10 -text "Back"     -command "goto_prev_pane $w"
  pack   $w.bf.help     -side right -padx 4 -pady 4
  pack   $w.bf.cancel   -side right -padx 4 -pady 4
  pack   $w.bf.generate -side right -padx 4 -pady 4
  pack   $w.bf.prev     -side left  -padx 4 -pady 4
  
  # Pack top-level frames
  pack $w.f    -padx 3 -pady 3 -fill both -expand yes
  pack $w.m    -padx 3 -pady 3 -fill x
  pack $w.save -padx 3 -pady 3 -fill x
  pack $w.bf   -side bottom -padx 3 -pady 3 -fill x

  bind $w.f.t.lb <<TablelistSelect>> "$w.f.b.delete configure -state normal"

  # Initialize the number of ranked CDD files
  handle_rank_cdds_num_files $w

  # Set the input focus on the first window
  focus $w.f.b.addfile

  return $w

}

proc create_rank_cdds_output {w} {

  global rank_filename

  # Create top-most frame
  ttk::frame $w

  # Create output textbox and associated scrollbars
  ttk::frame     $w.f
  text      $w.f.t -state disabled -xscrollcommand "$w.f.hb set" -yscrollcommand "$w.f.vb set" -wrap none
  ttk::scrollbar $w.f.vb -command "$w.f.t yview" -takefocus 0
  ttk::scrollbar $w.f.hb -orient horizontal -command "$w.f.t.xview" -takefocus 0
  grid rowconfigure    $w.f 0 -weight 1
  grid columnconfigure $w.f 0 -weight 1
  grid $w.f.t  -row 0 -column 0 -sticky news
  grid $w.f.vb -row 0 -column 1 -sticky ns
  grid $w.f.hb -row 1 -column 0 -sticky ew
  
  # Create viewer button
  ttk::frame  $w.view
  ttk::button $w.view.b -text "View the ranking report in the GUI" -state disabled -command { viewer_show rank {CDD Ranking Report} $rank_filename }
  pack   $w.view.b -pady 3
  
  # Create button bar
  ttk::frame  $w.bf
  help_button $w.bf.help chapter.gui.rank section.gui.rank.output
  ttk::button $w.bf.finish -width 10 -text "Finish" -command "destroy [winfo toplevel $w]"
  ttk::button $w.bf.prev   -width 10 -text "Back"   -command "goto_prev_pane $w"
  pack   $w.bf.help   -side right -padx 4 -pady 4
  pack   $w.bf.finish -side right -padx 4 -pady 4
  pack   $w.bf.prev   -side left  -padx 4 -pady 4

  # Pack frames
  pack $w.f    -fill both -expand 1
  pack $w.view -fill x
  pack $w.bf   -fill x -side bottom

  # Set the input focus on the finish button
  focus $w.bf.finish

  return $w

}
