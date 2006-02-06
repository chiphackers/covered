#!/usr/bin/env wish

# Include the necessary auxiliary files 
source $HOME/scripts/menu_create.tcl
source $HOME/scripts/cov_create.tcl
source $HOME/scripts/process_file.tcl
source $HOME/scripts/toggle.tcl
source $HOME/scripts/comb.tcl
source $HOME/scripts/help.tcl
source $HOME/scripts/summary.tcl
source $HOME/scripts/preferences.tcl

set last_lb_index ""
set lwidth        -1 

proc main_view {} {

  global race_msgs

  # Start off 

  # Get all race condition reason messages
  set race_msgs ""
  tcl_func_get_race_reason_msgs

  # Create the frame for menubar creation
  frame .menubar -width 710 -relief raised -borderwidth 1
  menu_create .menubar

  # Create the information frame
  frame .covbox -width 710 -height 25
  cov_create .covbox

  # Create the bottom frame
  frame .bot -width 120 -height 300

  # Create frames for pane handle
  frame .bot.left -relief raised -borderwidth 1
  frame .bot.right -relief raised -borderwidth 1
  frame .bot.handle -borderwidth 2 -relief raised -cursor sb_h_double_arrow

  place .bot.left  -relheight 1 -width -1
  place .bot.right -relheight 1 -relx 1 -anchor ne -width -1
  place .bot.handle -rely 0.5 -anchor s -width 8 -height 8

  bind .bot <Configure> {
    set W  [winfo width .bot]
    set X0 [winfo rootx .bot]
    if {$lwidth == -1} {
      set lwidth [expr .25 * $W]
    }
    main_place $W $lwidth
  }

  bind .bot.handle <B1-Motion> {
    set lwidth [expr %X - $X0]
    main_place $W $lwidth
  }

  # Create the listbox label
  label .bot.left.ll -text "Modules/Instances" -anchor w

  # Create the textbox label
  label .bot.right.tl -text "Line #       Verilog Source" -anchor w

  # Create the listbox widget to display file names
  listbox .bot.left.l -yscrollcommand ".bot.left.lvb set" -xscrollcommand ".bot.left.lhb set" -width 30
  bind .bot.left.l <<ListboxSelect>> populate_text
  scrollbar .bot.left.lvb -command ".bot.left.l yview"
  scrollbar .bot.left.lhb -orient horizontal -command ".bot.left.l xview"

  # Create the text widget to display the modules/instances
  text .bot.right.txt -yscrollcommand ".bot.right.vb set" -xscrollcommand ".bot.right.hb set" -wrap none -state disabled
  scrollbar .bot.right.vb -command ".bot.right.txt yview"
  scrollbar .bot.right.hb -orient horizontal -command ".bot.right.txt xview"

  # Create bottom information bar
  label .info -anchor w -relief raised -borderwidth 1

  # Pack the widgets into the bottom left and right frames
  grid rowconfigure    .bot.left 1 -weight 1
  grid columnconfigure .bot.left 0 -weight 1
  grid .bot.left.ll  -row 0 -column 0 -sticky nsew
  grid .bot.left.l   -row 1 -column 0 -sticky nsew
  grid .bot.left.lvb -row 1 -column 1 -sticky ns
  grid .bot.left.lhb -row 2 -column 0 -sticky ew

  grid rowconfigure    .bot.right 1 -weight 1
  grid columnconfigure .bot.right 0 -weight 1
  grid .bot.right.tl  -row 0 -column 0 -sticky nsew
  grid .bot.right.txt -row 1 -column 0 -sticky nsew
  grid .bot.right.vb  -row 1 -column 1 -sticky ns
  grid .bot.right.hb  -row 2 -column 0 -sticky ew

  # Pack the widgets
  pack .bot  -fill both -expand yes
  pack .info -fill both

  # Set the window icon
  global HOME
  # wm iconbitmap . @$HOME/images/myfile.xbm
  wm title . "Covered - Verilog Code Coverage Tool"

  # Set focus on the new window
  # wm focus .

}

