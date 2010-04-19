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

proc ttk_optionMenu {w v args} {

  global $v

  ttk::menubutton $w -textvariable $v
  $w configure -menu [menu $w.m -tearoff 0]

  foreach value $args {
    $w.m add radiobutton -variable $v -label $value -value $value
  }

  set $v [lindex $args 0]

  return $w

}

# Include the necessary auxiliary files
source [file join $HOME scripts menu_create.tcl]
source [file join $HOME scripts cov_create.tcl]
source [file join $HOME scripts process_file.tcl]
source [file join $HOME scripts toggle.tcl]
source [file join $HOME scripts comb.tcl]
source [file join $HOME scripts fsm.tcl]
source [file join $HOME scripts help.tcl]
source [file join $HOME scripts summary.tcl]
source [file join $HOME scripts preferences.tcl]
source [file join $HOME scripts cdd_view.tcl]
source [file join $HOME scripts assert.tcl]
source [file join $HOME scripts verilog.tcl]
source [file join $HOME scripts memory.tcl]
source [file join $HOME scripts wizard.tcl]
source [file join $HOME scripts gen_new.tcl]
source [file join $HOME scripts gen_report.tcl]
source [file join $HOME scripts gen_rank.tcl]
source [file join $HOME scripts viewer.tcl]
source [file join $HOME scripts balloon.tcl]
source [file join $HOME scripts exclude.tcl]
if {[tk windowingsystem] eq "aqua"} {
  source [file join $HOME scripts windowlist.tcl]
}

# The Tablelist package is used for displaying instance/module hit/miss/total/percent hit information
package require Tablelist
#package require tile
#package require tablelist_tile 4.8

set last_block              ""
set lwidth                  -1 
set lwidth_h1               -1
set main_start_search_index 1.0
set curr_uncov_index        ""
set prev_uncov_index        ""
set next_uncov_index        ""
set input_cdd               ""
set block_list              [list]

set bm_right [image create bitmap -data "#define bm_right_width 15\n#define bm_right_height 15\nstatic unsigned char bm_right_bits[] = {\n0x80, 0x00,
0x80, 0x01, 0x80, 0x03, 0x80, 0x07, 0xff, 0x0f, 0xff, 0x1f, 0xff, 0x3f, 0xff, 0x7f, 0xff, 0x3f, 0xff, 0x1f, 0xff, 0x0f, 0x80, 0x07, 0x80, 0x03, 0x80,
0x01, 0x80, 0x00};"]
set bm_left  [image create bitmap -data "#define bm_left_width 15\n#define bm_left_height 15\nstatic unsigned char bm_left_bits[] = {\n0x80, 0x00,
0xc0, 0x00, 0xe0, 0x00, 0xf0, 0x00, 0xf8, 0x7f, 0xfc, 0x7f, 0xfe, 0x7f, 0xff, 0x7f, 0xfe, 0x7f, 0xfc, 0x7f, 0xf8, 0x7f, 0xf0, 0x00, 0xe0, 0x00, 0xc0,
0x00, 0x80, 0x00};"]

# images indicating sort order 
image create bitmap treeview_arrow(0) -data {
  #define arrowUp_width 7
  #define arrowUp_height 4
  static char arrowUp_bits[] = {
    0x08, 0x1c, 0x3e, 0x7f
  };
}
image create bitmap treeview_arrow(1) -data {
  #define arrowDown_width 7
  #define arrowDown_height 4
  static char arrowDown_bits[] = {
    0x7f, 0x3e, 0x1c, 0x08
  };
}
image create bitmap treeview_arrowBlank -data {
  #define arrowBlank_width 7
  #define arrowBlank_height 4
  static char arrowBlank_bits[] = {
    0x00, 0x00, 0x00, 0x00
  };
}

array set tablelistopts {
  selectbackground   RoyalBlue1
  selectforeground   white
  stretch	     all
  stripebackground   #EDF3FE
  relief             flat
  border             0
  showseparators     yes
  takefocus          0
  setfocus           1
  activestyle        none
}

