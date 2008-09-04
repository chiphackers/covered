set more_dn_img [image create bitmap -data "#define dn_width 22\n#define dn_height 22\nstatic unsigned char dn_bits[] = {\n0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};"] 
set more_up_img [image create bitmap -data "#define up_width 22\n#define up_height 22\nstatic unsigned char up_bits[] = {\n0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};"]

set exclude_default_reasons {{This is a standard reason for exclusion, but it is quite long as you can tell.  What is going to happen with it I wonder?} {Another reason.}}

proc get_exclude_reason {w} {

  global more_dn_img more_up_img
  global exclude_default_reasons exclude_reason

  # Clear the exclusion reason string
  set exclude_reason ""

  toplevel .exclwin
  wm title .exclwin "Create a reason for the exclusion"
  wm resizable .exclwin 1 0

  # Create top- and bottom-most frame
  panedwindow .exclwin.pw -orient vertical
  .exclwin.pw add [frame .exclwin.pw.top -relief raised -borderwidth 1]
  .exclwin.pw add [frame .exclwin.pw.bot -relief raised -borderwidth 1] -hide true

  # Populate the textbox pane
  frame     .exclwin.pw.top.t
  text      .exclwin.pw.top.t.t -wrap word -yscrollcommand {.exclwin.pw.top.t.vb set} -height 10 -width 50
  bind .exclwin.pw.top.t.t <KeyRelease> {
    if {[.exclwin.pw.top.t.t count -chars 1.0 end] < 2} {
      .exclwin.pw.top.bf.save configure -state disabled
    } else {
      .exclwin.pw.top.bf.save configure -state normal
    }
  }
  scrollbar .exclwin.pw.top.t.vb -command {.exclwin.pw.top.t.t yview}
  grid columnconfigure .exclwin.pw.top.t 0 -weight 1
  grid .exclwin.pw.top.t.t  -row 0 -column 0 -sticky news
  grid .exclwin.pw.top.t.vb -row 0 -column 1 -sticky ns

  # Create button frame
  frame  .exclwin.pw.top.bf
  button .exclwin.pw.top.bf.save -text "Save" -width 10 -state disabled -command {
    set exclude_reason [.exclwin.pw.top.t.t get 1.0 end]
    destroy .exclwin
  } 
  button .exclwin.pw.top.bf.close -text "Close" -width 10 -command {
    destroy .exclwin
  }
  frame  .exclwin.pw.top.bf.ibf
  button .exclwin.pw.top.bf.ibf.more -image $more_dn_img -command {
    if {[.exclwin.pw panecget .exclwin.pw.bot -hide]} {
      .exclwin.pw paneconfigure .exclwin.pw.bot -hide false
      .exclwin.pw.top.bf.ibf.more configure -image $more_up_img
    } else {
      .exclwin.pw paneconfigure .exclwin.pw.bot -hide true
      .exclwin.pw.top.bf.ibf.more configure -image $more_dn_img
    }
  }
  set_balloon .exclwin.pw.top.bf.ibf.more "Click to show/hide the default exclusion reason list"
  help_button .exclwin.pw.top.bf.ibf.help ""
  pack .exclwin.pw.top.bf.ibf.more -side left -padx 3
  pack .exclwin.pw.top.bf.ibf.help -side right
  pack .exclwin.pw.top.bf.save     -fill x -pady 3
  pack .exclwin.pw.top.bf.close    -fill x -pady 3
  pack .exclwin.pw.top.bf.ibf      -fill x -side bottom

  # Pack the top pane
  grid columnconfigure .exclwin.pw.top 0 -weight 1
  grid .exclwin.pw.top.t  -row 0 -column 0 -sticky news
  grid .exclwin.pw.top.bf -row 0 -column 1 -sticky ns

  # Populate the listbox pane
  frame .exclwin.pw.bot.l
  tablelist::tablelist .exclwin.pw.bot.l.tl -columns {0 "Default Exclusion Reasons"} -labelcommand tablelist::sortByColumn \
    -yscrollcommand {.exclwin.pw.bot.l.vb set} -stretch all -movablecolumns 1
  foreach {key value} [array get tablelistopts] {
    .exclwin.pw.bot.l.tl configure -$key $value
  }
  bind .exclwin.pw.bot.l.tl <<ListboxSelect>> {
    set row [.exclwin.pw.bot.l.tl curselection]
    .exclwin.pw.top.t.t delete 1.0 end
    .exclwin.pw.top.t.t insert end [.exclwin.pw.bot.l.tl getcells [list $row,0]]
    .exclwin.pw.top.bf.save configure -state normal
  }
  scrollbar .exclwin.pw.bot.l.vb -command {.exclwin.pw.bot.l.tl yview}

  grid columnconfigure .exclwin.pw.bot.l 0 -weight 1
  grid .exclwin.pw.bot.l.tl -row 0 -column 0 -sticky news
  grid .exclwin.pw.bot.l.vb -row 0 -column 1 -sticky ns

  # Pack the bottom pane
  pack .exclwin.pw.bot.l -fill both -expand yes

  # Pack the paned window
  pack .exclwin.pw -fill both -expand yes

  # Populate the hidden listbox
  foreach reason $exclude_default_reasons {
    .exclwin.pw.bot.l.tl insert end [list $reason]
  }

  # Make sure that this window is a transient window and set focus
  #if {[winfo viewable $w] } {
  #  wm transient .exclwin $w
  #}
  tkwait visibility .exclwin
  catch {grab .exclwin}
  focus .exclwin.pw.top.t.t

  # Wait for window to be destroyed before moving on
  tkwait window .exclwin

  return $exclude_reason

}

proc show_exclude_reason {reason} {

  # TBD
  toplevel .exclwin
  label .exclwin.l -text $reason

}