proc main_place {width value} {

  place .bot.left   -width $value
  place .bot.handle -x $value
  place .bot.right  -width [expr $width - $value]

}
 
proc populate_listbox {listbox_w} {

  global mod_inst_type funit_names funit_types inst_list cov_rb file_name
  global line_summary_total line_summary_hit
  global toggle_summary_total toggle_summary_hit
  global uncov_fgColor uncov_bgColor
  global lb_fgColor lb_bgColor
 
  if {$file_name != 0} {

    # Remove contents currently in listbox
    set lb_size [$listbox_w size]
    $listbox_w delete 0 $lb_size

    # If we are in module mode, list modules (otherwise, list instances)
    if {$mod_inst_type == "module"} {
      set funit_names ""
      set funit_types ""
      tcl_func_get_funit_list 
      foreach funit_name $funit_names {
        $listbox_w insert end $funit_name
      }
    } else {
      set inst_list ""
      tcl_func_get_instance_list
      foreach inst_name $inst_list {
        $listbox_w insert end $inst_name
      }
    }

    # Get default colors of listbox
    set lb_fgColor [.bot.left.l itemcget 0 -foreground]
    set lb_bgColor [.bot.left.l itemcget 0 -background]

    highlight_listbox

  }

}

proc highlight_listbox {} {

  global file_name funit_names funit_types cov_rb
  global uncov_fgColor uncov_bgColor lb_fgColor lb_bgColor
  global line_summary_total line_summary_hit
  global toggle_summary_total toggle_summary_hit
  global comb_summary_total comb_summary_hit

  if {$file_name != 0} {

    # If we are in module mode, list modules (otherwise, list instances)
    set curr_line 0
    set funits [llength $funit_names]
    for {set i 0} {$i < $funits} {incr i} {
      if {$cov_rb == "line"} {
        tcl_func_get_line_summary [lindex $funit_names $i] [lindex $funit_types $i]
        set fully_covered [expr $line_summary_total == $line_summary_hit]
      } elseif {$cov_rb == "toggle"} {
        tcl_func_get_toggle_summary [lindex $funit_names $i] [lindex $funit_types $i]
        set fully_covered [expr $toggle_summary_total == $toggle_summary_hit]
      } elseif {$cov_rb == "comb"} {
        tcl_func_get_comb_summary [lindex $funit_names $i] [lindex $funit_types $i]
        set fully_covered [expr $comb_summary_total == $comb_summary_hit]
      } elseif {$cov_rb == "fsm"} {
      } else {
        # ERROR
      }
      if {$fully_covered == 0} {
        .bot.left.l itemconfigure $curr_line -foreground $uncov_fgColor -background $uncov_bgColor
      } else {
        .bot.left.l itemconfigure $curr_line -foreground $lb_fgColor -background $lb_bgColor
      }
      incr curr_line
    }

  }

}

proc populate_text {} {

  global cov_rb mod_inst_type funit_names funit_types
  global curr_funit_name curr_funit_type last_lb_index

  set index [.bot.left.l curselection]

  if {$index != ""} {

    if {$last_lb_index != $index} {

      set last_lb_index $index
      set curr_funit_name [lindex $funit_names $index]
      set curr_funit_type [lindex $funit_types $index]

      if {$cov_rb == "line"} {
        process_funit_line_cov
      } elseif {$cov_rb == "toggle"} {
        process_funit_toggle_cov
      } elseif {$cov_rb == "comb"} {
        process_funit_comb_cov
      } elseif {$cov_rb == "fsm"} {
        process_funit_fsm_cov
      } else {
        # ERROR
      }

    }

  }

}

proc bgerror {msg} {

  ;# Remove the status window if it exists
  destroy .status

  ;# Display error message
  set retval [tk_messageBox -message $msg -title "Error" -type ok]

}

# Read configuration file
read_coveredrc

# Display main window
main_view