proc main_view {} {

  global race_msgs prev_uncov_index next_uncov_index
  global HOME main_start_search_index
  global preferences mod_inst_type
  global bm_right bm_left
  global metric_src metric_dtl

  # Start off 

  # Get all race condition reason messages
  set race_msgs ""
  tcl_func_get_race_reason_msgs

  # Create the frame for menubar creation
  menu_create

  # Create the bottom frame
  ttk::panedwindow .bot -width 1000 -height 500 -orient horizontal

  # Create frames for pane handle
  ttk::frame .bot.left  -relief raised -borderwidth 1
  ttk::frame .bot.right -relief raised -borderwidth 1

  ###############################
  # POPULATE RIGHT BOTTOM FRAME #
  ###############################

  # Create table windows
  trace add variable cov_rb write cov_change_metric

  # Create notebook
  ttk::notebook .bot.right.nb
  bind .bot.right.nb <<NotebookTabChanged>> {
    set cov_rb [lindex [split [.bot.right.nb tab current -text] " "] 0]
    cov_change_metric
  }
  
  # Create global lookup for the source tab
  array set metric_src {
    line   .bot.right.nb.src
    toggle .bot.right.nb.src
    memory .bot.right.nb.src
    comb   .bot.right.nb.src
    fsm    .bot.right.nb.src
    assert .bot.right.nb.src
  }

  # Create global lookup for the detail tab
  array set metric_dtl {
    line   .bot.right.nb.dtl
    toggle .bot.right.nb.dtl
    memory .bot.right.nb.dtl
    comb   .bot.right.nb.dtl
    fsm    .bot.right.nb.dtl
    assert .bot.right.nb.dtl
  }

  # Create Source browser tab
  .bot.right.nb add [ttk::frame .bot.right.nb.src] -text "Source" -underline 0

  # Create the textbox header frame
  ttk::frame .bot.right.nb.src.h
  ttk::label .bot.right.nb.src.h.tl -text "Cur   Line #       Verilog Source" -anchor w
  ttk::frame .bot.right.nb.src.h.pn
  ttk::label .bot.right.nb.src.h.pn.prev -image [image create photo -file [file join $HOME scripts left_arrow.gif]] -state disabled
  bind .bot.right.nb.src.h.pn.prev <Button-1> {
    if {$cov_rb == "Line"} {
      goto_uncov $prev_uncov_index line
    } elseif {$cov_rb == "Toggle"} {
      goto_uncov $prev_uncov_index toggle
    } elseif {$cov_rb == "Memory"} {
      goto_uncov $prev_uncov_index memory
    } elseif {$cov_rb == "Logic"} {
      goto_uncov $prev_uncov_index logic
    } elseif {$cov_rb == "FSM"} {
      goto_uncov $prev_uncov_index fsm
    } elseif {$cov_rb == "Assert"} {
      goto_uncov $prev_uncov_index assert
    }
  }
  set_balloon .bot.right.nb.src.h.pn.prev "Click to view the previous uncovered item"
  ttk::label .bot.right.nb.src.h.pn.next -image [image create photo -file [file join $HOME scripts right_arrow.gif]] -state disabled
  bind .bot.right.nb.src.h.pn.next <Button-1> {
    if {$cov_rb == "Line"} {
      goto_uncov $next_uncov_index line
    } elseif {$cov_rb == "Toggle"} {
      goto_uncov $next_uncov_index toggle
    } elseif {$cov_rb == "Memory"} {
      goto_uncov $next_uncov_index memory
    } elseif {$cov_rb == "Logic"} {
      goto_uncov $next_uncov_index logic
    } elseif {$cov_rb == "FSM"} {
      goto_uncov $next_uncov_index fsm
    } elseif {$cov_rb == "Assert"} {
      goto_uncov $next_uncov_index assert
    }
  }
  set_balloon .bot.right.nb.src.h.pn.next "Click to view the next uncovered item"
  frame .bot.right.nb.src.h.search -borderwidth 1 -relief ridge -bg white
  label .bot.right.nb.src.h.search.find -image [image create photo -file [file join $HOME scripts find.gif]] -background white -state disabled -relief flat
  bind .bot.right.nb.src.h.search.find <ButtonPress-1> {
    perform_search .bot.right.nb.src.txt .bot.right.nb.src.h.search.e .info main_start_search_index
  }
  set_balloon .bot.right.nb.src.h.search.find "Click to find the next occurrence of the search string"
  entry .bot.right.nb.src.h.search.e -width 15 -state disabled -relief flat -bg white
  bind .bot.right.nb.src.h.search.e <Return> {
    perform_search .bot.right.nb.src.txt .bot.right.nb.src.h.search.e .info main_start_search_index
  }
  label .bot.right.nb.src.h.search.clear -image [image create photo -file [file join $HOME scripts clear.gif]] -background white -state disabled -relief flat
  bind .bot.right.nb.src.h.search.clear <ButtonPress-1> {
    .bot.right.nb.src.txt tag delete search_found
    .bot.right.nb.src.h.search.e delete 0 end
    set main_start_search_index 1.0
  }
  set_balloon .bot.right.nb.src.h.search.clear "Click to clear the search string"

  # Pack the previous/next frame
  pack .bot.right.nb.src.h.pn.prev -side left
  pack .bot.right.nb.src.h.pn.next -side left

  # Pack the search frame
  pack .bot.right.nb.src.h.search.find  -side left
  pack .bot.right.nb.src.h.search.e     -side left -padx 3
  pack .bot.right.nb.src.h.search.clear -side left

  # Pack the textbox header frame
  pack .bot.right.nb.src.h.tl     -side left
  pack .bot.right.nb.src.h.pn     -side left -expand yes
  pack .bot.right.nb.src.h.search -side right

  # Create the text widget to display the modules/instances
  text           .bot.right.nb.src.txt -yscrollcommand ".bot.right.nb.src.vb set" -xscrollcommand ".bot.right.nb.src.hb set" \
                                       -wrap none -state disabled
  ttk::scrollbar .bot.right.nb.src.vb -command ".bot.right.nb.src.txt yview"
  ttk::scrollbar .bot.right.nb.src.hb -orient horizontal -command ".bot.right.nb.src.txt xview"

  # Pack the right paned window
  grid rowconfigure    .bot.right.nb.src 1 -weight 1
  grid columnconfigure .bot.right.nb.src 0 -weight 1
  grid .bot.right.nb.src.h   -row 0 -column 0 -columnspan 2 -sticky nsew
  grid .bot.right.nb.src.txt -row 1 -column 0 -sticky nsew
  grid .bot.right.nb.src.vb  -row 1 -column 1 -sticky ns
  grid .bot.right.nb.src.hb  -row 2 -column 0 -sticky ew

  # Create Detail tab
  .bot.right.nb add [ttk::frame .bot.right.nb.dtl] -text "Detail" -underline 0
  .bot.right.nb hide 1

  pack .bot.right.nb -fill both -expand yes

  ##############################
  # POPULATE LEFT BOTTOM FRAME #
  ##############################

  ttk::frame .bot.left.f

  # Create module/instance menubutton
  ttk_optionMenu .bot.left.f.mi mod_inst_type Module Instance
  trace add variable mod_inst_type write cov_change_type
  set_balloon .bot.left.f.mi "Selects the coverage accumulated by module or instance"

  # Create summary panel switcher button
  ttk::button .bot.left.f.ps -image $bm_left -state disabled -command {
    if {[.bot.left.f.ps cget -image] == $bm_left} {
      .bot.left.f.ps configure -image $bm_right
      .bot.left.tree configure -displaycolumns [list $cov_rb]
      .bot.left.tree see $curr_block
      sashpos_move .bot 0 [expr 200 + [.bot.left.tree column $cov_rb -width] + [winfo width .bot.left.vb]]
      .bot pane 0 -weight 0
      .bot pane 1 -weight 1
    } else {
      .bot.left.f.ps configure -image $bm_left
      sashpos_move .bot 0 [winfo width .]
      .bot.left.tree configure -displaycolumns #all
      .bot pane 0 -weight 1
      .bot pane 1 -weight 0
    }
  }
  set_balloon .bot.left.f.ps "View/Hide the module/instance summary table"

  pack .bot.left.f.mi -side left
  pack .bot.left.f.ps -side right

  # Create hierarchical window
  ttk::treeview  .bot.left.tree -selectmode browse -xscrollcommand {.bot.left.hb set} -yscrollcommand {.bot.left.vb set} \
                                -columns {Total Line Toggle Memory Logic FSM Assert}
  .bot.left.tree heading #0     -text "Name"         -command "sort_treeview .bot.left.tree #0 0"
  .bot.left.tree heading Total  -text "Total Score"  -command "sort_treeview .bot.left.tree Total 0"
  .bot.left.tree heading Line   -text "Line Score"   -command "sort_treeview .bot.left.tree Line 0"
  .bot.left.tree heading Toggle -text "Toggle Score" -command "sort_treeview .bot.left.tree Toggle 0"
  .bot.left.tree heading Memory -text "Memory Score" -command "sort_treeview .bot.left.tree Memory 0"
  .bot.left.tree heading Logic  -text "Logic Score"  -command "sort_treeview .bot.left.tree Logic 0"
  .bot.left.tree heading FSM    -text "FSM Score"    -command "sort_treeview .bot.left.tree FSM 0"
  .bot.left.tree heading Assert -text "Assert Score" -command "sort_treeview .bot.left.tree Assert 0"

  set col_font  [::ttk::style lookup [.bot.left.tree cget -style] -font]
  set col_width [font measure $col_font "@@@@@@@@@@@@"]
  foreach col [.bot.left.tree cget -columns] {
    .bot.left.tree column $col -width $col_width -stretch 0
  }
  .bot.left.tree column #0 -stretch 1 -minwidth 200

  ttk::scrollbar .bot.left.vb                      -command {after idle update_treelabels .bot.left.tree; .bot.left.tree yview}
  ttk::scrollbar .bot.left.hb   -orient horizontal -command {after idle update_treelabels .bot.left.tree; .bot.left.tree xview}

  bind .bot.left.tree <B1-Motion>       {+if {$ttk::treeview::State(pressMode)=="resize"} { update_treelabels %W }}
  bind .bot.left.tree <Configure>       "+after idle update_treelabels %W"
  bind .bot.left.tree <<TreeviewOpen>>  "+after idle update_treelabels %W"
  bind .bot.left.tree <<TreeviewClose>> "+after idle update_treelabels %W"

  grid rowconfigure    .bot.left 1 -weight 1
  grid columnconfigure .bot.left 0 -weight 1
  grid .bot.left.f    -row 0 -column 0 -sticky ew   -columnspan 2
  grid .bot.left.tree -row 1 -column 0 -sticky news
  grid .bot.left.vb   -row 1 -column 1 -sticky ns
  grid .bot.left.hb   -row 2 -column 0 -sticky ew

  # Pack the bottom window
  after idle {
    .bot add .bot.left  -weight 1
    .bot add .bot.right -weight 0
    .bot sashpos 0 [winfo width .]
  }

  # Create bottom information bar
  ttk::label .info -anchor w -relief raised

  # Pack the widgets
  pack .bot  -fill both -expand yes
  pack .info -fill both

  # Set the window icon
  wm title . "Covered - Verilog Code Coverage Tool"

  # If window position variables have been set, use them
  if {$preferences(main_geometry) != ""} {
    wm geometry . $preferences(main_geometry)
  }

  # Set focus on the new window
  wm focusmodel . active
  raise .

  # Set icon
  set icon_img [image create photo -file [file join $HOME scripts cov_icon.gif]]
  wm iconphoto . -default $icon_img

  # Catch the closing of the application and potentially save GUI elements
  wm protocol . WM_DELETE_WINDOW {
    check_to_save_and_close_cdd exiting
    save_gui_elements . .
    destroy .
  }
 
}

