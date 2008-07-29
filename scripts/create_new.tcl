set dump_filetypes {
  {{VCD Dumpfiles} {.vcd}}
  {{LXT Dumpfiles} {.lxt}}
  {{LXT2 Dumpfiles} {.lxt2}}
  {{All Files}     * }
}

proc check_to_generate {} {

  global dump_vpi_none toplevel_name dump_file

  if {$dump_vpi_none eq "none" && $toplevel_name ne "" && 
    .newwin.bf.gen configure -state normal
  } elseif {$dump_vpi_none eq "dump" && $dump_file ne "" &&
    .newwin.bf.gen configure -state normal
  } elseif {$dump_vpi_none eq "vpi" && $toplevel_name ne "" &&
    .newwin.bf.gen configure -state normal
  } else {
    .newwin.bf.gen configure -state disabled
  }

}

# proc create_new_cdd {} {

proc create_new_cdd {} {

  global dump_filetypes file_types
  global cdd_filename toplevel_name delay_type
  global race_cond_action race_cond_pragma race_cond_pragma_name
  global exclude_always exclude_assign exclude_initial exclude_final exclude_pragma exclude_pragma_name
  global assert_ovl design_generation
  global dump_vpi_none dump_file vpi_file vpi_ts vpi_ts_num1 vpi_ts_scale1 vpi_ts_num2 vpi_ts_scale2 simulator

  # Now create the window and raise it to the front
  if {[winfo exists .newwin] == 0} {

    # Create new window
    toplevel .newwin
    wm title .newwin "Create new CDD file"

    ############################################
    # Add widgets to general option item frame #
    ############################################

    labelframe .newwin.general -text "General CDD Options" -takefocus 0

    # Add the CDD filename widgets
    set cdd_filename ""
    frame  .newwin.general.cdd
    label  .newwin.general.cdd.l -text "CDD name:"
    entry  .newwin.general.cdd.e -textvariable cdd_filename -validate all -vcmd {
      if {[file isfile $cdd_filename]} {
        set_widget_state .newwin.parse    disabled
        set_widget_state .newwin.bot.opts disabled
        set dump_vpi_none "dump"
        .newwin.general.dump.rn configure -state disabled
        .newwin.general.dump.ed configure -state normal
        .newwin.general.dump.bd configure -state normal
      } else {
        set_widget_state .newwin.parse    normal
        set_widget_state .newwin.bot.opts normal
        .newwin.general.dump.rn configure -state normal
      }
      check_to_generate
      return 1
    }
    button .newwin.general.cdd.b -text "Browse" -width 10 -command {
      set tmp_cdd [tk_getOpenFile -title "Select CDD Filename" -filetypes $file_types]
      if {$tmp_cdd ne ""} {
        set cdd_filename $tmp_cdd
      }
    }
    bind .newwin.general.cdd.b <Return> {%W invoke}
    pack .newwin.general.cdd.l -side left  -padx 3 -pady 3 -fill y
    pack .newwin.general.cdd.e -side left  -padx 3 -pady 3 -fill x -expand 1
    pack .newwin.general.cdd.b -side right -padx 3 -pady 3 -fill y

    # Add dumpfile widgets
    set dump_vpi_none "none"
    set dump_file     ""
    set vpi_file      ""
    set vpi_ts        "0"
    set vpi_ts_num1   "1"
    set vpi_ts_scale1 "s"
    set vpi_ts_num2   "1"
    set vpi_ts_scale2 "s"
    set simulator     "Icarus Verilog"
    frame  .newwin.general.dump
    radiobutton .newwin.general.dump.rn -text "Parse Design Only" -variable dump_vpi_none -value "none" -anchor w
    radiobutton .newwin.general.dump.rd -text "Dumpfile:"         -variable dump_vpi_none -value "dump" -anchor w
    entry  .newwin.general.dump.ed -textvariable dump_file -state disabled
    button .newwin.general.dump.bd -text "Browse" -width 10 -state disabled -command {
      set dump_file [tk_getOpenFile -title "Select VCD/LXT Dumpfile" -filetypes $dump_filetypes]
    }
    bind .newwin.general.dump.bd <Return> {%W invoke}
    radiobutton   .newwin.general.dump.rv -text "VPI Module:" -variable dump_vpi_none -value "vpi" -anchor w
    entry         .newwin.general.dump.ev -textvariable vpi_file -state disabled
    tk_optionMenu .newwin.general.dump.mv simulator {Icarus Verilog} {Cver} {VCS}
    frame         .newwin.general.dump.ts
    checkbutton   .newwin.general.dump.ts.cb -text "Set VPI Module Timescale:" -anchor w -variable vpi_ts -state disabled
    tk_optionMenu .newwin.general.dump.ts.n1 vpi_ts_num1   {1} {10} {100}
    tk_optionMenu .newwin.general.dump.ts.s1 vpi_ts_scale1 {s} {ms} {us} {ns} {ps} {fs}
    label         .newwin.general.dump.ts.l  -text "/" -state disabled
    tk_optionMenu .newwin.general.dump.ts.n2 vpi_ts_num2   {1} {10} {100}
    tk_optionMenu .newwin.general.dump.ts.s2 vpi_ts_scale2 {s} {ms} {us} {ns} {ps} {fs}
    pack .newwin.general.dump.ts.cb -side left -padx 3 -pady 3
    pack .newwin.general.dump.ts.n1 -side left -padx 3 -pady 3
    pack .newwin.general.dump.ts.s1 -side left -padx 3 -pady 3
    pack .newwin.general.dump.ts.l  -side left -padx 3 -pady 3
    pack .newwin.general.dump.ts.n2 -side left -padx 3 -pady 3
    pack .newwin.general.dump.ts.s2 -side left -padx 3 -pady 3
    .newwin.general.dump.rn configure -command {
      if {$dump_vpi_none eq "none"} {
        .newwin.general.dump.ed configure -state disabled
        .newwin.general.dump.bd configure -state disabled
        .newwin.general.dump.ev configure -state disabled
        .newwin.general.dump.mv configure -state disabled
        set_widget_state .newwin.general.dump.ts disabled
      }
    }
    .newwin.general.dump.rd configure -command {
      if {$dump_vpi_none eq "dump"} {
        .newwin.general.dump.ed configure -state normal
        .newwin.general.dump.bd configure -state normal
        .newwin.general.dump.ev configure -state disabled
        .newwin.general.dump.mv configure -state disabled
        set_widget_state .newwin.general.dump.ts disabled
      }
    }
    .newwin.general.dump.rv configure -command {
      if {$dump_vpi_none eq "vpi"} {
        .newwin.general.dump.ed configure -state disabled
        .newwin.general.dump.bd configure -state disabled
        .newwin.general.dump.ev configure -state normal
        .newwin.general.dump.mv configure -state normal
        if {$vpi_ts == 1} {
          set_widget_state .newwin.general.dump.ts normal
        }
        .newwin.general.dump.ts.cb configure -state normal
      }
    }
    .newwin.general.dump.ts.cb configure -command {
      if {$vpi_ts == 0} {
        .newwin.general.dump.ts.n1 configure -state disabled
        .newwin.general.dump.ts.s1 configure -state disabled
        .newwin.general.dump.ts.l  configure -state disabled
        .newwin.general.dump.ts.n2 configure -state disabled
        .newwin.general.dump.ts.s2 configure -state disabled
      } else {
        .newwin.general.dump.ts.n1 configure -state normal
        .newwin.general.dump.ts.s1 configure -state normal
        .newwin.general.dump.ts.l  configure -state normal
        .newwin.general.dump.ts.n2 configure -state normal
        .newwin.general.dump.ts.s2 configure -state normal
      }
    }
    .newwin.general.dump.mv    configure -state disabled
    .newwin.general.dump.ts.n1 configure -state disabled
    .newwin.general.dump.ts.s1 configure -state disabled
    .newwin.general.dump.ts.n2 configure -state disabled
    .newwin.general.dump.ts.s2 configure -state disabled

    grid columnconfig .newwin.general.dump 1 -weight 1
    grid .newwin.general.dump.rn -row 0 -column 0 -sticky news -padx 3 -pady 3 -columnspan 2
    grid .newwin.general.dump.rd -row 1 -column 0 -sticky news -padx 3 -pady 3
    grid .newwin.general.dump.ed -row 1 -column 1 -sticky news -padx 3 -pady 3
    grid .newwin.general.dump.bd -row 1 -column 2 -sticky news -padx 3 -pady 3
    grid .newwin.general.dump.rv -row 2 -column 0 -sticky news -padx 3 -pady 3
    grid .newwin.general.dump.ev -row 2 -column 1 -sticky news -padx 3 -pady 3
    grid .newwin.general.dump.mv -row 2 -column 2 -sticky news -padx 3 -pady 3
    grid .newwin.general.dump.ts -row 3 -column 0 -sticky news -padx 3 -pady 3 -columnspan 3

    # Pack the general widgets
    pack .newwin.general.cdd  -fill x
    pack .newwin.general.dump -fill x

    ############################################
    # Add widgets to design parsing item frame #
    ############################################

    labelframe .newwin.parse -text "Design parsing fields" -takefocus 0

    # Add toplevel design name widgets
    set toplevel_name ""
    frame .newwin.parse.top
    label .newwin.parse.top.l -text "Toplevel module name:"
    entry .newwin.parse.top.e -textvariable toplevel_name
    pack  .newwin.parse.top.l -side left -padx 3 -pady 3
    pack  .newwin.parse.top.e -side left -padx 3 -pady 3 -fill x -expand 1

    # Add root pathname widgets
    set inst_name ""
    frame .newwin.parse.inst
    label .newwin.parse.inst.l -text "Root pathname:"
    entry .newwin.parse.inst.e -textvariable inst_name
    pack  .newwin.parse.inst.l -side left -padx 3 -pady 3
    pack  .newwin.parse.inst.e -side left -padx 3 -pady 3 -fill x -expand 1

    # Add delay specification
    set delay_type "None"
    frame .newwin.parse.delay
    label .newwin.parse.delay.l -text "Delay Type:"
    tk_optionMenu .newwin.parse.delay.m delay_type None Min Typ Max
    pack .newwin.parse.delay.l -side left -padx 3 -pady 3
    pack .newwin.parse.delay.m -side left -padx 3 -pady 3 -fill x

    # Add race condition option
    set race_cond_action      "None"
    set race_cond_pragma      0
    set race_cond_pragma_name "racecheck"
    frame         .newwin.parse.race
    label         .newwin.parse.race.l  -text "Race Condition Action:"
    tk_optionMenu .newwin.parse.race.m race_cond None Silent Warning Error Ignore
    checkbutton   .newwin.parse.race.cb -text "Use embedded race condition pragmas" -variable race_cond_pragma
    entry         .newwin.parse.race.e  -state disabled -textvariable race_cond_pragma_name
    .newwin.parse.race.cb configure -command {
      if {$race_cond_pragma == 0} {
        .newwin.parse.race.e configure -state disabled
      } else {
        .newwin.parse.race.e configure -state normal
      }
    }
    pack .newwin.parse.race.l  -side left -padx 3 -pady 3
    pack .newwin.parse.race.m  -side left -padx 3 -pady 3
    pack .newwin.parse.race.cb -side left -padx 3 -pady 3
    pack .newwin.parse.race.e  -side left -padx 3 -pady 3 -fill x -expand 1

    # Add assertion options
    set assert_ovl 0
    frame .newwin.parse.assert
    checkbutton .newwin.parse.assert.ovl -text "Include OVL Assertions" -variable assert_ovl
    pack .newwin.parse.assert.ovl -side left -padx 3 -pady 3

    # Add generation options
    set design_generation "System Verilog"
    frame .newwin.parse.gen
    label .newwin.parse.gen.l -text "Universal Verilog Generation:"
    tk_optionMenu .newwin.parse.gen.m design_generation {Verilog 1995} {Verilog 2001} {System Verilog}
    pack .newwin.parse.gen.l -side left -padx 3 -pady 3
    pack .newwin.parse.gen.m -side left -padx 3 -pady 3

    # Add exclusion options
    set exclude_always      0
    set exclude_assign      0
    set exclude_initial     0
    set exclude_final       0
    set exclude_pragma      0
    set exclude_pragma_name "coverage"
    frame       .newwin.parse.exclude
    checkbutton .newwin.parse.exclude.ea  -text "Exclude Always Blocks"  -variable exclude_always
    checkbutton .newwin.parse.exclude.ec  -text "Exclude Assign Blocks"  -variable exclude_assign
    checkbutton .newwin.parse.exclude.ei  -text "Exclude Initial Blocks" -variable exclude_initial
    checkbutton .newwin.parse.exclude.ef  -text "Exclude Final Blocks"   -variable exclude_final
    checkbutton .newwin.parse.exclude.epc -text "Exclude Within Pragma:" -variable exclude_pragma
    entry       .newwin.parse.exclude.epe -state disabled -textvariable exclude_pragma_name
    .newwin.parse.exclude.epc configure -command {
      if {$exclude_pragma == 0} {
        .newwin.parse.exclude.epe configure -state disabled
      } else {
        .newwin.parse.exclude.epe configure -state normal
      }
    }
    pack .newwin.parse.exclude.ea  -side left -padx 3 -pady 3
    pack .newwin.parse.exclude.ec  -side left -padx 3 -pady 3
    pack .newwin.parse.exclude.ei  -side left -padx 3 -pady 3
    pack .newwin.parse.exclude.ef  -side left -padx 3 -pady 3
    pack .newwin.parse.exclude.epc -side left -padx 3 -pady 3
    pack .newwin.parse.exclude.epe -side left -padx 3 -pady 3 -fill x -expand 1

    # Pack the general options frame
    grid columnconfigure .newwin.parse 0 -weight 1
    grid columnconfigure .newwin.parse 1 -weight 1
    grid .newwin.parse.top     -row 0 -column 0 -sticky news -padx 3 -pady 3
    grid .newwin.parse.inst    -row 0 -column 1 -sticky news -padx 3 -pady 3
    grid .newwin.parse.delay   -row 1 -column 0 -sticky news -padx 3 -pady 3
    grid .newwin.parse.race    -row 1 -column 1 -sticky news -padx 3 -pady 3
    grid .newwin.parse.assert  -row 2 -column 0 -sticky news -padx 3 -pady 3
    grid .newwin.parse.gen     -row 2 -column 1 -sticky news -padx 3 -pady 3
    grid .newwin.parse.exclude -row 3 -column 0 -sticky news -padx 3 -pady 3 -columnspan 2

    ############################################################
    # Create paned window for verilog paths and console output #
    ############################################################

    panedwindow .newwin.bot -sashrelief raised -sashwidth 4 -orient vertical

    #####################################
    # Add widgets to verilog path frame #
    #####################################

    labelframe .newwin.bot.opts -text "Command-line Options" -takefocus 0

    # Create and pack the listbox frame
    frame     .newwin.bot.opts.lbf
    tablelist::tablelist .newwin.bot.opts.lbf.lb -exportselection 0 -movablerows 1 -selectmode single -columns {0 "Type" 0 "Argument"} \
      -labelcommand tablelist::sortByColumn -xscrollcommand {.newwin.bot.opts.lbf.hb set} -yscrollcommand {.newwin.bot.opts.lbf.vf.vb set} -stretch all
    frame     .newwin.bot.opts.lbf.vf
    label     .newwin.bot.opts.lbf.vf.l
    scrollbar .newwin.bot.opts.lbf.vf.vb -command ".newwin.bot.opts.lbf.lb yview" -takefocus 0
    pack .newwin.bot.opts.lbf.vf.l  -fill x
    pack .newwin.bot.opts.lbf.vf.vb -fill y -expand 1
    scrollbar .newwin.bot.opts.lbf.hb -orient horizontal -command ".newwin.bot.opts.lbf.lb xview" -takefocus 0

    grid rowconfigure    .newwin.bot.opts.lbf 0 -weight 1
    grid columnconfigure .newwin.bot.opts.lbf 0 -weight 1
    grid .newwin.bot.opts.lbf.lb -row 0 -column 0 -sticky news
    grid .newwin.bot.opts.lbf.vf -row 0 -column 1 -sticky ns
    grid .newwin.bot.opts.lbf.hb -row 1 -column 0 -sticky ew

    # Create and pack the button frame
    frame       .newwin.bot.opts.bf
    menubutton  .newwin.bot.opts.bf.ins_mb -text "Insert" -relief raised -highlightthickness 1 -takefocus 1
    set m [menu .newwin.bot.opts.bf.ins_mb.m -tearoff false -takefocus 1]
    .newwin.bot.opts.bf.ins_mb configure -menu $m
    bind .newwin.bot.opts.bf.ins_mb <Return> {
      .newwin.bot.opts.bf.ins_mb.m post [winfo rootx .newwin.bot.opts.bf.ins_mb] [winfo rooty .newwin.bot.opts.bf.ins_mb]
      .newwin.bot.opts.bf.ins_mb.m activate 0
    }
    bind .newwin.bot.opts.bf.ins_mb <FocusOut> {.newwin.bot.opts.bf.ins_mb.m unpost}

    # Add the menu items
    $m add command -label "Source File..." -command {
      set value [tk_getOpenFile -title "Select Verilog Source File"]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Source File" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Source File" $value]
        }
      }
    }
    $m add command -label "Library Directory..." -command {
      set value [tk_chooseDirectory -title "Select Verilog Library Directory" -mustexist true]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Library Directory" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Library Directory" $value]
        }
      }
    }
    $m add command -label "Library Extension(s)..." -command {
      set value [get_library_extensions ""]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Library Extension" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Library Extension" $value]
        }
      }
    }
    $m add command -label "Include Directory..." -command {
      set value [tk_chooseDirectory -title "Select Include Directory" -mustexist true]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Include Directory" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Include Directory" $value]
        }
      }
    }
    $m add command -label "Define..." -command {
      set value [get_define "" ""]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Define" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Define" $value]
        }
      }
    }
    $m add command -label "Parameter Override..." -command {
      set value [get_parameter_override "" ""]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Param Override" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Param Override" $value]
        }
      }
    }
    $m add separator
    $m add command -label "FSM..." -command {
      set value [get_fsm "" "" ""]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "FSM" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "FSM" $value]
        }
      }
    }
    $m add command -label "Module Generation..." -command {
      set value [get_module_generation "" ""]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Module Generation" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Module Generation" $value]
        }
      }
    }
    $m add command -label "Module Exclusion..." -command {
      set value [get_module_exclusion ""]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Module Exclusion" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Module Exclusion" $value]
        }
      }
    }
    $m add separator
    $m add command -label "Command File..." -command {
      set value [tk_getOpenFile -title "Select Command File"]
      if {$value ne ""} {
        set index [.newwin.bot.opts.lbf.lb curselection]
        if {$index eq ""} {
          .newwin.bot.opts.lbf.lb insert end [list "Command File" $value]
        } else {
          .newwin.bot.opts.lbf.lb insert $index [list "Command File" $value]
        }
      }
    }

    button .newwin.bot.opts.bf.edit -text "Edit..." -width 10 -state disabled -command {
      set index  [.newwin.bot.opts.lbf.lb curselection]
      set values [.newwin.bot.opts.lbf.lb get $index]
      set type   [lindex $values 0]
      set value  [lindex $values 1]
      if {$type eq "Source File"} {
        set value [tk_getOpenFile -title "Select Verilog Source File" -initialfile $value]
      } elseif {$type eq "Library Directory"} {
        set value [tk_chooseDirectory -title "Select Verilog Library Directory" -mustexist true -initialdir $value]
      } elseif {$type eq "Library Extension"} {
        set value [get_library_extensions [lrange [split $value "+"] 1 end-1]]
      } elseif {$type eq "Include Directory"} {
        set value [tk_chooseDirectory -title "Select Include Directory" -mustexist true -initialdir $value]
      } elseif {$type eq "Command File"} {
        set value [tk_getOpenFile -title "Select Command File" -initialfile $value]
      } elseif {$type eq "Module Exclusion"} {
        set value [get_module_exclusion $value]
      } elseif {$type eq "Module Generation"} {
        set old_value [split $value "="]
        set value [get_module_generation [lindex $old_value 0] [lindex $old_value 1]]
      } elseif {$type eq "Define"} {
        set old_value [split $value "="]
        set value [get_define [lindex $old_value 0] [lindex $old_value 1]]
      } elseif {$type eq "Param Override"} {
        set old_value [split $value "="]
        set value [get_parameter_override [lindex $old_value 0] [lindex $old_value 1]]
      } elseif {$type eq "FSM"} {
        set old_value1 [split $value "="]
        set old_value2 [split [lindex $old_value1 1] ","]
        set value [get_fsm [lindex $old_value1 0] [lindex $old_value2 0] [lindex $old_value2 1]]
      }
      set index  [.newwin.bot.opts.lbf.lb curselection]
      if {$value ne ""} {
        .newwin.bot.opts.lbf.lb delete $index
        .newwin.bot.opts.lbf.lb insert $index [list $type $value]
        .newwin.bot.opts.lbf.lb selection set $index
      }
    }
    bind .newwin.bot.opts.bf.edit <Return> {%W invoke}
    button .newwin.bot.opts.bf.delete -text "Delete" -width 10 -state disabled -command {
      .newwin.bot.opts.lbf.lb delete [.newwin.bot.opts.lbf.lb curselection]
    }
    bind .newwin.bot.opts.bf.delete <Return> {%W invoke}
    bind .newwin.bot.opts.lbf.lb <<ListboxSelect>> {
      if {[.newwin.bot.opts.lbf.lb curselection] ne ""} {
        .newwin.bot.opts.bf.edit   configure -state normal
        .newwin.bot.opts.bf.delete configure -state normal
      }
    }
    pack .newwin.bot.opts.bf.ins_mb -fill x -padx 3 -pady 3
    pack .newwin.bot.opts.bf.edit   -fill x -padx 3 -pady 3
    pack .newwin.bot.opts.bf.delete -fill x -padx 3 -pady 3

    # Now pack the buttons and listbox frames in the labelframe
    pack .newwin.bot.opts.lbf -side left  -fill both -expand 1
    pack .newwin.bot.opts.bf  -side right -fill y

    ################################
    # Add widgets to console frame #
    ################################

    labelframe .newwin.bot.console -text "Console Output" -takefocus 0
    frame      .newwin.bot.console.f
    text       .newwin.bot.console.f.t -state disabled -xscrollcommand {.newwin.bot.console.f.hb set} -yscrollcommand {.newwin.bot.console.f.vb set} -takefocus 0
    scrollbar  .newwin.bot.console.f.vb -command {.newwin.bot.console.f.t yview} -takefocus 0
    scrollbar  .newwin.bot.console.f.hb -orient horizontal -command {.newwin.bot.console.f.t xview} -takefocus 0

    grid rowconfigure    .newwin.bot.console.f 0 -weight 1
    grid columnconfigure .newwin.bot.console.f 0 -weight 1
    grid .newwin.bot.console.f.t  -row 0 -column 0 -sticky news
    grid .newwin.bot.console.f.vb -row 0 -column 1 -sticky ns
    grid .newwin.bot.console.f.hb -row 1 -column 0 -sticky ew

    frame  .newwin.bot.console.bf
    button .newwin.bot.console.bf.clear -text "Clear" -width 7 -command {
      .newwin.bot.console.f.t configure -state normal
      .newwin.bot.console.f.t delete 1.0 end
      .newwin.bot.console.f.t configure -state disabled
    }
    button .newwin.bot.console.bf.save -text "Save..." -width 7 -state disabled -command {
      puts "NEED TO save console output to a file"
    }
    pack .newwin.bot.console.bf.clear -side right -padx 8 -pady 4
    pack .newwin.bot.console.bf.save  -side right -padx 8 -pady 4

    # Pack the console frame
    pack .newwin.bot.console.bf -fill x
    pack .newwin.bot.console.f  -fill both -expand 1

    # Pack the panedwindow
    .newwin.bot add .newwin.bot.opts
    .newwin.bot add .newwin.bot.console

    #####################################
    # Add widgets to lower button frame #
    #####################################

    frame .newwin.bf
    button .newwin.bf.gen -text "Generate" -width 10 -state disabled -command {
      puts "Generating CDD file"
    }
    bind .newwin.bf.gen <Return> {%W invoke}
    button .newwin.bf.help -text "Help" -width 10 -command {
      puts "Help"
    }
    bind .newwin.bf.help <Return> {%W invoke}
    button .newwin.bf.cancel -text "Cancel" -width 10 -command {
      destroy .newwin
    }
    bind .newwin.bf.cancel <Return> {%W invoke}
    pack .newwin.bf.help   -side right -pady 3 -padx 3
    pack .newwin.bf.cancel -side right -pady 3 -padx 3
    pack .newwin.bf.gen    -side right -pady 3 -padx 3

    # Pack the main frames
    pack .newwin.general -side top    -fill x
    pack .newwin.parse   -side top    -fill x
    pack .newwin.bot     -side top    -fill both -expand 1
    pack .newwin.bf      -side bottom -fill x

    # Cause this window to stay on top of main window
    wm transient .newwin .

  }

  # Set beginning focus
  focus .newwin.general.cdd.e

}

