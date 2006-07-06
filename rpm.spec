Summary: Verilog code coverage analyzer
Name: covered
Version: 0.4.4
Release: 1
Copyright: GPL
Group: Applications/Engineering
Source: http://prdownloads.sourceforge.net/covered/covered-0.4.4.tar.gz?download
BuildRoot: /tmp/%{name}-buildroot

%description
Covered is a Verilog code-coverage utility using VCD/LXT style dumpfiles and the
design to generate line, toggle, combinational logic and FSM state/arc coverage
reports. Covered also contains a built-in race condition checker and GUI report viewer.

%prep
%setup

%build
make

%install
make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc README TODO COPYING ChangeLog
%defattr(-,root,root)

/usr/local/share/covered/.coveredrc
/usr/local/share/covered/scripts/assert.tcl
/usr/local/share/covered/scripts/banner.gif
/usr/local/share/covered/scripts/cdd_view.tcl
/usr/local/share/covered/scripts/comb.tcl
/usr/local/share/covered/scripts/cov_create.tcl
/usr/local/share/covered/scripts/fsm.tcl
/usr/local/share/covered/scripts/help.tcl
/usr/local/share/covered/scripts/main_view.tcl
/usr/local/share/covered/scripts/menu_create.tcl
/usr/local/share/covered/scripts/preferences.tcl
/usr/local/share/covered/scripts/process_file.tcl
/usr/local/share/covered/scripts/summary.tcl
/usr/local/share/covered/scripts/toggle.tcl
/usr/local/share/covered/scripts/verilog.tcl
/usr/local/bin/covered
/usr/local/share/covered/doc/user/001.html
/usr/local/share/covered/doc/user/002.html
/usr/local/share/covered/doc/user/003.html
/usr/local/share/covered/doc/user/004.html
/usr/local/share/covered/doc/user/005.html
/usr/local/share/covered/doc/user/006.html
/usr/local/share/covered/doc/user/007.html
/usr/local/share/covered/doc/user/008.html
/usr/local/share/covered/doc/user/009.html
/usr/local/share/covered/doc/user/010.html
/usr/local/share/covered/doc/user/011.html
/usr/local/share/covered/doc/user/012.html
/usr/local/share/covered/doc/user/013.html
/usr/local/share/covered/doc/user/014.html
/usr/local/share/covered/doc/user/015.html
/usr/local/share/covered/doc/user/016.html
/usr/local/share/covered/doc/user/017.html
/usr/local/share/covered/doc/user/018.html
/usr/local/share/covered/doc/user/019.html
/usr/local/share/covered/doc/user/020.html
/usr/local/share/covered/doc/user/021.html
/usr/local/share/covered/doc/user/022.html
/usr/local/share/covered/doc/user/023.html
/usr/local/share/covered/doc/user/024.html
/usr/local/share/covered/doc/user/025.html
/usr/local/share/covered/doc/user/banner.gif
/usr/local/share/covered/doc/user/example.rptI.html
/usr/local/share/covered/doc/user/example.rptM.html
/usr/local/share/covered/doc/user/example.v.html
/usr/local/share/covered/doc/user/index.html
/usr/local/share/covered/doc/user/styles.css
/usr/local/man/man1/covered.1
/usr/local/share/covered/doc/gui/assert.html
/usr/local/share/covered/doc/gui/comb.html
/usr/local/share/covered/doc/gui/cov_bar.html
/usr/local/share/covered/doc/gui/cov_view.html
/usr/local/share/covered/doc/gui/file_menu.html
/usr/local/share/covered/doc/gui/fsm.html
/usr/local/share/covered/doc/gui/help_menu.html
/usr/local/share/covered/doc/gui/info_bar.html
/usr/local/share/covered/doc/gui/intro.html
/usr/local/share/covered/doc/gui/line.html
/usr/local/share/covered/doc/gui/main.html
/usr/local/share/covered/doc/gui/main_menu.html
/usr/local/share/covered/doc/gui/mod_inst_lb.html
/usr/local/share/covered/doc/gui/pref_color.html
/usr/local/share/covered/doc/gui/pref_goals.html
/usr/local/share/covered/doc/gui/pref_main.html
/usr/local/share/covered/doc/gui/pref_report.html
/usr/local/share/covered/doc/gui/pref_syntax.html
/usr/local/share/covered/doc/gui/report_gen.html
/usr/local/share/covered/doc/gui/report_menu.html
/usr/local/share/covered/doc/gui/src_view.html
/usr/local/share/covered/doc/gui/summary.html
/usr/local/share/covered/doc/gui/summary_bar.html
/usr/local/share/covered/doc/gui/toggle.html
/usr/local/share/covered/doc/gui/view_menu.html
/usr/local/share/covered/doc/gui/welcome.html
/usr/local/share/covered/doc/gui/images/assert_excl.png
/usr/local/share/covered/doc/gui/images/assert_window.png
/usr/local/share/covered/doc/gui/images/assert_src.png
/usr/local/share/covered/doc/gui/images/banner.gif
/usr/local/share/covered/doc/gui/images/close_warn.png
/usr/local/share/covered/doc/gui/images/color.png
/usr/local/share/covered/doc/gui/images/comb_complex.png
/usr/local/share/covered/doc/gui/images/comb_event.png
/usr/local/share/covered/doc/gui/images/comb_simple_and.png
/usr/local/share/covered/doc/gui/images/comb_simple_or.png
/usr/local/share/covered/doc/gui/images/comb_simple.png
/usr/local/share/covered/doc/gui/images/comb_unary.png
/usr/local/share/covered/doc/gui/images/dn_button.png
/usr/local/share/covered/doc/gui/images/exit_warn.png
/usr/local/share/covered/doc/gui/images/file_menu.png
/usr/local/share/covered/doc/gui/images/file_viewer.png
/usr/local/share/covered/doc/gui/images/fsm_excl.png
/usr/local/share/covered/doc/gui/images/fsm_window.png
/usr/local/share/covered/doc/gui/images/gen_menu.png
/usr/local/share/covered/doc/gui/images/help_menu.png
/usr/local/share/covered/doc/gui/images/line_excl.png
/usr/local/share/covered/doc/gui/images/line_incl.png
/usr/local/share/covered/doc/gui/images/main_cov.png
/usr/local/share/covered/doc/gui/images/main_info.png
/usr/local/share/covered/doc/gui/images/main_lb.png
/usr/local/share/covered/doc/gui/images/main_menu.png
/usr/local/share/covered/doc/gui/images/main_summary.png
/usr/local/share/covered/doc/gui/images/main_viewer.png
/usr/local/share/covered/doc/gui/images/main_window.png
/usr/local/share/covered/doc/gui/images/open_cdd.png
/usr/local/share/covered/doc/gui/images/populated_lb.png
/usr/local/share/covered/doc/gui/images/pref_main.png
/usr/local/share/covered/doc/gui/images/pref_color.png
/usr/local/share/covered/doc/gui/images/pref_goals.png
/usr/local/share/covered/doc/gui/images/pref_syntax.png
/usr/local/share/covered/doc/gui/images/pref_report.png
/usr/local/share/covered/doc/gui/images/report_gen.png
/usr/local/share/covered/doc/gui/images/report_menu.png
/usr/local/share/covered/doc/gui/images/summary_window.png
/usr/local/share/covered/doc/gui/images/toggle_full.png
/usr/local/share/covered/doc/gui/images/up_button.png
/usr/local/share/covered/doc/gui/images/view_menu.png

%changelog

