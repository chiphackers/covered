package require Tablelist

proc create_exclude_reason {} {

  if {[winfo exists .exclwin] == 0} {

    toplevel .exclwin
    wm title .exclwin "Create a reason for the exclusion"

    # Create top- and bottom-most frame
    panedwindow .exclwin.pw
    .exclwin.pw add [frame .exclwin.pw.top -relief raised -borderwidth 1]
    .exclwin.pw add [frame .exclwin.pw.bot -relief raised -borderwidth 1] -hide true

    # Create button frame
    frame  .exclwin.pw.top.bf -relief raised -borderwidth 1
    button .exclwin.pw.top.bf.save -text "Save" -state disabled -command {
      # Take the text from the textbox and save it in the database
    } 
    button .exclwin.pw.top.bf.save_std -text "Save As Standard" -state disabled -command {
      # Take the text from the textbox and save it as a standard reason
    }
    button .exclwin.pw.top.bf.use_std -text "Show Standard" -command {
      if {[.exclwin.pw.top.bf.use_std cget -text] eq "Show Standard"} {
        .exclwin.pw paneconfigure .exclwin.pw.bot -hide false
        .exclwin.pw.top.bf.use_std configure -text "Hide Standard"
      } else {
        .exclwin.pw paneconfigure .exclwin.pw.bot -hide true
        .exclwin.pw.top.bf.use_std configure -text "Show Standard"
      }
    }
    button .exclwin.pw.top.bf.close -text "Close" -command {
      destroy .exclwin
    }
    pack .exclwin.pw.top.bf.save     -fill x
    pack .exclwin.pw.top.bf.save_std -fill x
    pack .exclwin.pw.top.bf.use_std  -fill x
    pack .exclwin.pw.top.bf.close    -fill x

    # Populate the textbox pane
    frame     .exclwin.pw.top.t
    text      .exclwin.pw.top.t.t -wrap word -yscrollcommand {.exclwin.pw.top.t.vb set} -height 5 -width 30
    scrollbar .exclwin.pw.top.t.vb -command {.exclwin.pw.top.t.t yview}
    
    grid columnconfigure .exclwin.pw.top.t 0 -weight 1
    grid .exclwin.pw.top.t.t  -row 0 -column 0 -sticky news
    grid .exclwin.pw.top.t.vb -row 0 -column 1 -sticky ns

    grid columnconfigure .exclwin.pw.top 1 -weight 1
    grid .exclwin.pw.top.bf -row 0 -column 0 -sticky ns
    grid .exclwin.pw.top.t  -row 0 -column 1 -sticky news

    # Populate the listbox pane
    frame .exclwin.pw.bot.l
    tablelist::tablelist .exclwin.pw.bot.l.tl -columns {0 "Reason"} -labelcommand tablelist::sortByColumn \
      -yscrollcommand {.exclwin.pw.bot.l.vb set} -stretch all -movablecolumns 1
    scrollbar .exclwin.pw.bot.l.vb -command {.exclwin.pw.bot.l.tl yview}

    grid columnconfigure .exclwin.pw.bot.l 0 -weight 1
    grid .exclwin.pw.bot.l.tl -row 0 -column 0 -sticky news
    grid .exclwin.pw.bot.l.vb -row 0 -column 1 -sticky ns

    pack .exclwin.pw.bot.l -fill both -expand yes

    pack .exclwin.pw -fill both -expand yes

  }

  # Bring the window to the foreground
  raise .exclwin

}

create_exclude_reason
