#windowlist.tcl: provides routines for managing windows from menu, i.e. minimize, raise, bring all to front; standard menu item on Mac OS X. 

#(c) 2010 WordTech Communications LLC. License: standard Tcl license, http://www.tcl.tk/software/tcltk/license.html

#includes code from http://wiki.tcl.tk/1461

##"cycle through windows" code courtesy of Tom Hennigan, tomhennigan@gmail.com, (c) 2009

package provide windowlist 1.3

namespace eval windowlist {

    #make the window menu
    proc windowMenu {mainmenu} {

	menu $mainmenu.windowlist

	$mainmenu.windowlist add command -label "Minimize" -command [namespace current]::minimizeFrontWindow -accelerator Command-M
	$mainmenu.windowlist add separator
	$mainmenu.windowlist add command -label "Bring All to Front" -command [namespace current]::raiseAllWindows
	$mainmenu.windowlist add separator
	$mainmenu.windowlist add command -label "Cycle Through Windows" \
	    -command  {raise [lindex [wm stackorder .] 0]} \
	    -accelerator "Command-`"
       	bind all <Command-quoteleft> {raise [lindex [wm stackorder .] 0]}
	$mainmenu.windowlist add separator
	$mainmenu.windowlist add command -label "Main Window" -command ::tk::mac::ReopenApplication
	$mainmenu.windowlist add separator
	$mainmenu.windowlist add command -label [wm title .] -command ::tk::mac::ReopenApplication
	
	$mainmenu add cascade -label "Window" -menu $mainmenu.windowlist
	
        #bind the window menu to update whenever a new window is added, on menu selection
       	bind all <<MenuSelect>> +[list [namespace current]::updateWindowMenu $mainmenu.windowlist]
	bind all <Command-M>  [namespace current]::minimizeFrontWindow
	bind all <Command-m>  [namespace current]::minimizeFrontWindow
	bind . <Command-w> {wm state . withdrawn}
	bind .  <Command-W> {wm state . withdrawn}
	wm protocol . WM_DELETE_WINDOW {wm state . withdrawn}

    }

    
    #update the window menu with windows
    proc updateWindowMenu {windowmenu} {

	set windowlist [wm stackorder .]

	#search for drawer window first
	if {[lsearch $windowlist ".drawer"] >= 0 } {
	    set windowlist [lreplace $windowlist [lsearch $windowlist ".drawer"] [lsearch $windowlist ".drawer"]]
	    update
	}
	
	if {$windowlist == {}} {
	    return
	}

	$windowmenu delete 8 end
	foreach item $windowlist {
	    $windowmenu add command -label "[wm title $item]"  -command [list raise $item]

	}
    }



    #make all windows visible
    proc raiseAllWindows {} {

	#get list of mapped windows
	set windowlist [wm stackorder .]

	#do nothing if all windows are minimized
	if {$windowlist == {}} {
	    return
	}

	#use [winfo children .] here to get windows that are minimized
	foreach item [winfo children .] {
	    
	    #get all toplevel windows, exclude menubar windows
	    if { [string equal [winfo toplevel $item] $item] && [catch {$item cget -tearoff}]} {
		wm deiconify $item
	    }
	}
	#be sure to deiconify ., since the above command only gets the child toplevels
	wm deiconify .
    }

    #minimize the selected window
    proc minimizeFrontWindow {} {

	#get list of mapped windows
	set windowlist [wm stackorder .]

	#do nothing if all windows are minimized
	if {$windowlist == {}} {
	    return
	} else {

	    #minimize topmost window
	    set topwindow [lindex $windowlist end]
	    wm iconify $topwindow

	}
    }
    
    namespace export *
}

#raise window if closed--dock click
proc ::tk::mac::ReopenApplication {} {

    if { [wm state .] == "withdrawn"} {
	wm state . normal
	raise .
    } else {
	wm deiconify .
	raise .
    }

}