proc set_widget_state {name state} {

  global new_widget_state

  set children [winfo children $name]

  foreach child $children {
    set_widget_state $child $state
  }

  # If this widget has a -state option, set it to the given value
  if {[lsearch -exact [join [$name configure]] {-state}] != -1} {
    if {$state eq "disabled"} {
      if {[$name cget -state] ne "disabled"} {
        set new_widget_state($name) [$name cget -state]
      }
    } else {
      if {[info exists new_widget_state($name)]} {
        set state $new_widget_state($name)
      } else {
        set new_widget_state($name) [$name cget -state]
      }
    }
    if {$state ne ""} {
      $name configure -state $state
    }
  }

}

proc get_module_generation {modname gen} {

  global mod_gen_retval mod_gen_modname

  set mod_gen_retval  ""
  set mod_gen_modname $modname

  # Create new window
  toplevel .mgenwin
  wm title .mgenwin "Specify generation for module"

  # Add selection widgets
  frame .mgenwin.f -relief raised
  label .mgenwin.f.l -text "Module name:"
  entry .mgenwin.f.e -textvariable mod_gen_modname -validate all
  tk_optionMenu .mgenwin.f.m modgen "Verilog 1995" "Verilog 2001" "System Verilog"
  pack .mgenwin.f.l -side left  -padx 8 -pady 4 -fill x
  pack .mgenwin.f.e -side left  -padx 8 -pady 4 -fill x -expand 1
  pack .mgenwin.f.m -side right -padx 8 -pady 4 -fill x

  # Add button frame widgets
  frame  .mgenwin.bf -relief raised
  button .mgenwin.bf.ok -text "OK" -width 10 -state disabled -command {
    if {$modgen eq "Verilog 1995"} {
      set mod_gen_retval "$mod_gen_modname=1"
    } elseif {$modgen eq "Verilog 2001"} {
      set mod_gen_retval "$mod_gen_modname=2"
    } elseif {$modgen eq "System Verilog"} {
      set mod_gen_retval "$mod_gen_modname=3"
    }
    destroy .mgenwin
  }
  bind .mgenwin.bf.ok <Return> {%W invoke}
  button .mgenwin.bf.cancel -text "Cancel" -width 10 -command {
    set mod_gen_retval ""
    destroy .mgenwin
  }
  bind .mgenwin.bf.cancel <Return> {%W invoke}
  pack .mgenwin.bf.cancel -side right -padx 8 -pady 4
  pack .mgenwin.bf.ok     -side right -padx 8 -pady 4

  # Add functionality for entry
  .mgenwin.f.e configure -vcmd {
    if {$mod_gen_modname ne ""} {
      .mgenwin.bf.ok configure -state normal
    } else {
      .mgenwin.bf.ok configure -state disabled
    }
    return 1
  }

  # Pack the window
  pack .mgenwin.f  -fill x
  pack .mgenwin.bf -fill x

  # Make sure that this window is a transient window and set focus
  wm transient .mgenwin .newwin
  focus .mgenwin.f.e

  # Wait for the module generation window to be destroyed before returning
  tkwait window .mgenwin

  return $mod_gen_retval

}

