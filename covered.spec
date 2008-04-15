%define version 0.7

Summary: Verilog code coverage analyzer
Name: covered
Version: %{version}
Release: 1
Copyright: GPL
Group: Applications/Engineering
Source: covered-%{version}.tar.gz
BuildRoot: /tmp/%{name}-buildroot
URL: http://covered.sourceforge.net
Provides: covered
Requires: tcl >= 8.4
Requires: tk >= 8.4

%description
Covered is a Verilog code-coverage utility using VCD/LXT style dumpfiles and the
design to generate line, toggle, memory, combinational logic, FSM state/arc and
assertion coverage reports. Covered also contains a built-in race condition checker
and GUI report viewer.

%prep
%setup

%build
./configure --with-iv=/usr/local --with-cver=/usr/local/include/cver --with-vcs=$VCS_HOME/include
make

%install
rm -rf $RPM_BUILD_ROOT
make install prefix=$RPM_BUILD_ROOT/usr/local

%clean
make clean

%files
%defattr(-,root,root)

/usr/local/share/covered/.coveredrc'
/usr/local/share/covered/scripts/assert.tcl'
/usr/local/share/covered/scripts/banner.gif'
/usr/local/share/covered/scripts/cdd_view.tcl'
/usr/local/share/covered/scripts/comb.tcl'
/usr/local/share/covered/scripts/cov_create.tcl'
/usr/local/share/covered/scripts/cov_icon.gif'
/usr/local/share/covered/scripts/fsm.tcl'
/usr/local/share/covered/scripts/help.tcl'
/usr/local/share/covered/scripts/help_wrapper.tcl'
/usr/local/share/covered/scripts/help_tbl.tcl'
/usr/local/share/covered/scripts/left_arrow.gif'
/usr/local/share/covered/scripts/main_view.tcl'
/usr/local/share/covered/scripts/memory.tcl'
/usr/local/share/covered/scripts/menu_create.tcl'
/usr/local/share/covered/scripts/preferences.tcl'
/usr/local/share/covered/scripts/process_file.tcl'
/usr/local/share/covered/scripts/right_arrow.gif'
/usr/local/share/covered/scripts/summary.tcl'
/usr/local/share/covered/scripts/toggle.tcl'
/usr/local/share/covered/scripts/verilog.tcl'
/usr/local/libexec/covered.vpi'
/usr/local/libexec/covered.vcs.so'
/usr/local/libexec/covered.cver.so'
/usr/local/bin/covered'
/usr/local/share/covered/doc/user/001.html'
/usr/local/share/covered/doc/user/002.html'
/usr/local/share/covered/doc/user/003.html'
/usr/local/share/covered/doc/user/004.html'
/usr/local/share/covered/doc/user/005.html'
/usr/local/share/covered/doc/user/006.html'
/usr/local/share/covered/doc/user/007.html'
/usr/local/share/covered/doc/user/008.html'
/usr/local/share/covered/doc/user/009.html'
/usr/local/share/covered/doc/user/010.html'
/usr/local/share/covered/doc/user/011.html'
/usr/local/share/covered/doc/user/012.html'
/usr/local/share/covered/doc/user/013.html'
/usr/local/share/covered/doc/user/014.html'
/usr/local/share/covered/doc/user/015.html'
/usr/local/share/covered/doc/user/016.html'
/usr/local/share/covered/doc/user/017.html'
/usr/local/share/covered/doc/user/018.html'
/usr/local/share/covered/doc/user/019.html'
/usr/local/share/covered/doc/user/020.html'
/usr/local/share/covered/doc/user/021.html'
/usr/local/share/covered/doc/user/022.html'
/usr/local/share/covered/doc/user/023.html'
/usr/local/share/covered/doc/user/024.html'
/usr/local/share/covered/doc/user/025.html'
/usr/local/share/covered/doc/user/026.html'
/usr/local/share/covered/doc/user/027.html'
/usr/local/share/covered/doc/user/028.html'
/usr/local/share/covered/doc/user/029.html'
/usr/local/share/covered/doc/user/030.html'
/usr/local/share/covered/doc/user/banner.gif'
/usr/local/share/covered/doc/user/example.rptI.html'
/usr/local/share/covered/doc/user/example.rptM.html'
/usr/local/share/covered/doc/user/example.v.html'
/usr/local/share/covered/doc/user/index.html'
/usr/local/share/covered/doc/user/styles.css'
/usr/local/share/covered/doc/user/img/000000.gif'
/usr/local/share/covered/doc/user/img/12568c.gif'
/usr/local/share/covered/doc/user/img/butprev.png'
/usr/local/share/covered/doc/user/img/fsm_example.png'
/usr/local/share/covered/doc/user/img/tile.jpeg'
/usr/local/share/covered/doc/user/img/toolbar_cont.png'
/usr/local/share/covered/doc/user/img/toolbar_first.png'
/usr/local/share/covered/doc/user/img/toolbar_home.png'
/usr/local/share/covered/doc/user/img/toolbar_last.png'
/usr/local/share/covered/doc/user/img/toolbar_left.png'
/usr/local/share/covered/doc/user/img/toolbar_next.png'
/usr/local/share/covered/doc/user/img/toolbar_prev.png'
/usr/local/share/covered/doc/user/img/toolbar_right.png'
/usr/local/share/covered/doc/user/img/vhier.png'
/usr/local/man/man1/covered.1'
/usr/local/share/covered/doc/gui/assert.help'
/usr/local/share/covered/doc/gui/comb.help'
/usr/local/share/covered/doc/gui/cov_bar.help'
/usr/local/share/covered/doc/gui/cov_view.help'
/usr/local/share/covered/doc/gui/file_menu.help'
/usr/local/share/covered/doc/gui/fsm.help'
/usr/local/share/covered/doc/gui/help.help'
/usr/local/share/covered/doc/gui/help_menu.help'
/usr/local/share/covered/doc/gui/info_bar.help'
/usr/local/share/covered/doc/gui/intro.help'
/usr/local/share/covered/doc/gui/line.help'
/usr/local/share/covered/doc/gui/main.help'
/usr/local/share/covered/doc/gui/main_menu.help'
/usr/local/share/covered/doc/gui/memory.help'
/usr/local/share/covered/doc/gui/mod_inst_lb.help'
/usr/local/share/covered/doc/gui/pref_color.help'
/usr/local/share/covered/doc/gui/pref_goals.help'
/usr/local/share/covered/doc/gui/pref_main.help'
/usr/local/share/covered/doc/gui/pref_report.help'
/usr/local/share/covered/doc/gui/pref_syntax.help'
/usr/local/share/covered/doc/gui/report_gen.help'
/usr/local/share/covered/doc/gui/report_menu.help'
/usr/local/share/covered/doc/gui/src_view.help'
/usr/local/share/covered/doc/gui/summary.help'
/usr/local/share/covered/doc/gui/summary_bar.help'
/usr/local/share/covered/doc/gui/toggle.help'
/usr/local/share/covered/doc/gui/view_menu.help'
/usr/local/share/covered/doc/gui/images/assert_excl.gif'
/usr/local/share/covered/doc/gui/images/assert_window.gif'
/usr/local/share/covered/doc/gui/images/assert_src.gif'
/usr/local/share/covered/doc/gui/images/banner.gif'
/usr/local/share/covered/doc/gui/images/close_warn.gif'
/usr/local/share/covered/doc/gui/images/color.gif'
/usr/local/share/covered/doc/gui/images/comb_complex.gif'
/usr/local/share/covered/doc/gui/images/comb_event.gif'
/usr/local/share/covered/doc/gui/images/comb_simple_and.gif'
/usr/local/share/covered/doc/gui/images/comb_simple_or.gif'
/usr/local/share/covered/doc/gui/images/comb_simple.gif'
/usr/local/share/covered/doc/gui/images/comb_unary.gif'
/usr/local/share/covered/doc/gui/images/dn_button.gif'
/usr/local/share/covered/doc/gui/images/exit_warn.gif'
/usr/local/share/covered/doc/gui/images/file_menu.gif'
/usr/local/share/covered/doc/gui/images/file_viewer.gif'
/usr/local/share/covered/doc/gui/images/fsm_excl.gif'
/usr/local/share/covered/doc/gui/images/fsm_window.gif'
/usr/local/share/covered/doc/gui/images/gen_menu.gif'
/usr/local/share/covered/doc/gui/images/help_menu.gif'
/usr/local/share/covered/doc/gui/images/line_excl.gif'
/usr/local/share/covered/doc/gui/images/line_incl.gif'
/usr/local/share/covered/doc/gui/images/main_cov.gif'
/usr/local/share/covered/doc/gui/images/main_info.gif'
/usr/local/share/covered/doc/gui/images/main_lb.gif'
/usr/local/share/covered/doc/gui/images/main_menu.gif'
/usr/local/share/covered/doc/gui/images/main_summary.gif'
/usr/local/share/covered/doc/gui/images/main_viewer.gif'
/usr/local/share/covered/doc/gui/images/main_window.gif'
/usr/local/share/covered/doc/gui/images/memory_full.gif'
/usr/local/share/covered/doc/gui/images/open_cdd.gif'
/usr/local/share/covered/doc/gui/images/populated_lb.gif'
/usr/local/share/covered/doc/gui/images/pref_main.gif'
/usr/local/share/covered/doc/gui/images/pref_color.gif'
/usr/local/share/covered/doc/gui/images/pref_goals.gif'
/usr/local/share/covered/doc/gui/images/pref_syntax.gif'
/usr/local/share/covered/doc/gui/images/pref_report.gif'
/usr/local/share/covered/doc/gui/images/report_gen.gif'
/usr/local/share/covered/doc/gui/images/report_menu.gif'
/usr/local/share/covered/doc/gui/images/summary_window.gif'
/usr/local/share/covered/doc/gui/images/toggle_full.gif'
/usr/local/share/covered/doc/gui/images/up_button.gif'
/usr/local/share/covered/doc/gui/images/view_menu.gif'

%changelog

