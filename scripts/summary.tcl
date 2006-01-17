proc add_func_unit { name percent num } {

  append label_funit .funit_name $num
  append label_percent .percent_name $num
  append scale_name .scale_name $num

  ;# Calculate color
  if {$percent < 90} {
    set color red
  } elseif {$percent < 100} {
    set color yellow
  } else {
    set color green
  }

  ;# Create module name label
  label $label_funit -text "$name" -anchor w

  ;# Create percent label
  label $label_percent -text "$percent%" -anchor e

  ;# Create scale
  scale $scale_name -bg $color -bd 0 -from 0 -to 100 -orient horizontal -sliderrelief flat -sliderlength $percent -troughcolor #AAAAAA -state normal -showvalue 0 -state disabled

  ;# Pack it in the window manager
  grid $label_funit   -row $num -column 0
  grid $label_percent -row $num -column 1
  grid $scale_name    -row $num -column 2

}

add_func_unit foo 50 0
add_func_unit bar 90 1
add_func_unit foobar 100 2

