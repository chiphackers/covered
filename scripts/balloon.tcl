namespace eval balloon {
  variable long_delay 750
  variable short_delay 50
  variable off_delay 5000
  variable delay $long_delay
  variable family {}
}

bind . <Enter> {
  if {$show_tooltips == true} {
    if {$balloon::family != ""} {
      if {[lsearch -exact $balloon::family %W] == -1} {
        set balloon::family {}
        set balloon::delay $balloon::long_delay
      }
    }
  }
}

proc balloon_show {w help} {

  global show_tooltips

  if {$show_tooltips == true} {
    after $balloon::delay [list balloon::show $w $help]
    set balloon::delay $balloon::short_delay
    set balloon::family [balloon::getwfamily $w]
    after $balloon::off_delay destroy $w.balloon
  }

}

proc balloon_destroy {w} {

  global show_tooltips

  if {$show_tooltips == true} {
    destroy $w.balloon
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

proc balloon::show {w arg} {

  if {[eval winfo containing  [winfo pointerxy .]]!=$w} {return}

  set top $w.balloon
  catch {destroy $top}
  toplevel $top -bd 1 -bg black
  wm overrideredirect $top 1
  if {[string equal [tk windowingsystem] aqua]}  {
    ::tk::unsupported::MacWindowStyle style $top help none
  }
  pack [message $top.txt -aspect 10000 -bg lightyellow -padx 1 -pady 0 \
          -text $arg]
  set wmx [expr [winfo rootx $w]+5]
  set wmy [expr [winfo rooty $w]+[winfo height $w]+7]
  wm geometry $top \
    [winfo reqwidth $top.txt]x[winfo reqheight $top.txt]+$wmx+$wmy
  raise $top

}
