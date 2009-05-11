SUBDIRS = docbook

docdir   = $(datadir)/covered/doc/html
doc_DATA = html/chapter.attr.html \
           html/chapter.boundaries.html \
           html/chapter.debug.html \
           html/chapter.epilogue.html \
           html/chapter.exclude.html \
           html/chapter.faq.html \
           html/chapter.gui.assert.html \
           html/chapter.gui.assert.source.html \
           html/chapter.gui.exclude.html \
           html/chapter.gui.fsm.html \
           html/chapter.gui.genreport.html \
           html/chapter.gui.intro.html \
           html/chapter.gui.line.html \
           html/chapter.gui.logic.html \
           html/chapter.gui.main.html \
           html/chapter.gui.memory.html \
           html/chapter.gui.new.html \
           html/chapter.gui.preferences.html \
           html/chapter.gui.rank.html \
           html/chapter.gui.toggle.html \
           html/chapter.gui.wizard.html \
           html/chapter.installation.html \
           html/chapter.intro.html \
           html/chapter.merge.html \
           html/chapter.metrics.html \
           html/chapter.race.html \
           html/chapter.rank.html \
           html/chapter.reading.html \
           html/chapter.report.html \
           html/chapter.score.html \
           html/chapter.start.html \
           html/chapter.using.html \
           html/covered.css \
           html/index.html \
           html/part.command.line.usage.html \
           html/part.epilogue.html \
           html/part.faq.html \
           html/part.gui.html \
           html/part.installation.html \
           html/part.overview.html \
           html/test/example.rptI.html \
           html/test/example.rptM.html \
           html/test/example.v.html

imgdir   = $(datadir)/covered/doc/html/img
img_DATA = html/img/app_menu.gif \
           html/img/assert_excl.gif \
           html/img/assert_src.gif \
           html/img/assert_window.gif \
           html/img/banner.jpg \
           html/img/close_warn.gif \
           html/img/col_show_hide.gif \
           html/img/comb_complex.gif \
           html/img/comb_event.gif \
           html/img/comb_simple.gif \
           html/img/comb_simple_and.gif \
           html/img/comb_simple_or.gif \
           html/img/comb_unary.gif \
           html/img/dn_button.gif \
           html/img/er_full.gif \
           html/img/er_part.gif \
           html/img/exit_warn.gif \
           html/img/file_menu.gif \
           html/img/file_viewer.gif \
           html/img/fsm_example.png \
           html/img/fsm_excl.gif \
           html/img/fsm_window.gif \
           html/img/gen_menu.gif \
           html/img/help_menu.gif \
           html/img/home.gif \
           html/img/line_excl.gif \
           html/img/line_incl.gif \
           html/img/main_cov.gif \
           html/img/main_info.gif \
           html/img/main_lb.gif \
           html/img/main_menu.gif \
           html/img/main_viewer.gif \
           html/img/main_window.gif \
           html/img/memory_full.gif \
           html/img/new_cdd_define.gif \
           html/img/new_cdd_fsm.gif \
           html/img/new_cdd_gen.gif \
           html/img/new_cdd_insert_menu.gif \
           html/img/new_cdd_lib_ext.gif \
           html/img/new_cdd_mod_excl.gif \
           html/img/new_cdd_mod_gen.gif \
           html/img/new_cdd_name.gif \
           html/img/new_cdd_options.gif \
           html/img/new_cdd_options2.gif \
           html/img/new_cdd_parm_oride.gif \
           html/img/new_cdd_selection.gif \
           html/img/new_cdd_type.gif \
           html/img/next.gif \
           html/img/note.gif \
           html/img/populated_lb.gif \
           html/img/pref_color.gif \
           html/img/pref_exclude.gif \
           html/img/pref_goals.gif \
           html/img/pref_main.gif \
           html/img/pref_merge.gif \
           html/img/pref_syntax.gif \
           html/img/prev.gif \
           html/img/rank_files.gif \
           html/img/rank_options.gif \
           html/img/rank_output.gif \
           html/img/rank_report.gif \
           html/img/rank_selection.gif \
           html/img/report_menu.gif \
           html/img/rpt_gen_view.gif \
           html/img/rpt_gen_selection.gif \
           html/img/rpt_gen_options.gif \
           html/img/toggle_full.gif \
           html/img/up.gif \
           html/img/up_button.gif \
           html/img/vhier.png \
           html/img/vhier2.png \
           html/img/view_menu.gif \
           html/img/wizard.gif
man_MANS = covered.1

user_doc:   
	cd docbook; $(MAKE)
           
devel_doc:
	-doxygen covered.dox

devel_doc_clean:
	rm -rf devel/html devel/latex
