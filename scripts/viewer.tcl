set viewer_start_search_index 1.0

proc viewer_show {type title fname} {

  global viewer_start_search_index
  global HOME

  # Create window name
  set win [join [list .$type viewwin] ""]

  if {[winfo exists $win] == 0} {

    toplevel $win
    wm title $win "Covered - $title - $fname"

    # Create search button frame
    frame  $win.top
    frame  $win.top.search -borderwidth 1 -relief ridge -bg white
    label  $win.top.search.f -image [image create photo -file [file join $HOME scripts find.gif]] -bg white -relief flat -borderwidth 0
    bind   $win.top.search.f <ButtonPress-1> "perform_search $win.text.t $win.top.search.e $win.info viewer_start_search_index"
    set_balloon $win.top.search.f "Click to find the next occurrence of the search string"
    entry  $win.top.search.e -width 15 -bg white -relief flat -insertborderwidth 0 -highlightthickness 0 -disabledbackground white
    bind   $win.top.search.e <Return> "perform_search $win.text.t $win.top.search.e $win.info viewer_start_search_index"
    label  $win.top.search.c -image [image create photo -file [file join $HOME scripts clear.gif]] -bg white -relief flat -borderwidth 0
    bind   $win.top.search.c <ButtonPress-1> "
      $win.text.t tag delete search_found
      $win.top.search.e delete 0 end
      $win.info configure -text {}
    "
    set_balloon $win.top.search.c "Click to clear the search string"
    pack $win.top.search.c -side right
    pack $win.top.search.e -side right -padx 3
    pack $win.top.search.f -side right

    pack $win.top.search -side right

    # Create text box frame
    frame     $win.text
    text      $win.text.t -xscrollcommand "$win.text.hb set" -yscrollcommand "$win.text.vb set"
    scrollbar $win.text.vb -command "$win.text.t yview"
    scrollbar $win.text.hb -orient horizontal -command "$win.text.t xview"
    grid rowconfigure    $win.text 0 -weight 1
    grid columnconfigure $win.text 0 -weight 1
    grid $win.text.t  -row 0 -column 0 -sticky news
    grid $win.text.vb -row 0 -column 1 -sticky ns
    grid $win.text.hb -row 1 -column 0 -sticky ew

    # Create information bar frame
    label $win.info -anchor w
   
    # Pack the top-level widgets
    pack $win.top  -fill x
    pack $win.text -fill both -expand yes
    pack $win.info -fill x

    # Read the given filename
    if {[catch {set fp [open $fname "r"]}]} {
      tk_messageBox -message "File $fname Not Found!" -title "No File" -icon error
    }
    set contents [split [read $fp] \n] 
    close $fp

    # Calculate the width to use
    set max_width 0
    foreach line $contents {
      if {[string length $line] > $max_width} {
        set max_width [string length $line]
      }
    }
    $win.text.t configure -width $max_width

    # Populate the textbox
    foreach line $contents {
      $win.text.t insert end "$line\n"
    }

    # Make the textbox read-only
    $win.text.t configure -state disabled

  }

  # Raise the report window to the front
  raise $win

}