proc get_module_exclusion {modname} {

  global mod_excl_retval mod_excl_modname

  set mod_excl_retval  ""
  set mod_excl_modname $modname

  # Create new window
  toplevel .mexclwin
  wm title .mexclwin "Specify module to exclude"

  # Add selection widgets
  frame .mexclwin.f -relief raised
  label .mexclwin.f.l -text "Module name:"
  entry .mexclwin.f.e -textvariable mod_excl_modname -validate all
  pack .mexclwin.f.l -side left -padx 8 -pady 4 -fill x
  pack .mexclwin.f.e -side left -padx 8 -pady 4 -fill x -expand 1

  # Add button frame widgets
  frame .mexclwin.bf -relief raised
  button .mexclwin.bf.ok -text "OK" -width 10 -state disabled -command {
    set mod_excl_retval "$mod_excl_modname"
    destroy .mexclwin
  }
  bind .mexclwin.bf.ok <Return> {%W invoke}
  button .mexclwin.bf.cancel -text "Cancel" -width 10 -command {
    set mod_excl_retval ""
    destroy .mexclwin
  }
  bind .mexclwin.bf.cancel <Return> {%W invoke}
  pack .mexclwin.bf.cancel -side right -padx 8 -pady 4
  pack .mexclwin.bf.ok     -side right -padx 8 -pady 4

  # Provide functionality for the entry
  .mexclwin.f.e configure -vcmd {
    if {$mod_excl_modname ne ""} {
      .mexclwin.bf.ok configure -state normal
    } else {
      .mexclwin.bf.ok configure -state disabled
    }
    return 1
  }

  # Pack the window
  pack .mexclwin.f  -fill x
  pack .mexclwin.bf -fill x

  # Make sure that this window is transient and sets focus to first widget
  wm transient .mexclwin .newwin
  focus .mexclwin.f.e

  # Wait for the module exclusion window to be destroyed before returning
  tkwait window .mexclwin

  return $mod_excl_retval

}