proc populate_treeview {} {

  global preferences
  global last_mod_inst_type mod_inst_type cdd_name block_list
  global lb_fgColor lb_bgColor
  global summary_list

  # Remove contents currently in listboxes
  .bot.left.tree delete [.bot.left.tree children {}]

  # Delete the treelabels
  foreach block $block_list {
    foreach col [.bot.left.tree cget -columns] {
      destroy .bot.left.tree.[lindex $block 0]$col
    }
  }

  if {$cdd_name != ""} {

    # If we are in module mode, list modules (otherwise, list instances)
    if {$mod_inst_type == "Module"} {

      # Get the list of functional units
      set block_list [tcl_func_get_funit_list]

      # Calculate the summary_list array
      calculate_summary

      foreach block $block_list {
        set total  $summary_list($block,total_percent)
        set line   $summary_list($block,line_percent)
        set toggle $summary_list($block,toggle_percent)
        set memory $summary_list($block,memory_percent)
        set logic  $summary_list($block,comb_percent)
        set fsm    $summary_list($block,fsm_percent)
        set assert $summary_list($block,assert_percent)
        .bot.left.tree insert {} end -id $block -text [tcl_func_get_funit_name $block] -values [list $total $line $toggle $memory $logic $fsm $assert]
        foreach col [.bot.left.tree cget -columns] {
          treelabel .bot.left.tree $block $col
        }
      }

    } else {

      # Get the list of functional unit instances
      set block_list [tcl_func_get_instance_list]

      # Calculate the summary_list array
      calculate_summary

      set inst_block() {}
      foreach block $block_list {
        set inst_name [tcl_func_get_inst_scope $block]
        set mod_name  [tcl_func_get_funit_name $block]
        set inst_block($inst_name) $block
        set parent    $inst_block([join [lrange [split $inst_name .] 0 end-1] .])
        set child     [lindex [split $inst_name .] end]
        set total     $summary_list($block,total_percent)
        set line      $summary_list($block,line_percent)
        set toggle    $summary_list($block,toggle_percent)
        set memory    $summary_list($block,memory_percent)
        set logic     $summary_list($block,comb_percent)
        set fsm       $summary_list($block,fsm_percent)
        set assert    $summary_list($block,assert_percent)
        .bot.left.tree insert $parent end -id $block -text "$child ($mod_name)" -values [list $total $line $toggle $memory $logic $fsm $assert]
        foreach col [.bot.left.tree cget -columns] {
          treelabel .bot.left.tree $block $col
        }
      }

    }

    # Update the headers
    foreach col [.bot.left.tree cget -columns] {
      .bot.left.tree column $col -minwidth [.bot.left.tree.0Total cget -width] -width [.bot.left.tree.0Total cget -width] -stretch 0
    }

    # Update the treelabels
    after idle update_treelabels .bot.left.tree

    # Set the last module/instance type variable to the current
    set last_mod_inst_type $mod_inst_type

  }

}

