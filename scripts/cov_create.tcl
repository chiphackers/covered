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

set mod_inst_type "Module"
set cov_rb        Line
set last_cov_rb   Line

proc cov_create {f} {

  global cov_rb file_name start_line end_line last_cov_rb preferences

  # Create option menus
  ttk_optionMenu $f.mod_inst mod_inst_type Module Instance
  set_balloon $f.mod_inst "Selects the coverage accumulated by module or instance"

  ttk_optionMenu $f.metrics cov_rb Line Toggle Memory Logic FSM Assert
  set_balloon $f.metrics "Selects the current coverage metric to examine"

  $f.mod_inst configure -width 8
  $f.metrics  configure -width 8

  trace add variable cov_rb        write cov_change_metric
  trace add variable mod_inst_type write cov_change_type

  # Pack radiobuttons
  pack $f.metrics  -side left  -fill x
  pack $f.mod_inst -side right -fill x

  # Pack the metric selection and summary frames into the current window
  pack $f -side top -fill both

}

proc cov_change_metric {args} {

  global cdd_name cov_rb last_cov_rb

  if {$cdd_name != ""} {
    if {$last_cov_rb != $cov_rb} {
      set last_cov_rb $cov_rb
      if {$cov_rb == "Line"} {
        process_line_cov
      } elseif {$cov_rb == "Toggle"} {
        process_toggle_cov
      } elseif {$cov_rb == "Memory"} {
        process_memory_cov
      } elseif {$cov_rb == "Logic"} {
        process_comb_cov
      } elseif {$cov_rb == "FSM"} {
        process_fsm_cov
      } elseif {$cov_rb == "Assert"} {
        process_assert_cov
      }
    } else {
      if {$cov_rb == "Line"} {
        display_line_cov 
      } elseif {$cov_rb == "Toggle"} { 
        display_toggle_cov
      } elseif {$cov_rb == "Memory"} {
        display_memory_cov
      } elseif {$cov_rb == "Logic"} {
        display_comb_cov
      } elseif {$cov_rb == "FSM"} {
        display_fsm_cov
      } elseif {$cov_rb == "Assert"} {
        display_assert_cov
      }
    }
  }

}

proc cov_change_type args {

  global preferences last_mod_inst_type mod_inst_type

  if {$mod_inst_type != $last_mod_inst_type} {

    set last_mod_inst_type $mod_inst_type

    populate_treeview
    clear_all_windows

  }

}
