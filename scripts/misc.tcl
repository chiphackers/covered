# Contains miscellaneous procs used by various other scripts.

proc create_horizontal_pane {name left_widget right_widget init_handle_pos} {

  # Create pane frames
  frame $name -bg black
  frame $name.l
  frame $name.r
  frame $name.h -borderwidth 2 -relief raised -cursor sb_h_double_arrow

  # Fix placement paremeters
  place $name.l -relheight 1 -width -1
  place $name.r -relheight 1 -relx 1 -anchor sw -width -1
  place $name.h -rely 0.9 -anchor s -width 10 -height 10

  bind $name <Configure> {
    set H  [winfo width %W]
    set X0 [winfo rootx %W]
  }

  bind $name.h <B1-Motion> {
    place_hp [winfo parent %W] [expr {(%X-$X0)/double($H)}]
  }

  place_hp $name $init_handle_pos

  # Pack left and right widgets into frames
  pack $left_widget  -in $name.l -expand yes
  pack $right_widget -in $name.r -expand yes

  return $name

}

proc place_hp {name fract} {

  place $name.l -relwidth $fract
  place $name.h -relx $fract
  place $name.r -relwidth [expr {1.0 - $fract}]

}

proc create_vertical_pane {name top_widget bottom_widget init_handle_pos} {

  # Create pane frames
  frame $name -bg black
  frame $name.t
  frame $name.b
  frame $name.h -borderwidth 2 -relief raised -cursor sb_v_double_arrow

  # Fix placement paremeters
  place $name.t -relwidth 1 -height -1
  place $name.b -relwidth 1 -rely 1 -anchor sw -height -1
  place $name.h -relx 0.9 -anchor e -width 10 -height 10
  pack  $name

  puts "toplevel: [winfo toplevel $name]"
  bind [winfo toplevel $name] <Configure> {
    global H Y0
    puts %W
    set H  [winfo height [winfo toplevel %W]]
    set Y0 [winfo rooty  [winfo toplevel %W]]
    puts "Initial: $H $Y0"
  }

  bind $name.h <B1-Motion> {
    global H Y0
    puts "%Y $Y0 $H"
    place_vp [winfo parent %W] [expr {(%Y-$Y0)/double($H)}]
  }

  # place_vp $name $init_handle_pos

  # Pack left and right widgets into frames
  # pack $top_widget    -in $name.t -expand yes
  # pack $bottom_widget -in $name.b -expand yes

  return $name

}

proc place_vp {name fract} {

  puts $fract
  place $name.t -in $name -relheight $fract
  place $name.h -in $name -rely $fract
  place $name.b -in $name -relheight [expr {1.0 - $fract}]

}


text .t1 -width 10 -height 10
text .t2 -width 10 -height 10
create_vertical_pane .vp .t1 .t2 .5