proc animate_sashpos_move {pw sash diff mv_right goal} {

  # If we still need to move the sash, do so
  if {$diff > 0} {

    # Calculate the new position
    set newdiff [expr $diff >> 1]

    # Move the sash
    if {$mv_right} {
      $pw sashpos $sash [expr $goal - $newdiff]
    } else {
      $pw sashpos $sash [expr $goal + $newdiff]
    }

    # Move after 30 milliseconds
    after 30 animate_sashpos_move $pw $sash $newdiff $mv_right $goal

  }

}

proc sashpos_move {pw sash goal} {

  global preferences

  if {$preferences(enable_animations)} {
    set sashpos [$pw sashpos $sash]
    if {$sashpos < $goal} {
      animate_sashpos_move $pw $sash [expr $goal - $sashpos] 1 $goal
    } else {
      animate_sashpos_move $pw $sash [expr $sashpos - $goal] 0 $goal
    }
  } else {
    $pw sashpos $sash $goal
  }

}

proc treelabel_selected {w col block} {

  global cov_rb curr_block bm_right

  # Update the global variables
  set cov_rb     $col
  set curr_block $block

  # Update the treeview
  update_treelabels $w

  # Update the list of displayed columns to include the name and selected metric
  $w configure -displaycolumns [list $col]

  # We do this to avoid getting an error from the ttk::treeview class
  set ttk::treeview::State(activeHeading) {}

  # Slide the panedwindow sash to hug the last columnconfigure
  if {[.bot.left.f.ps cget -image] != $bm_right} {
    # Need to figure out the desired width of the #0 column
    sashpos_move .bot 0 [expr 200 + [$w column $col -width] + [winfo width .bot.left.vb]]
    .bot.left.f.ps configure -state normal
    .bot pane 0 -weight 0
    .bot pane 1 -weight 1
  }

  # Change the button direction
  .bot.left.f.ps configure -image $bm_right

  # Populate the textbox with source
  populate_text

}

