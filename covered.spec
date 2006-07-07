Summary: Verilog code coverage analyzer
Name: covered
Version: %{version}
Release: 1
Copyright: GPL
Group: Applications/Engineering
Source: covered-%{version}.tar.gz
BuildRoot: /tmp/%{name}-buildroot
URL: http://covered.sourceforge.net
Prefix: /usr/local
Provides: covered

%description
Covered is a Verilog code-coverage utility using VCD/LXT style dumpfiles and the
design to generate line, toggle, combinational logic and FSM state/arc coverage
reports. Covered also contains a built-in race condition checker and GUI report viewer.

%prep
%setup

%build
./configure --prefix=$RPM_BUILD_ROOT/usr/local
make

%install
rm -rf $RPM_BUILD_ROOT
make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc README TODO COPYING ChangeLog
%defattr(-,root,root)

/usr/local/share/covered/.coveredrc
/usr/local/share/covered/scripts/banner.gif
/usr/local/share/covered/scripts/cdd_view.tcl
/usr/local/share/covered/scripts/comb.tcl
/usr/local/share/covered/scripts/cov_create.tcl
/usr/local/share/covered/scripts/help.tcl
/usr/local/share/covered/scripts/main_view.tcl
/usr/local/share/covered/scripts/menu_create.tcl
/usr/local/share/covered/scripts/preferences.tcl
/usr/local/share/covered/scripts/process_file.tcl
/usr/local/share/covered/scripts/summary.tcl
/usr/local/share/covered/scripts/toggle.tcl
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
/usr/local/share/covered/doc/user/banner.gif
/usr/local/share/covered/doc/user/example.rptI.html
/usr/local/share/covered/doc/user/example.rptM.html
/usr/local/share/covered/doc/user/example.v.html
/usr/local/share/covered/doc/user/index.html
/usr/local/share/covered/doc/user/styles.css
/usr/local/man/man1/covered.1
/usr/local/share/covered/doc/gui/comb.html
/usr/local/share/covered/doc/gui/cov_bar.html
/usr/local/share/covered/doc/gui/cov_view.html
/usr/local/share/covered/doc/gui/file_menu.html
/usr/local/share/covered/doc/gui/help_menu.html
/usr/local/share/covered/doc/gui/info_bar.html
/usr/local/share/covered/doc/gui/intro.html
/usr/local/share/covered/doc/gui/line.html
/usr/local/share/covered/doc/gui/main.html
/usr/local/share/covered/doc/gui/main_menu.html
/usr/local/share/covered/doc/gui/mod_inst_lb.html
/usr/local/share/covered/doc/gui/preferences.html
/usr/local/share/covered/doc/gui/report_menu.html
/usr/local/share/covered/doc/gui/summary.html
/usr/local/share/covered/doc/gui/summary_bar.html
/usr/local/share/covered/doc/gui/toggle.html
/usr/local/share/covered/doc/gui/view_menu.html
/usr/local/share/covered/doc/gui/welcome.html
/usr/local/share/covered/doc/gui/images/banner.gif
/usr/local/share/covered/doc/gui/images/color.png
/usr/local/share/covered/doc/gui/images/comb_complex.png
/usr/local/share/covered/doc/gui/images/comb_event.png
/usr/local/share/covered/doc/gui/images/comb_simple_and.png
/usr/local/share/covered/doc/gui/images/comb_simple_or.png
/usr/local/share/covered/doc/gui/images/comb_simple.png
/usr/local/share/covered/doc/gui/images/comb_unary.png
/usr/local/share/covered/doc/gui/images/dn_button.png
/usr/local/share/covered/doc/gui/images/file_menu.png
/usr/local/share/covered/doc/gui/images/help_menu.png
/usr/local/share/covered/doc/gui/images/main_cov.png
/usr/local/share/covered/doc/gui/images/main_info.png
/usr/local/share/covered/doc/gui/images/main_lb.png
/usr/local/share/covered/doc/gui/images/main_menu.png
/usr/local/share/covered/doc/gui/images/main_summary.png
/usr/local/share/covered/doc/gui/images/main_viewer.png
/usr/local/share/covered/doc/gui/images/main_window.png
/usr/local/share/covered/doc/gui/images/open_cdd.png
/usr/local/share/covered/doc/gui/images/populated_lb.png
/usr/local/share/covered/doc/gui/images/pref_window.png
/usr/local/share/covered/doc/gui/images/report_menu.png
/usr/local/share/covered/doc/gui/images/summary_window.png
/usr/local/share/covered/doc/gui/images/toggle_full.png
/usr/local/share/covered/doc/gui/images/up_button.png
/usr/local/share/covered/doc/gui/images/view_menu.png

%changelog

