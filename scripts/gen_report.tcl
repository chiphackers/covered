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

set rsel_wsel  0
set rsel_width "105"
set rsel_eid   0
set rsel_excl  0

proc create_report_generation_window {} {

  global rptgen_fname rptgen_sel

  if {[winfo exists .rselwin] == 0} {

    toplevel .rselwin
    wm title .rselwin "Create ASCII Report"

    # Create labelframe that will hold the contents
    panedwindow .rselwin.p

    # Initialize global values
    set rptgen_sel   "options"
    set rptgen_fname ""
    
    # Add panes
    .rselwin.p add [create_report_generation_source  .rselwin.p.rs] -width 600 -height 650
    .rselwin.p add [create_report_generation_options .rselwin.p.ro] -width 600 -height 650 -hide true

    # Pack the panedwindow
    pack .rselwin.p -fill both -expand yes
    
  }

  # Finally, raise this window
  raise .rselwin

}

proc read_report_option_file {fname} {
  
  global rsel_wsel rsel_width rsel_sdv rsel_cu rsel_mi rsel_sup rsel_bw
  global rsel_l rsel_t rsel_m rsel_c rsel_f rsel_a rsel_r
  global rsel_eid rsel_excl
  global rsel_fname rptgen_fname
    
  if {[catch {set fp [open $rptgen_fname "r"]}]} {
    tk_messageBox -message "File $rptgen_fname Not Found!" -title "No File" -icon error
  }
 
  set contents [join [list [read $fp]]]
 
  for {set i 0} {$i<[llength $contents]} {incr i} {
 
    if {[lindex $contents $i] eq "-d"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[string first [lindex $contents $i] "sdv"] != -1} {
          set rsel_sdv [lindex $contents $i]
        }
      } else {
        set i [expr $i - 1]
      }
      
    } elseif {[lindex $contents $i] eq "-w"} {
      set rsel_wsel 1
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[string is integer [lindex $contents $i]] == 1} {
          set rsel_width [lindex $contents $i]
        }
      } else {
        set i [expr $i - 1]
      }
      
    } elseif {[lindex $contents $i] eq "-m"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        foreach val [split [lindex $contents $i] ""] {
          if {$val eq "l"} {
            set rsel_l "l"
          } elseif {$val eq "t"} {
            set rsel_t "t"
          } elseif {$val eq "m"} {
            set rsel_m "m"
          } elseif {$val eq "c"} {
            set rsel_c "c"
          } elseif {$val eq "f"} {
            set rsel_f "f"
          } elseif {$val eq "a"} {
            set rsel_a "a"
          } elseif {$val eq "r"} {
            set rsel_r "r"
          }
        }
      } else {
        set i [expr $i - 1]
      }
      
    } elseif {[lindex $contents $i] eq "-o"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set rsel_fname [lindex $contents $i]
      } else {
        set i [expr $i - 1]
      }
      
    } elseif {[lindex $contents $i] eq "-s"} {
      set rsel_sup "-s"
      
    } elseif {[lindex $contents $i] eq "-b"} {
      set rsel_bw "-b"
      
    } elseif {[lindex $contents $i] eq "-i"} {
      set rsel_mi "-i"
      
    } elseif {[lindex $contents $i] eq "-c"} {
      set rsel_cu "-c"
      
    } elseif {[lindex $contents $i] eq "-x"} {
      set rsel_eid 1

    } elseif {[lindex $contents $i] eq "-e"} {
      set rsel_excl 1

    } elseif {[lindex $contents $i] eq "-f"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        read_report_option_file [lindex $contents $i]
      } else {
        set i [expr $i - 1]
      }
    }
  } 
 
  close $fp
  
}