# Creates and binds a single label for a cell in the treeview table
proc treelabel {tv block col} {

  global summary_list

  set f [frame $tv.[lindex $block 0]$col -background white -padx 2 -pady 2]

  switch $col {
    Total   { set type "total"  }
    Line    { set type "line"   }
    Toggle  { set type "toggle" }
    Memory  { set type "memory" }
    Logic   { set type "comb"   }
    FSM     { set type "fsm"    }
    Assert  { set type "assert" }
  }

  # Create the format string
  set fstr "(%$summary_list(max_len)d / %$summary_list(max_len)d)"

  ttk::label $f.summary -text [format $fstr $summary_list($block,$type\_hit) $summary_list($block,$type\_total)] \
                        -background $summary_list($block,$type\_color) -width [expr ($summary_list(max_len) * 2) + 5] -anchor e
  ttk::label $f.percent -text [format "%3d%%" $summary_list($block,$type\_percent)] \
                        -background $summary_list($block,$type\_color) -width 4 -anchor e

  # Set the pixel width of the frame
  set font [::ttk::style lookup [$f.summary cget -style] -font]
  $f configure -width [font measure $font [string repeat "@" [expr ($summary_list(max_len) * 2) + 5]]]

  pack $f.percent -side right
  pack $f.summary -side right -fill x

  # Create bindings
  if {$col ne "Total"} {
    bind $f.percent <Button-1> "treelabel_selected $tv $col [list $block]"
    bind $f.summary <Button-1> "treelabel_selected $tv $col [list $block]"
  }

  return $f

}