proc get_library_extensions {extensions} {

  global lib_ext_retval
  global lib_ext_name

  set lib_ext_retval ""
  set lib_ext_name   ""

  # Create new window
  toplevel .lextwin
  wm title .lextwin "Specify library extensions"

  # Add selection widgets
  frame     .lextwin.f
  frame     .lextwin.f.ef
  entry     .lextwin.f.ef.e -validate all -textvariable lib_ext_name
  button    .lextwin.f.ef.b -text "Update" -width 10 -state disabled
  pack .lextwin.f.ef.e -side left  -fill x -expand 1 -padx 3 -pady 3
  pack .lextwin.f.ef.b -side right -padx 3 -pady 3
  frame     .lextwin.f.lf
  listbox   .lextwin.f.lf.lb -xscrollcommand {.lextwin.f.lf.hb set} -yscrollcommand {.lextwin.f.lf.vb set} -exportselection 0 -selectmode single
  scrollbar .lextwin.f.lf.hb -orient horizontal -command {.lextwin.f.lf.lb xset}
  scrollbar .lextwin.f.lf.vb -command {.lextwin.f.lf.lb yset}
  grid rowconfigure    .lextwin.f.lf 0 -weight 1
  grid columnconfigure .lextwin.f.lf 0 -weight 1
  grid .lextwin.f.lf.lb -row 0 -column 0 -sticky news -padx 3 -pady 3
  grid .lextwin.f.lf.vb -row 0 -column 1 -sticky ns   -padx 3 -pady 3
  grid .lextwin.f.lf.hb -row 1 -column 0 -sticky ew   -padx 3 -pady 3
  pack .lextwin.f.ef -fill x    -expand 1 -padx 3 -pady 3
  pack .lextwin.f.lf -fill both -expand 1 -padx 3 -pady 3

  # Update the listbox with any previous values
  .lextwin.f.lf.lb insert end "Click to Add New Extension"
  foreach extension $extensions {
    if {$extension eq ""} {
      .lextwin.f.lf.lb insert end "<None>"
    } else {
      .lextwin.f.lf.lb insert end $extension
    }
  }
  .lextwin.f.lf.lb selection set 0

  # Create bindings
  bind .lextwin.f.lf.lb <<ListboxSelect>> {
    set index [.lextwin.f.lf.lb curselection]
    if {$index ne ""} {
      if {[.lextwin.f.lf.lb get $index] eq "Click to Add New Extension"} {
        set lib_ext_name ""
      } elseif {[.lextwin.f.lf.lb get $index] eq "<None>"} {
        set lib_ext_name ""
        .lextwin.f.lf.lb delete $index
      } else {
        set lib_ext_name [.lextwin.f.lf.lb get $index]
        .lextwin.f.lf.lb delete $index
      }
      .lextwin.f.lf.lb selection set 0
      .lextwin.f.ef.e  selection range 0 end
      .lextwin.f.ef.e  icursor end
    }
  }
  .lextwin.f.ef.e configure -vcmd {
    # See if the current name exists in the listbox
    if {[llength %P] == 0} {
      set curr_val "<None>"
    } else {
      set curr_val "%P"
    }
    for {set i 1} {$i < [.lextwin.f.lf.lb size]} {incr i} {
      if {[.lextwin.f.lf.lb get $i] eq $curr_val} {
        break
      }
    }
    if {$i == [.lextwin.f.lf.lb size]} {
      if {[llength %P] == 0} {
        .lextwin.f.ef.b configure -state normal
      } elseif {[string index "%P" 0] eq "."} {
        .lextwin.f.ef.b configure -state normal
      } else {
        .lextwin.f.ef.b configure -state disabled
      }
    } else {
      .lextwin.f.ef.b configure -state disabled
    }
    focus .lextwin.f.ef.e
    return 1
  }
  .lextwin.f.ef.b configure -command {
    if {$lib_ext_name eq ""} {
      .lextwin.f.lf.lb insert end "<None>"
    } else {
      .lextwin.f.lf.lb insert end $lib_ext_name
    }
    .lextwin.f.lf.lb selection set 0
    set lib_ext_name ""
  }
  bind .lextwin.f.ef.e <Return> {.lextwin.f.ef.b invoke}

  # Add button frame and buttons
  frame  .lextwin.bf
  button .lextwin.bf.ok -text "OK" -width 10 -command {
    set lib_ext_retval "+"
    for {set i 1} {$i < [.lextwin.f.lf.lb size]} {incr i} {
      if {[.lextwin.f.lf.lb get $i] eq "<None>"} {
        set lib_ext_retval "$lib_ext_retval+"
      } else {
        set lib_ext_retval "$lib_ext_retval[.lextwin.f.lf.lb get $i]+"
      }
    }
    destroy .lextwin
  }
  button .lextwin.bf.cancel -text "Cancel" -width 10 -command {
    set lib_ext_retval ""
    destroy .lextwin
  }
  pack .lextwin.bf.cancel -side right -padx 3 -pady 3
  pack .lextwin.bf.ok     -side right -padx 3 -pady 3

  # Pack the window
  pack .lextwin.f  -fill both -expand 1
  pack .lextwin.bf -fill x

  # Make sure that this window is transient and sets focus to first widget
  wm transient .lextwin .newwin
  focus .lextwin.f.ef.e

  # Wait for the module exclusion window to be destroyed before returning
  tkwait window .lextwin

  return $lib_ext_retval

}

