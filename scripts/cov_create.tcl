set cov_rb line

proc cov_create {.covbox} {

  global cov_rb file_name start_line end_line

  radiobutton .covbox.line -variable cov_rb -value line   -text "Line" -command { 
    if {$file_name != 0} {
      display_line_cov .bot.txt $start_line $end_line
    } 
  }
  radiobutton .covbox.tog  -variable cov_rb -value toggle -text "Toggle" -command {
    if {$file_name != 0} {
      display_toggle_cov .bot.txt $start_line $end_line
    }
  } -state disabled
  radiobutton .covbox.comb -variable cov_rb -value comb   -text "Logic" -command {
    if {$file_name != 0} {
      display_comb_cov .bot.txt $start_line $end_line
    }
  } -state disabled
  radiobutton .covbox.fsm  -variable cov_rb -value fsm    -text "FSM" -command {
    if {$file_name != 0} {
      display_fsm_cov .bot.txt $start_line $end_line
    }
  } -state disabled

  label .covbox.l -text "Summary Information:"
  label .covbox.ht -relief sunken -width 10 -anchor e -text "0"
  label .covbox.h -text "hit out of"
  label .covbox.tt -relief sunken -width 10 -anchor e -text "0"

  # Cause line coverage to be the default
  .covbox.line select

  # Pack the .covbox frame
  pack .covbox -side top -fill both
  pack .covbox.line -side left
  pack .covbox.tog  -side left
  pack .covbox.comb -side left
  pack .covbox.fsm  -side left
  pack .covbox.tt   -side right
  pack .covbox.h    -side right
  pack .covbox.ht   -side right
  pack .covbox.l    -side right

}