# update inplace edit widgets positions
proc update_treelabels {w} {

  global block_list curr_block cov_rb

  foreach block $block_list {
    foreach col [$w cget -columns] {
      set wnd $w.[lindex $block 0]$col
      if {[winfo exists $wnd]} {
        set bbox [$w bbox $block $col]
        if {$bbox==""} {
          place forget $wnd
        } else {
          place $wnd -x [lindex $bbox 0] -y [lindex $bbox 1] -width [lindex $bbox 2] -height [lindex $bbox 3]
        }
        if {$curr_block == $block && $cov_rb == $col} {
          $wnd configure -background black
        } else {
          $wnd configure -background white
        }
      }
    }
  }

}

# Hierarchical sorting procedure
proc sort_treeview {tree col direction {isroot 1} {root {}} } {

  if {$isroot} {
    if {$col!="#0"} {
      set col [$tree column $col -id]
    }
    set selection [$tree selection]
    $tree selection remove $selection
    set focus [$tree focus]
    $tree focus {}
  }

  # Build something we can sort
  set data {}
  if {$col=="#0"} {
    foreach row [$tree children $root] {
      lappend data [list [$tree item $row -text] $row]
    }
  } else {
    foreach row [$tree children $root] {
      lappend data [list [$tree set $row $col] $row]
    }
  }  
  if {$data!=""} {
    set dir [expr {$direction ? "-decreasing" : "-increasing"}]
    set r -1
    # Now reshuffle the rows into the sorted order
    foreach info [lsort -dictionary -index 0 $dir $data] {
      $tree move [lindex $info 1] $root [incr r]
      if {[$tree item [lindex $info 1] -open]} {
        sort_treeview $tree $col $direction 0 [lindex $info 1]
      }  
    }
  }  
  if {$isroot} {
    # Switch the heading so that it will sort in the opposite direction
    #variable curfocus
    #catch {
    #  eval [lindex [after info $curfocus($tree,sorticon)] 0]
    #  after cancel $curfocus($tree,sorticon)
    #}
    $tree heading $col -command [list sort_treeview $tree $col [expr {1-$direction}]] -image ::treeview_arrow($direction)
    $tree selection set $selection
    $tree focus $focus
  }

  # Update the treelabels
  update_treelabels $tree
  
}

