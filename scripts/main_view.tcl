#!/usr/bin/env wish

# Include the necessary auxiliary files 
source $HOME/scripts/menu_create.tcl
source $HOME/scripts/cov_create.tcl
source $HOME/scripts/process_file.tcl
source $HOME/scripts/toggle.tcl
source $HOME/scripts/comb.tcl

set last_lb_index ""

proc main_view {} {

  # Start off 

  # Create the frame for menubar creation
  frame .menubar -width 710 -height 20 
  menu_create .menubar

  # Create the information frame
  frame .covbox -width 710 -height 25
  cov_create .covbox

  # Create the bottom frame
  frame .bot -width 120 -height 300

  # Create the listbox label
  label .bot.ll -text "Modules/Instances" -anchor w

  # Create the textbox label
  label .bot.tl -text "Line #       Verilog Source" -anchor w

  # Create the listbox widget to display file names
  listbox .bot.l -yscrollcommand ".bot.lvb set" -xscrollcommand ".bot.lhb set" -width 30
  bind .bot.l <<ListboxSelect>> populate_text
  scrollbar .bot.lvb -command ".bot.l yview"
  scrollbar .bot.lhb -orient horizontal -command ".bot.l xview"

  # populate_listbox .bot.l

  # Create the text widget to display the modules/instances
  text .bot.txt -yscrollcommand ".bot.vb set" -xscrollcommand ".bot.hb set" -wrap none -state disabled
  scrollbar .bot.vb -command ".bot.txt yview"
  scrollbar .bot.hb -orient horizontal -command ".bot.txt xview"

  # Create bottom information bar
  label .bot.info -anchor w

  # Pack the widgets into the bottom frame
  grid rowconfigure    .bot 1 -weight 1
  grid columnconfigure .bot 0 -weight 0
  grid columnconfigure .bot 2 -weight 1
  grid .bot.ll   -row 0 -column 0 -sticky nsew
  grid .bot.tl   -row 0 -column 2 -sticky nsew
  grid .bot.l    -row 1 -column 0 -sticky nsew
  grid .bot.lvb  -row 1 -column 1 -sticky ns
  grid .bot.lhb  -row 2 -column 0 -sticky ew
  grid .bot.txt  -row 1 -column 2 -sticky nsew
  grid .bot.vb   -row 1 -column 3 -sticky ns
  grid .bot.hb   -row 2 -column 2 -sticky ew
  grid .bot.info -row 3 -column 0 -columnspan 4 -sticky ew

  # Pack the widgets
  pack .bot -fill both -expand yes -side bottom

  # Set the window icon
  global HOME
  # wm iconbitmap . @$HOME/images/myfile.xbm
  wm title . "Covered - Verilog Code Coverage Tool"

  # Set focus on the new window
  # wm focus .

}

proc populate_listbox {listbox_w} {

  global mod_inst_type mod_list inst_list cov_rb file_name
  global line_summary_total line_summary_hit
  global toggle_summary_total toggle_summary_hit01 toggle_summary_hit10
  global uncov_fgColor uncov_bgColor
  global lb_fgColor lb_bgColor
 
  if {$file_name != 0} {

    # Remove contents currently in listbox
    set lb_size [$listbox_w size]
    $listbox_w delete 0 $lb_size

    # If we are in module mode, list modules (otherwise, list instances)
    if {$mod_inst_type == "module"} {
      set mod_list ""
      tcl_func_get_module_list 
      foreach mod_name $mod_list {
        $listbox_w insert end $mod_name
      }
    } else {
      set inst_list ""
      tcl_func_get_instance_list
      foreach inst_name $inst_list {
        $listbox_w insert end $inst_name
      }
    }

    # Get default colors of listbox
    set lb_fgColor [.bot.l itemcget 0 -foreground]
    set lb_bgColor [.bot.l itemcget 0 -background]

    highlight_listbox

  }

}

proc highlight_listbox {} {

  global file_name mod_list cov_rb
  global uncov_fgColor uncov_bgColor lb_fgColor lb_bgColor
  global line_summary_total line_summary_hit
  global toggle_summary_total toggle_summary_hit01 toggle_summary_hit10
  global comb_summary_total comb_summary_hit

  if {$file_name != 0} {

    # If we are in module mode, list modules (otherwise, list instances)
    set curr_line 0
    foreach mod_name $mod_list {
      if {$cov_rb == "line"} {
        tcl_func_get_line_summary $mod_name
        set fully_covered [expr $line_summary_total == $line_summary_hit]
      } elseif {$cov_rb == "toggle"} {
        tcl_func_get_toggle_summary $mod_name
        set fully_covered [expr [expr $toggle_summary_total == $toggle_summary_hit01] && [expr $toggle_summary_total == $toggle_summary_hit10]]
      } elseif {$cov_rb == "comb"} {
        tcl_func_get_comb_summary $mod_name
        set fully_covered [expr $comb_summary_total == $comb_summary_hit]
      } elseif {$cov_rb == "fsm"} {
      } else {
        # ERROR
      }
      if {$fully_covered == 0} {
        .bot.l itemconfigure $curr_line -foreground $uncov_fgColor -background $uncov_bgColor
      } else {
        .bot.l itemconfigure $curr_line -foreground $lb_fgColor -background $lb_bgColor
      }
      incr curr_line
    }

  }

}

proc populate_text {} {

  global cov_rb mod_inst_type mod_list
  global curr_mod_name last_lb_index

  set index [.bot.l curselection]

  if {$index != ""} {

    if {$last_lb_index != $index} {

      set last_lb_index $index

      if {$mod_inst_type == "instance"} {
        set curr_mod_name [lindex $mod_list $index]
      } else {
        set curr_mod_name [.bot.l get $index]
      }

      if {$cov_rb == "line"} {
        process_module_line_cov
      } elseif {$cov_rb == "toggle"} {
        process_module_toggle_cov
      } elseif {$cov_rb == "comb"} {
        process_module_comb_cov
      } elseif {$cov_rb == "fsm"} {
        process_module_fsm_cov
      } else {
        # ERROR
      }

    }

  }

}

# proc bgerror {msg} {

  ;# Remove the status window if it exists
#   destroy .status

  ;# Display error message
#   set retval [tk_messageBox -message $msg -title "Error" -type ok]

# }

main_view