proc define_update_state {} {

  global define_name define_value

  if {$define_name ne ""} {
    if {[.defwin.f.b cget -relief] eq "raised"} {
      .defwin.bf.ok configure -state normal
    } elseif {$define_value ne ""} {
      .defwin.bf.ok configure -state normal
    } else {
      .defwin.bf.ok configure -state disabled
    }
  } else {
    .defwin.bf.ok configure -state disabled
  }

  return 1

}

proc get_define {defname value} {

  global def_retval define_name define_value

  set def_retval   ""
  set define_name  $defname
  set define_value $value

  # Create new window
  toplevel .defwin
  wm title .defwin "Specify a command-line define value"

  # Add selection widgets
  frame .defwin.f
  label .defwin.f.l -text "Define name:"
  entry .defwin.f.e -textvariable define_name -validate all
  entry .defwin.f.v -state disabled -textvariable define_value -validate all
  button .defwin.f.b -text "=" -width 1 -relief raised -command {
    if {[.defwin.f.b cget -relief] eq "raised"} {
      .defwin.f.v configure -state normal
      .defwin.f.b configure -relief flat
      if {$define_value eq ""} {
        .defwin.bf.ok configure -state disabled
      }
    } else {
      .defwin.f.v delete 0 end
      .defwin.f.v configure -state disabled
      .defwin.f.b configure -relief raised
    }
  }
  bind .defwin.f.b <Return> {%W invoke}

  # If a value was specified, enable the textbox and set the relief to flat on the '=' button
  if {$value ne ""} {
    .defwin.f.v configure -state normal
    .defwin.f.b configure -relief flat
  }
  
  # Pack the input widgets
  grid rowconfigure    .defwin.f 1 -weight 1
  grid columnconfigure .defwin.f 3 -weight 1
  grid .defwin.f.l -row 0 -column 0 -sticky news -pady 4 -padx 8
  grid .defwin.f.e -row 0 -column 1 -sticky news -pady 4 -padx 8
  grid .defwin.f.b -row 0 -column 2 -sticky news -pady 4 -padx 8
  grid .defwin.f.v -row 0 -column 3 -sticky news -pady 4 -padx 8

  # Add button frame and widgets
  frame .defwin.bf
  button .defwin.bf.ok -text "OK" -width 10 -state disabled -command {
    if {[.defwin.f.b cget -relief] eq "raised"} {
      set def_retval "$define_name"
    } else {
       set def_retval "$define_name=$define_value"
    }
    destroy .defwin
  }
  bind .defwin.bf.ok <Return> {%W invoke}
  button .defwin.bf.cancel -text "Cancel" -width 10 -pady 4 -padx 8 -command {
    set def_retval ""
    destroy .defwin
  }
  bind .defwin.bf.cancel <Return> {%W invoke}
  pack .defwin.bf.cancel -side right -padx 8 -pady 4
  pack .defwin.bf.ok     -side right -padx 8 -pady 4

  # Provide functionality for entries
  .defwin.f.e configure -vcmd { return [define_update_state] }
  .defwin.f.v configure -vcmd { return [define_update_state] }

  # Pack the frames
  pack .defwin.f  -fill x
  pack .defwin.bf -fill x

  # Make sure that this window is transient and sets the first focus
  wm transient .defwin .newwin
  focus .defwin.f.e

  # Wait for the define window to be destroyed before returning
  tkwait window .defwin

  return $def_retval
  
}