proc populate_text {} {

  global cov_rb block_list curr_block summary_list
  global last_mod_inst_type mod_inst_type
  global last_block metric_src
  global start_search_index
  global curr_toggle_ptr

  if {$curr_block != ""} {

    if {$last_block != $curr_block || $last_mod_inst_type != $mod_inst_type} {

      set last_block      $curr_block
      set curr_toggle_ptr ""

      # Reset starting search index
      set start_search_index 1.0
      set curr_uncov_index   ""

      if {$cov_rb == "Line"} {
        process_line_cov
        goto_uncov $curr_uncov_index line
        $metric_src(line).h.search.e     configure -state normal -bg white
        $metric_src(line).h.search.find  configure -state normal
        $metric_src(line).h.search.clear configure -state normal
      } elseif {$cov_rb == "Toggle"} {
        process_toggle_cov
        goto_uncov $curr_uncov_index toggle
        $metric_src(toggle).h.search.e     configure -state normal -bg white
        $metric_src(toggle).h.search.find  configure -state normal
        $metric_src(toggle).h.search.clear configure -state normal
      } elseif {$cov_rb == "Memory"} {
        process_memory_cov
        goto_uncov $curr_uncov_index memory
        $metric_src(memory).h.search.e     configure -state normal -bg white
        $metric_src(memory).h.search.find  configure -state normal
        $metric_src(memory).h.search.clear configure -state normal
      } elseif {$cov_rb == "Logic"} {
        process_comb_cov
        goto_uncov $curr_uncov_index logic
        $metric_src(comb).h.search.e     configure -state normal -bg white
        $metric_src(comb).h.search.find  configure -state normal
        $metric_src(comb).h.search.clear configure -state normal
      } elseif {$cov_rb == "FSM"} {
        process_fsm_cov
        goto_uncov $curr_uncov_index fsm
        $metric_src(fsm).h.search.e     configure -state normal -bg white
        $metric_src(fsm).h.search.find  configure -state normal
        $metric_src(fsm).h.search.clear configure -state normal
      } elseif {$cov_rb == "Assert"} {
        process_assert_cov
        goto_uncov $curr_uncov_index assert
        $metric_src(assert).h.search.e     configure -state normal -bg white
        $metric_src(assert).h.search.find  configure -state normal
        $metric_src(assert).h.search.clear configure -state normal
      }

    }

  }

}

proc clear_text {} {

  global last_block metric_src

  # Clear the textboxes
  foreach metric [list line toggle memory comb fsm assert] {
    $metric_src($metric).txt configure -state normal
    $metric_src($metric).txt delete 1.0 end
    $metric_src($metric).txt configure -state disabled
  }

  # Reset the last_block
  set last_block ""

}

proc perform_search {tbox ebox ibox index} {

  upvar $index start_search_index 

  set value [$ebox get]
  set index [$tbox search $value $start_search_index]

  # Delete search_found tag
  $tbox tag delete search_found

  if {$index != ""} {

    if {$start_search_index > $index} {
      $ibox configure -text "End of file reached.  Searching from the beginning..."
    } else {
      $ibox configure -text ""
    }

    # Highlight found text
    set value_len [string length $value]
    $tbox tag add search_found $index "$index + $value_len chars"
    $tbox tag configure search_found -background orange1

    # Make the text visible
    $tbox see $index 

    # Calculate next starting index
    set indices [split $index .]
    set start_search_index [lindex $indices 0].[expr [lindex $indices 1] + 1]

  } else {

    # Output a message specifying that the searched string could not be found
    $ibox configure -text "String \"$value\" not found"

    # Clear the contents of the search entry box
    $ebox delete 0 end

    # Reset search index
    set start_search_index 1.0

  }

  # Set focus to text box
  focus $tbox

  return 1

}

proc rm_pointer {curr_ptr metric} {

  global metric_src
  upvar  $curr_ptr ptr

  # Allow the textbox to be changed
  $metric_src($metric).txt configure -state normal

  # Delete old cursor, if it is displayed
  if {$ptr != ""} {
    $metric_src($metric).txt delete $ptr.0 $ptr.3
    $metric_src($metric).txt insert $ptr.0 "   "
  }

  # Disable textbox
  $metric_src($metric).txt configure -state disabled

  # Disable "Show current selection" menu item
  .menubar.view entryconfigure 2 -state disabled

  # Clear current pointer
  set ptr ""

}

proc set_pointer {curr_ptr line metric} {

  global metric_src
  upvar  $curr_ptr ptr

  # Remove old pointer
  rm_pointer ptr $metric

  # Allow the textbox to be changed
  $metric_src($metric).txt configure -state normal

  # Display new pointer
  $metric_src($metric).txt delete $line.0 $line.3
  $metric_src($metric).txt insert $line.0 "-->"

  # Set the textbox to not be changed
  $metric_src($metric).txt configure -state disabled

  # Make sure that we can see the current toggle pointer in the textbox
  $metric_src($metric).txt see $line.0

  # Enable the "Show current selection" menu option
  .menubar.view entryconfigure 2 -state normal

  # Set the current pointer to the specified line
  set ptr $line

}

