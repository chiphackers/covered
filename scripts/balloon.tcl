namespace eval balloon {
  variable long_delay 750
  variable short_delay 50
  variable off_delay 5000
  variable delay $long_delay
  variable family {}
  variable after_id {}
}

bind . <Enter> {
  if {$preferences(show_tooltips)} {
    if {$balloon::family != ""} {
      if {[lsearch -exact $balloon::family %W] == -1} {
        set balloon::family {}
        set balloon::delay $balloon::long_delay
      }
    }
  }
}

proc balloon_show {w help} {

  global preferences

  if {$preferences(show_tooltips)} {
    set balloon::after_id [after $balloon::delay [list balloon::show $w $help]]
    set balloon::delay $balloon::short_delay
    set balloon::family [balloon::getwfamily $w]
    set balloon::after_id [after $balloon::off_delay destroy $w.balloon]
  }

}

proc balloon_destroy {w} {

  global preferences

  if {$preferences(show_tooltips)} {
    balloon::hide $w
  }

}

proc set_balloon {w help} {

  bind $w <Enter> "+balloon_show %W [list $help]"
  bind $w <Leave> "+balloon_destroy %W"
  bind $w <Button> "+balloon_destroy %W"

}

# Add these to the namespace
proc balloon::getwfamily {w} {
  return [winfo children [winfo parent $w]]
}

proc balloon::show {w arg {bgcolor lightyellow} {fgcolor black}} {

  if {[string compare -length [string length $w] [eval winfo containing  [winfo pointerxy .]] $w] != 0} {return}

  after cancel $balloon::after_id
  set top $w.balloon
  catch {destroy $top}
  toplevel $top -bd 1 -bg black
  wm overrideredirect $top 1
  if {[string equal [tk windowingsystem] aqua]}  {
    ::tk::unsupported::MacWindowStyle style $top help none
  }
  pack [message $top.txt -aspect 1500 -bg $bgcolor -fg $fgcolor -padx 1 -pady 0 -text $arg]
  set wmx [winfo pointerx $w]
  set wmy [expr [winfo pointery $w]+20]
  wm geometry $top [winfo reqwidth $top.txt]x[winfo reqheight $top.txt]+$wmx+$wmy
  raise $top

}

proc balloon::hide {w} {
  destroy $w.balloon
}