proc parameter_update_state {} {

  global param_name param_value
 
  if {$param_name ne ""} {
    if {$param_value ne ""} {
      .parmwin.bf.ok configure -state normal
    } else {
      .parmwin.bf.ok configure -state disabled
    }
  } else {
    .parmwin.bf.ok configure -state disabled
  }

  return 1

}

proc get_parameter_override {parmname value} {

  global param_retval param_name param_value

  set param_retval ""
  set param_name   $parmname
  set param_value  $value

  # Create new window
  toplevel .parmwin
  wm title .parmwin "Specify a command-line parameter override"

  # Add selection widgets
  frame .parmwin.f -relief raised
  label .parmwin.f.l1 -text "Parameter scope:"
  entry .parmwin.f.e  -textvariable param_name -validate all
  label .parmwin.f.l2 -text "="
  entry .parmwin.f.v  -textvariable param_value -validate all

  # Pack the input widgets
  pack .parmwin.f.l1 -padx 8 -pady 4 -side left -fill x
  pack .parmwin.f.e  -padx 8 -pady 4 -side left -fill x -expand 1
  pack .parmwin.f.l2 -padx 8 -pady 4 -side left -fill x
  pack .parmwin.f.v  -padx 8 -pady 4 -side left -fill x -expand 1

  # Add button frame and widgets
  frame .parmwin.bf -relief raised
  button .parmwin.bf.ok -text "OK" -width 10 -state disabled -command {
    set param_retval "$param_name=$param_value"
    destroy .parmwin
  }
  bind .parmwin.bf.ok <Return> {%W invoke}
  button .parmwin.bf.cancel -text "Cancel" -width 10 -command {
    set param_retval ""
    destroy .parmwin
  }
  bind .parmwin.bf.cancel <Return> {%W invoke}
  pack .parmwin.bf.cancel -side right -padx 8 -pady 4
  pack .parmwin.bf.ok     -side right -padx 8 -pady 4

  # Provide entry functionality
  .parmwin.f.e configure -vcmd { return [parameter_update_state] }
  .parmwin.f.v configure -vcmd { return [parameter_update_state] }

  # Pack the frames
  pack .parmwin.f  -fill x
  pack .parmwin.bf -fill x

  # Make sure that this window is transient and sets the first focus
  wm transient .parmwin .newwin
  focus .parmwin.f.e

  # Wait for the parameter window to be destroyed before returning
  tkwait window .parmwin

  return $param_retval

}