proc goto_uncov {curr_index metric} {

  global prev_uncov_index next_uncov_index curr_uncov_index
  global cov_rb metric_src

  # Calculate the name of the tag to use
  if {$cov_rb == "Line"} {
    set tag_name "uncov_colorMap"
  } else {
    set tag_name "uncov_button"
  }

  # Display the given index, if it has been specified
  if {$curr_index != ""} {
    $metric_src($metric).txt see $curr_index
    set curr_uncov_index $curr_index
  } else {
    set curr_uncov_index 1.0
  }

  # Get previous uncovered index
  set prev_uncov_index [lindex [$metric_src($metric).txt tag prevrange $tag_name $curr_uncov_index] 0]

  # Disable/enable previous button
  if {$prev_uncov_index != ""} {
    $metric_src($metric).h.pn.prev configure -state normal
    .menubar.view entryconfigure 1 -state normal
  } else {
    $metric_src($metric).h.pn.prev configure -state disabled
    .menubar.view entryconfigure 1 -state disabled
  }

  # Get next uncovered index
  set next_uncov_index [lindex [$metric_src($metric).txt tag nextrange $tag_name "$curr_uncov_index + 1 chars"] 0]

  # Disable/enable next button
  if {$next_uncov_index != ""} {
    $metric_src($metric).h.pn.next configure -state normal
    .menubar.view entryconfigure 0 -state normal
  } else {
    $metric_src($metric).h.pn.next configure -state disabled
    .menubar.view entryconfigure 0 -state disabled
  }

}

proc update_all_windows {} {

  global curr_uncov_index

  # Update the main window
  goto_uncov $curr_uncov_index
  .menubar.view entryconfigure 2 -state disabled

  # Update the toggle window
  update_toggle

  # Update the memory window
  update_memory

  # Update the combinational logic window
  update_comb

  # Update the FSM window
  update_fsm

  # Update the assertion window
  update_assert

}

proc clear_all_windows {} {

  # Clear the main window
  clear_text

  # Clear the summary window
  clear_summary

  # Clear the toggle window
  clear_toggle

  # Clear the memory window
  clear_memory

  # Clear the combinational logic window
  clear_comb

  # Clear the FSM window
  clear_fsm

  # Clear the assertion window
  clear_assert

}

proc bgerror {msg} {

  global errorInfo

  ;# Remove the status window if it exists
  destroy .status

  ;# Display error message
  set retval [tk_messageBox -message "$msg\n$errorInfo" -title "Error" -type ok]

}

proc listbox_yset {args} {
    
  eval [linsert $args 0 .bot.left.fvb.lvb set]
  listbox_yview moveto [lindex [.bot.left.fvb.lvb get] 0]

} 
  
proc listbox_yview {args} {

  eval [linsert $args 0 .bot.left.pw.ff.l   yview]
  eval [linsert $args 0 .bot.left.pw.fhmt.l yview]
  eval [linsert $args 0 .bot.left.pw.fp.l   yview]

}

proc listbox_xset {args} {

  eval [linsert $args 0 .bot.left.lhb set]
  listbox_xview moveto [lindex [.bot.left.lhb get] 0]

}

proc listbox_xview {args} {

  eval [linsert $args 0 .bot.left.pw.ff.l   xview]
  eval [linsert $args 0 .bot.left.pw.fhmt.l xview]
  eval [linsert $args 0 .bot.left.pw.fp.l   xview]

}

proc goto_next_pane {w} {
  
  set parent [winfo parent $w]
  set panes  [$parent panes]
  
  $parent paneconfigure $w -hide true
  $parent paneconfigure [lindex $panes [expr [lsearch $panes $w] + 1]] -hide false
  
}

proc goto_prev_pane {w} {
  
  set parent [winfo parent $w]
  set panes  [$parent panes]
  
  $parent paneconfigure $w -hide true
  $parent paneconfigure [lindex $panes [expr [lsearch $panes $w] - 1]] -hide false
  
}

# Read configuration file
read_coveredrc

# Display main window
main_view

# Set the theme to the clam theme
ttk::style theme use $preferences(ttk_style)

# If an input CDD(s) was specified, load them now
if {$input_cdd ne ""} {

  open_files $input_cdd

} else {

  # Display the wizard, if the configuration option is set
  if {$preferences(show_wizard)} {
    create_wizard
  }

}
