proc create_horizontal_pane {} {

  frame .left
  frame .right
  frame .handle -borderwidth 2 -relief raised -cursor sb_h_double_arrow
  . configure -bg black

  # fixed placement parameters
  place .left  -relheight 1 -width -1
  place .right -relheight 1 -relx 1 -anchor ne -width -1
  place .handle -rely 0.9 -anchor s -width 10 -height 10

  bind . <Configure> {
    puts %W
    set W [winfo width .]
    set X0 [winfo rootx .]
    puts "Initial: $W $X0"
  }
 
  bind .handle <B1-Motion> {
    puts "%X $X0 $W"
    Place [expr {(%X-$X0)/double($W)}]
  }
 
  Place .5                ;# initialization
  # now "pack" whatever you like in ".left" and ".right"

}

proc Place {fract} {
  puts $fract
  place .left -relwidth $fract
  place .handle -relx $fract
  place .right -relwidth [expr {1.0 - $fract}]
}
 
create_horizontal_pane

text .left.t
text .right.t

pack .left.t
pack .right.t