proc create_report_cmd_options {} {
  
  global rsel_wsel rsel_width rsel_sdv rsel_mi rsel_cu rsel_sup rsel_bw
  global rsel_l rsel_t rsel_m rsel_c rsel_f rsel_a rsel_r
  global rsel_eid rsel_excl
  global rsel_fname cdd_name
  
  # Create command-line to report command of Covered
  if {$rsel_wsel == 0}      { set w    "" } else { set w    " -w $rsel_width" }
  if {$rsel_mi   == "None"} { set mi   "" } else { set mi   " $rsel_mi" }
  if {$rsel_cu   == "None"} { set cu   "" } else { set cu   " $rsel_cu" }
  if {$rsel_sup  == "None"} { set sup  "" } else { set sup  " $rsel_sup" }
  if {$rsel_bw   == "None"} { set bw   "" } else { set bw   " $rsel_bw" }
  if {$rsel_l    == "None"} { set l    "" } else { set l    $rsel_l }
  if {$rsel_t    == "None"} { set t    "" } else { set t    $rsel_t }
  if {$rsel_m    == "None"} { set m    "" } else { set m    $rsel_m }
  if {$rsel_c    == "None"} { set c    "" } else { set c    $rsel_c }
  if {$rsel_f    == "None"} { set f    "" } else { set f    $rsel_f }
  if {$rsel_a    == "None"} { set a    "" } else { set a    $rsel_a }
  if {$rsel_r    == "None"} { set r    "" } else { set r    $rsel_r }
  if {$rsel_eid  == 0}      { set eid  "" } else { set eid  " -x" }
  if {$rsel_excl == 0}      { set excl "" } else { set excl " -e" }
  
  set cmd "-d $rsel_sdv$mi$cu -m $l$t$m$c$f$a$r$w$sup$bw$eid$excl"
  
  return $cmd
  
}

proc create_report {w} {
  
  global rsel_view cdd_name rsel_fname
  
  eval "tcl_func_generate_report [create_report_cmd_options] -o $rsel_fname $cdd_name"
  
  destroy [winfo toplevel $w]
  
  if {$rsel_view == 1} {
    viewer_show rpt {ASCII Report} $rsel_fname
  }
  
}

proc setup_report_selection_options {} {

  global rsel_wsel rsel_width rsel_sdv rsel_cu rsel_mi rsel_sup rsel_bw
  global rsel_l rsel_t rsel_m rsel_c rsel_f rsel_a rsel_r
  global rsel_eid rsel_excl
  global rptgen_fname

  if {$rptgen_fname ne ""} {

    set rsel_wsel  0
    set rsel_width 105
    set rsel_sdv   s
    set rsel_cu    None
    set rsel_mi    None
    set rsel_sup   None
    set rsel_bw    None
    set rsel_l     None
    set rsel_t     None
    set rsel_m     None
    set rsel_c     None
    set rsel_f     None
    set rsel_a     None
    set rsel_r     None
    set rsel_eid   0
    set rsel_excl  0

    read_report_option_file $rptgen_fname

  } else {

    set rsel_wsel  0
    set rsel_width 105
    set rsel_sdv   s
    set rsel_cu    None
    set rsel_mi    None
    set rsel_sup   None
    set rsel_bw    None
    set rsel_l     l
    set rsel_t     t
    set rsel_m     m
    set rsel_c     c
    set rsel_f     f
    set rsel_a     None
    set rsel_r     None
    set rsel_eid   0
    set rsel_excl  0

  }

}

