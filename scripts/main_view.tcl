#!/usr/bin/env wish

# Include the necessary auxiliary files 
source $HOME/scripts/menu_create.tcl
source $HOME/scripts/cov_create.tcl
source $HOME/scripts/process_file.tcl

proc main_view {} {

  # Start off 

  # Create the frame for menubar creation
  frame .menubar -width 710 -height 20 
  menu_create .menubar

  # Create the frame for toolbars
  #frame .toolbar -width 710 -height 12
  #toolbar_create .toolbar

  # Create the information frame
  frame .covbox -width 710 -height 25
  cov_create .covbox

  # Create the bottom frame
  frame .bot -width 120 -height 300

  # Create the tree widget to display file names
  listbox .bot.l -yscrollcommand ".bot.lvb set" -xscrollcommand ".bot.lhb set"
  bind .bot.l <<ListboxSelect>> populate_text
  scrollbar .bot.lvb -command ".bot.l yview"
  scrollbar .bot.lhb -orient horizontal -command ".bot.l xview"

  populate_listbox .bot.l

  # Create the text widget to display the modules/instances
  text .bot.txt -yscrollcommand ".bot.vb set" -xscrollcommand ".bot.hb set" -wrap none
  scrollbar .bot.vb -command ".bot.txt yview"
  scrollbar .bot.hb -orient horizontal -command ".bot.txt xview"

  # Pack the widgets into the bottom frame
  grid rowconfigure .bot 0 -weight 1
  grid columnconfigure .bot 0 -weight 1
  grid .bot.l   -row 0 -column 0 -sticky nsew
  grid .bot.lvb -row 0 -column 1 -sticky ns
  grid .bot.lhb -row 1 -column 0 -sticky ew
  grid .bot.txt -row 0 -column 2 -sticky nsew
  grid .bot.vb  -row 0 -column 3 -sticky ns
  grid .bot.hb  -row 1 -column 2 -sticky ew

  # Pack the widgets
  # pack .covbar -fill both -side top
  pack .bot -fill both -expand yes -side bottom

  # Set the window icon
  global HOME
  wm iconbitmap . @$HOME/images/myfile.xbm
  wm title . "Covered - Verilog Code Coverage Tool"

  # Set focus on the new window
  # wm focus .

}

proc populate_listbox {listbox_w} {

  global mod_inst_type mod_list inst_list
  global line_summary_total line_summary_hit
  global uncov_fgColor uncov_bgColor
 
  # Remove contents currently in listbox
  set lb_size [$listbox_w size]
  $listbox_w delete 0 $lb_size

  # If we are in module mode, list modules (otherwise, list instances)
  if {$mod_inst_type == "module"} {
    set mod_list ""
    tcl_func_get_module_list 
    foreach mod_name $mod_list {
      $listbox_w insert end $mod_name
      tcl_func_get_line_summary $mod_name
      if {$line_summary_total != $line_summary_hit} {
        $listbox_w itemconfigure [expr [$listbox_w size] - 1] -foreground $uncov_fgColor -background $uncov_bgColor
      }
    }
  } else {
    set inst_list ""
    tcl_func_get_instance_list
    foreach inst_name $inst_list {
      $listbox_w insert end $inst_name
      tcl_func_get_line_summary [lindex $mod_list [expr [$listbox_w size] - 1]]
      if {$line_summary_total != $line_summary_hit} {
        $listbox_w itemconfigure [expr [$listbox_w size] - 1] -foreground $uncov_fgColor -background $uncov_bgColor
      }
    }
  }

}

proc populate_text {} {

  global cov_rb mod_inst_type mod_list

  set index [.bot.l curselection]

  if {$index != ""} {

    if {$mod_inst_type == "instance"} {
      set mod_name [lindex $mod_list $index]
    } else {
      set mod_name [.bot.l get $index]
    }

    if {$cov_rb == "line"} {
      process_module_line_cov $mod_name .bot.txt
    } elseif {$cov_rb == "toggle"} {
      process_module_toggle_cov $mod_name .bot.txt
    } elseif {$cov_rb == "comb"} {
      process_module_comb_cov $mod_name .bot.txt
    } elseif {$cov_rb == "fsm"} {
      process_module_fsm_cov $mod_name .bot.txt
    } else {
      # ERROR
    }

  }

}

main_view
