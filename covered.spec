%define version 0.7

Summary: Verilog code coverage analyzer
Name: covered
Version: %{version}
Release: 1
License: GPL
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

/usr/local/share/covered/.coveredrc
/usr/local/share/covered/scripts/assert.tcl
/usr/local/share/covered/scripts/balloon.tcl
/usr/local/share/covered/scripts/banner.gif
/usr/local/share/covered/scripts/cdd_view.tcl
/usr/local/share/covered/scripts/checked.gif
/usr/local/share/covered/scripts/clear.gif
/usr/local/share/covered/scripts/comb.tcl
/usr/local/share/covered/scripts/cov_create.tcl
/usr/local/share/covered/scripts/cov_icon.gif
/usr/local/share/covered/scripts/exclude.tcl
/usr/local/share/covered/scripts/find.gif
/usr/local/share/covered/scripts/fsm.tcl
/usr/local/share/covered/scripts/gen_new.tcl
/usr/local/share/covered/scripts/gen_rank.tcl
/usr/local/share/covered/scripts/gen_report.tcl
/usr/local/share/covered/scripts/help.tcl
/usr/local/share/covered/scripts/left_arrow.gif
/usr/local/share/covered/scripts/main_view.tcl
/usr/local/share/covered/scripts/memory.tcl
/usr/local/share/covered/scripts/menu_create.tcl
/usr/local/share/covered/scripts/preferences.tcl
/usr/local/share/covered/scripts/process_file.tcl
/usr/local/share/covered/scripts/right_arrow.gif
/usr/local/share/covered/scripts/summary.tcl
/usr/local/share/covered/scripts/toggle.tcl
/usr/local/share/covered/scripts/unchecked.gif
/usr/local/share/covered/scripts/verilog.tcl
/usr/local/share/covered/scripts/viewer.tcl
/usr/local/share/covered/scripts/wizard.tcl
/usr/local/libexec/covered.vpi
/usr/local/libexec/covered.vcs.so
/usr/local/libexec/covered.cver.so
/usr/local/bin/covered
/usr/local/share/covered/doc/html/chapter.attr.html
/usr/local/share/covered/doc/html/chapter.boundaries.html
/usr/local/share/covered/doc/html/chapter.debug.html
/usr/local/share/covered/doc/html/chapter.epilogue.html
/usr/local/share/covered/doc/html/chapter.exclude.html
/usr/local/share/covered/doc/html/chapter.faq.html
/usr/local/share/covered/doc/html/chapter.gui.assert.html
/usr/local/share/covered/doc/html/chapter.gui.assert.source.html
/usr/local/share/covered/doc/html/chapter.gui.exclude.html
/usr/local/share/covered/doc/html/chapter.gui.fsm.html
/usr/local/share/covered/doc/html/chapter.gui.genreport.html
/usr/local/share/covered/doc/html/chapter.gui.intro.html
/usr/local/share/covered/doc/html/chapter.gui.line.html
/usr/local/share/covered/doc/html/chapter.gui.logic.html
/usr/local/share/covered/doc/html/chapter.gui.main.html
/usr/local/share/covered/doc/html/chapter.gui.memory.html
/usr/local/share/covered/doc/html/chapter.gui.new.html
/usr/local/share/covered/doc/html/chapter.gui.preferences.html
/usr/local/share/covered/doc/html/chapter.gui.rank.html
/usr/local/share/covered/doc/html/chapter.gui.toggle.html
/usr/local/share/covered/doc/html/chapter.gui.wizard.html
/usr/local/share/covered/doc/html/chapter.installation.html
/usr/local/share/covered/doc/html/chapter.intro.html
/usr/local/share/covered/doc/html/chapter.merge.html
/usr/local/share/covered/doc/html/chapter.metrics.html
/usr/local/share/covered/doc/html/chapter.race.html
/usr/local/share/covered/doc/html/chapter.rank.html
/usr/local/share/covered/doc/html/chapter.reading.html
/usr/local/share/covered/doc/html/chapter.report.html
/usr/local/share/covered/doc/html/chapter.score.html
/usr/local/share/covered/doc/html/chapter.start.html
/usr/local/share/covered/doc/html/chapter.using.html
/usr/local/share/covered/doc/html/covered.css
/usr/local/share/covered/doc/html/index.html
/usr/local/share/covered/doc/html/part.command.line.usage.html
/usr/local/share/covered/doc/html/part.epilogue.html
/usr/local/share/covered/doc/html/part.faq.html
/usr/local/share/covered/doc/html/part.gui.html
/usr/local/share/covered/doc/html/part.installation.html
/usr/local/share/covered/doc/html/part.overview.html
/usr/local/share/covered/doc/html/example.rptI.html
/usr/local/share/covered/doc/html/example.rptM.html
/usr/local/share/covered/doc/html/example.v.html
/usr/local/share/covered/doc/html/img/app_menu.gif
/usr/local/share/covered/doc/html/img/assert_excl.gif
/usr/local/share/covered/doc/html/img/assert_src.gif
/usr/local/share/covered/doc/html/img/assert_window.gif
/usr/local/share/covered/doc/html/img/banner.jpg
/usr/local/share/covered/doc/html/img/close_warn.gif
/usr/local/share/covered/doc/html/img/col_show_hide.gif
/usr/local/share/covered/doc/html/img/comb_complex.gif
/usr/local/share/covered/doc/html/img/comb_event.gif
/usr/local/share/covered/doc/html/img/comb_simple.gif
/usr/local/share/covered/doc/html/img/comb_simple_and.gif
/usr/local/share/covered/doc/html/img/comb_simple_or.gif
/usr/local/share/covered/doc/html/img/comb_unary.gif
/usr/local/share/covered/doc/html/img/dn_button.gif
/usr/local/share/covered/doc/html/img/er_full.gif
/usr/local/share/covered/doc/html/img/er_part.gif
/usr/local/share/covered/doc/html/img/exit_warn.gif
/usr/local/share/covered/doc/html/img/file_menu.gif
/usr/local/share/covered/doc/html/img/file_viewer.gif
/usr/local/share/covered/doc/html/img/fsm_example.png
/usr/local/share/covered/doc/html/img/fsm_excl.gif
/usr/local/share/covered/doc/html/img/fsm_window.gif
/usr/local/share/covered/doc/html/img/gen_menu.gif
/usr/local/share/covered/doc/html/img/help_menu.gif
/usr/local/share/covered/doc/html/img/home.gif
/usr/local/share/covered/doc/html/img/line_excl.gif
/usr/local/share/covered/doc/html/img/line_incl.gif
/usr/local/share/covered/doc/html/img/main_cov.gif
/usr/local/share/covered/doc/html/img/main_info.gif
/usr/local/share/covered/doc/html/img/main_lb.gif
/usr/local/share/covered/doc/html/img/main_menu.gif
/usr/local/share/covered/doc/html/img/main_viewer.gif
/usr/local/share/covered/doc/html/img/main_window.gif
/usr/local/share/covered/doc/html/img/memory_full.gif
/usr/local/share/covered/doc/html/img/new_cdd_define.gif
/usr/local/share/covered/doc/html/img/new_cdd_fsm.gif
/usr/local/share/covered/doc/html/img/new_cdd_gen.gif
/usr/local/share/covered/doc/html/img/new_cdd_insert_menu.gif
/usr/local/share/covered/doc/html/img/new_cdd_lib_ext.gif
/usr/local/share/covered/doc/html/img/new_cdd_mod_excl.gif
/usr/local/share/covered/doc/html/img/new_cdd_mod_gen.gif
/usr/local/share/covered/doc/html/img/new_cdd_name.gif
/usr/local/share/covered/doc/html/img/new_cdd_options.gif
/usr/local/share/covered/doc/html/img/new_cdd_options2.gif
/usr/local/share/covered/doc/html/img/new_cdd_parm_oride.gif
/usr/local/share/covered/doc/html/img/new_cdd_selection.gif
/usr/local/share/covered/doc/html/img/new_cdd_type.gif
/usr/local/share/covered/doc/html/img/next.gif
/usr/local/share/covered/doc/html/img/note.gif
/usr/local/share/covered/doc/html/img/populated_lb.gif
/usr/local/share/covered/doc/html/img/pref_color.gif
/usr/local/share/covered/doc/html/img/pref_exclude.gif
/usr/local/share/covered/doc/html/img/pref_goals.gif
/usr/local/share/covered/doc/html/img/pref_main.gif
/usr/local/share/covered/doc/html/img/pref_merge.gif
/usr/local/share/covered/doc/html/img/pref_syntax.gif
/usr/local/share/covered/doc/html/img/prev.gif
/usr/local/share/covered/doc/html/img/rank_files.gif
/usr/local/share/covered/doc/html/img/rank_options.gif
/usr/local/share/covered/doc/html/img/rank_output.gif
/usr/local/share/covered/doc/html/img/rank_report.gif
/usr/local/share/covered/doc/html/img/rank_selection.gif
/usr/local/share/covered/doc/html/img/report_menu.gif
/usr/local/share/covered/doc/html/img/rpt_gen_view.gif
/usr/local/share/covered/doc/html/img/rpt_gen_selection.gif
/usr/local/share/covered/doc/html/img/rpt_gen_options.gif
/usr/local/share/covered/doc/html/img/toggle_full.gif
/usr/local/share/covered/doc/html/img/up.gif
/usr/local/share/covered/doc/html/img/up_button.gif
/usr/local/share/covered/doc/html/img/vhier.png
/usr/local/share/covered/doc/html/img/vhier2.png
/usr/local/share/covered/doc/html/img/view_menu.gif
/usr/local/share/covered/doc/html/img/wizard.gif
/usr/local/man/man1/covered.1

%changelog