proc handle_report_generation_source {w {fname ""} {use_fname 0}} {

  global rptgen_sel rptgen_fname

  if {$use_fname == 0} {
    set fname $rptgen_fname
  }

  if {$rptgen_sel eq "options"} {
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

proc create_report_generation_source {w} {
 
  global rptgen_sel rptgen_fname
  
  # Create the upper widget frame for this pane
  ttk::frame $w
  
  # Create upper widgets
  ttk::frame $w.f
  ttk::frame $w.f.fu
  ttk::frame $w.f.fc
  ttk::frame $w.f.fl
  ttk::radiobutton $w.f.fc.rb_opts -text "Create report by interactively selecting options" -variable rptgen_sel -value "options" \
     -command "handle_report_generation_source $w"
  ttk::radiobutton $w.f.fc.rb_file -text "Create report by using option file" -variable rptgen_sel -value "file" \
     -command "handle_report_generation_source $w"
  ttk::entry  $w.f.fc.e -state disabled -textvariable rptgen_fname -validate all -validatecommand "handle_report_generation_source $w %P 1"
  ttk::button $w.f.fc.b -text "Browse..." -state disabled -command {
    set fname [tk_getOpenFile -title "Select a Report Command Option File" -parent .rselwin]
    if {$fname ne ""} {
      set rptgen_fname $fname
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
  help_button $w.bf.help chapter.gui.genreport section.gui.genreport.select
  ttk::button $w.bf.cancel -width 10 -text "Cancel" -command "destroy [winfo toplevel $w]"
  ttk::button $w.bf.next -width 10 -text "Next" -command "
    setup_report_selection_options
    goto_next_pane $w
  "
  pack $w.bf.help   -side right -padx 4 -pady 4
  pack $w.bf.cancel -side right -padx 4 -pady 4
  pack $w.bf.next   -side right -padx 4 -pady 4
  
  # Pack top-level frames
  pack $w.f  -fill both -expand yes
  pack $w.bf -fill x
  
  return $w
   
}

proc create_report_generation_options {w} {

  global rsel_wsel rsel_width rsel_sup rsel_cu rsel_mi rsel_sdv rsel_view rsel_bw
  global rsel_l rsel_t rsel_m rsel_c rsel_f rsel_a rsel_r
  global rsel_fname cdd_name rsel_sname

  # Create default report filename
  set rsel_fname "[file rootname $cdd_name].rpt"
  set rsel_sname ""

  ttk::frame $w

  ttk::frame      $w.f
  ttk::labelframe $w.f.misc -labelanchor nw -text "Set ASCII Report Generation Options"

  # Create width area
  ttk::checkbutton $w.f.misc.width_val -text "Limit line width to:" -variable rsel_wsel -command "
    if {$rsel_wsel == 0} {
      $w.f.misc.width_w configure -state disabled
    } else {
      $w.f.misc.width_w configure -state normal
    }
  "
  ttk::entry $w.f.misc.width_w -textvariable rsel_width -width 3 -validate key -validatecommand {string is int %P} -invalidcommand bell -state disabled
  ttk::label $w.f.misc.width_lbl -text "characters" -anchor w
  
  # Create empty module/instance suppression area
  ttk::checkbutton $w.f.misc.sup_val -text "Suppress modules/instances from output if they contain no coverage information" \
                                        -variable rsel_sup -onvalue "-s" -offvalue "None"

  # Create bitwise vector combinational logic output
  ttk::checkbutton $w.f.misc.bw_val -text "Output combinational logic vector operations in bitwise format" -variable rsel_bw -onvalue "-b" -offvalue "None"

  # Create checkbutton for showing exclusion IDs
  ttk::checkbutton $w.f.misc.eid_val -text "Show exclusion IDs in detailed/verbose output" -variable rsel_eid -onvalue 1 -offvalue 0

  grid $w.f.misc.width_val -row 0 -column 0 -sticky news -pady 4
  grid $w.f.misc.width_w   -row 0 -column 1 -sticky news -pady 4
  grid $w.f.misc.width_lbl -row 0 -column 2 -sticky news -pady 4
  grid $w.f.misc.sup_val   -row 1 -column 0 -columnspan 3 -sticky nw -pady 4
  grid $w.f.misc.bw_val    -row 2 -column 0 -columnspan 3 -sticky nw -pady 4
  grid $w.f.misc.eid_val   -row 3 -column 0 -columnspan 3 -sticky nw -pady 4

  # Create and pack detail selection area
  ttk::labelframe  $w.f.sdv -text "Level of Detail" -labelanchor nw
  ttk::radiobutton $w.f.sdv.s -text "Summary"  -variable rsel_sdv -value "s"
  ttk::radiobutton $w.f.sdv.d -text "Detailed" -variable rsel_sdv -value "d"
  ttk::radiobutton $w.f.sdv.v -text "Verbose"  -variable rsel_sdv -value "v"

  pack $w.f.sdv.s -anchor w
  pack $w.f.sdv.d -anchor w
  pack $w.f.sdv.v -anchor w

  # Create module/instance selection area
  ttk::labelframe  $w.f.mi -text "Accumulate By" -labelanchor nw
  ttk::radiobutton $w.f.mi.m -text "Module"   -variable rsel_mi -value "None"
  ttk::radiobutton $w.f.mi.i -text "Instance" -variable rsel_mi -value "-i"

  pack $w.f.mi.m -anchor w
  pack $w.f.mi.i -anchor w

  # Create metric selection area
  ttk::labelframe  $w.f.metric -text "Show Metrics" -labelanchor nw
  ttk::checkbutton $w.f.metric.l -text "Line"            -variable rsel_l -onvalue "l" -offvalue "None"
  ttk::checkbutton $w.f.metric.t -text "Toggle"          -variable rsel_t -onvalue "t" -offvalue "None"
  ttk::checkbutton $w.f.metric.m -text "Memory"          -variable rsel_m -onvalue "m" -offvalue "None"
  ttk::checkbutton $w.f.metric.c -text "Logic"           -variable rsel_c -onvalue "c" -offvalue "None"
  ttk::checkbutton $w.f.metric.f -text "FSM"             -variable rsel_f -onvalue "f" -offvalue "None"
  ttk::checkbutton $w.f.metric.a -text "Assertion"       -variable rsel_a -onvalue "a" -offvalue "None"
  ttk::checkbutton $w.f.metric.r -text "Race Conditions" -variable rsel_r -onvalue "r" -offvalue "None"

  pack $w.f.metric.l -anchor w
  pack $w.f.metric.t -anchor w
  pack $w.f.metric.m -anchor w
  pack $w.f.metric.c -anchor w
  pack $w.f.metric.f -anchor w
  pack $w.f.metric.a -anchor w
  pack $w.f.metric.r -anchor w

  # Create covered/uncovered selection area
  ttk::labelframe  $w.f.cue   -text "Coverage Type" -labelanchor nw
  ttk::radiobutton $w.f.cue.u -text "Uncovered" -variable rsel_cu -value "None"
  ttk::radiobutton $w.f.cue.c -text "Covered"   -variable rsel_cu -value "-c"
  ttk::checkbutton $w.f.cue.e -text "Excluded" -variable rsel_excl -onvalue 1 -offvalue 0

  pack $w.f.cue.u -anchor w
  pack $w.f.cue.c -anchor w
  pack $w.f.cue.e -anchor w

  # Now pack all of the labelframes
  grid columnconfigure $w.f 2 -weight 1
  grid $w.f.misc   -row 0 -column 0 -columnspan 3 -sticky news -pady 4 -padx 6
  grid $w.f.sdv    -row 1 -column 0               -sticky news -pady 4 -padx 6
  grid $w.f.metric -row 1 -column 2 -rowspan 3    -sticky news -pady 4 -padx 6
  grid $w.f.mi     -row 2 -column 0               -sticky news -pady 4 -padx 6
  grid $w.f.cue    -row 3 -column 0               -sticky news -pady 4 -padx 6
  
  # Create save frame
  ttk::frame $w.save
  ttk::button $w.save.b -text "Save Options to File..." -command {
    set rsel_sname [tk_getSaveFile -title "Save Report Options to File..." -initialfile $rsel_sname -parent .rselwin]
    if {$rsel_sname ne ""} {
      if {[catch {set fp [open $rsel_sname "w"]}]} {
        tk_messageBox -message "File $sname Not Writable!" -title "No File" -icon error
      }
      puts $fp "[create_report_cmd_options]\n"
      close $fp
    }
  }
  pack $w.save.b -pady 4

  # Create filename frame
  ttk::frame  $w.fname
  ttk::label  $w.fname.lbl -text "Output report to file:" -anchor e
  ttk::entry  $w.fname.e -textvariable rsel_fname
  ttk::button $w.fname.b -text "Browse..." -command {
    set rsel_fname [tk_getSaveFile -filetypes $rpt_file_types -initialfile $rsel_fname -title "Save Generated Report As" -parent .rselwin]
  }
  grid columnconfigure $w.fname 1 -weight 1
  grid $w.fname.lbl -row 0 -column 0 -sticky ew -pady 4
  grid $w.fname.e   -row 0 -column 1 -sticky ew -pady 4
  grid $w.fname.b   -row 0 -column 2 -sticky ew -pady 4

  # Allow the user to specify if they would like to view the report after it is generated
  ttk::frame $w.view
  ttk::checkbutton $w.view.cb -text "View the report in the GUI after it is created" -variable rsel_view
  pack $w.view.cb -side left -pady 4

  # Create button frame
  ttk::frame  $w.bf
  ttk::button $w.bf.back   -width 10 -text "Back" -command "goto_prev_pane $w"
  ttk::button $w.bf.create -width 10 -text "Create" -command "create_report $w"
  ttk::button $w.bf.cancel -width 10 -text "Cancel" -command "destroy [winfo toplevel $w]"
  help_button $w.bf.help chapter.gui.genreport section.gui.genreport.options
  pack $w.bf.help   -side right -padx 4 -pady 4
  pack $w.bf.cancel -side right -padx 4 -pady 4
  pack $w.bf.create -side right -padx 4 -pady 4
  pack $w.bf.back   -side left  -padx 4 -pady 4

  # Now pack all of the frames
  pack $w.f     -fill both -side top
  pack $w.save  -fill x
  pack $w.fname -fill x
  pack $w.view  -fill x
  pack $w.bf    -fill both -side bottom

  return $w

}