proc fsmd_update_state {} {

  global fsmd_modname fsmd_output

  if {$fsmd_modname ne ""} {
    if {$fsmd_output ne ""} {
      .fsmdwin.bf.ok configure -state normal
    } else {
      .fsmdwin.bf.ok configure -state disabled
    }
  } else {
    .fsmdwin.bf.ok configure -state disabled
  }

  return 1

}

proc get_fsm {modname input output} {

  global fsmd_retval
  global fsmd_modname fsmd_input fsmd_output

  set fsmd_retval  ""
  set fsmd_modname $modname
  set fsmd_input   $input
  set fsmd_output  $output

  # Create new window
  toplevel .fsmdwin
  wm title .fsmdwin "Specify an FSM input/output state"

  # Add input widgets
  frame .fsmdwin.f -relief raised
  label .fsmdwin.f.l -text "Module name:"
  entry .fsmdwin.f.e -textvariable fsmd_modname -validate all
  label .fsmdwin.f.lo -text "Output Expression:"
  button .fsmdwin.f.b -text "Input == Output" -command {
    set fsmd_input $fsmd_output
  }
  bind .fsmdwin.f.b <Return> {%W invoke}
  entry .fsmdwin.f.eo -textvariable fsmd_output -validate all
  label .fsmdwin.f.li -text "Input Expression:"
  entry .fsmdwin.f.ei -textvariable fsmd_input

  grid columnconfigure .fsmdwin.f 1 -weight 1
  grid columnconfigure .fsmdwin.f 3 -weight 1
  grid .fsmdwin.f.l  -row 0 -column 0 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.e  -row 0 -column 1 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.lo -row 0 -column 2 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.eo -row 0 -column 3 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.b  -row 1 -column 1 -sticky e    -padx 8 -pady 4
  grid .fsmdwin.f.li -row 1 -column 2 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.ei -row 1 -column 3 -sticky news -padx 8 -pady 4

  # Add button frame and widgets
  frame .fsmdwin.bf -relief raised
  button .fsmdwin.bf.ok -text "OK" -width 10 -state disabled -command {
    set fsmd_retval "$fsmd_modname=$fsmd_input,$fsmd_output"
    destroy .fsmdwin
  }
  bind .fsmdwin.bf.ok <Return> {%W invoke}
  button .fsmdwin.bf.cancel -text "Cancel" -width 10 -command {
    set fsmd_retval ""
    destroy .fsmdwin
  }
  bind .fsmdwin.bf.cancel <Return> {%W invoke}
  pack .fsmdwin.bf.cancel -side right -padx 8 -pady 4
  pack .fsmdwin.bf.ok     -side right -padx 8 -pady 4

  # Provide functionality for entries
  .fsmdwin.f.e  configure -vcmd { return [fsmd_update_state] }
  .fsmdwin.f.eo configure -vcmd { return [fsmd_update_state] }

  # Pack frames
  pack .fsmdwin.f  -fill x
  pack .fsmdwin.bf -fill x
  
  # Make sure that this window is transient and sets the first focus
  wm transient .fsmdwin .newwin
  focus .fsmdwin.f.e

  # Wait for the parameter window to be destroyed before returning
  tkwait window .fsmdwin

  return $fsmd_retval

}
