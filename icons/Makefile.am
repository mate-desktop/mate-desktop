NULL =

public_icons_themes = \
	hicolor \
	$(NULL)

png_icons = \
	hicolor_apps_16x16_mate-desktop.png \
	hicolor_apps_22x22_mate-desktop.png \
	hicolor_apps_24x24_mate-desktop.png \
	hicolor_apps_32x32_mate-desktop.png \
	hicolor_apps_48x48_mate-desktop.png \
	hicolor_apps_128x128_mate-desktop.png \
	$(NULL)

public_icons = \
	$(png_icons) \
	hicolor_apps_scalable_mate-desktop.svg \
	hicolor_apps_scalable_mate-desktop-symbolic.svg \
	$(NULL)

hicolor_apps_%.png: hicolor_apps_scalable_mate-desktop.svg
	$(RSVG_CONVERT) \
		-w $(word 1, $(subst x, ,$(word 1,$(subst _, ,$*)))) \
		-h $(word 2, $(subst x, ,$(word 1,$(subst _, ,$*)))) \
		$^ -o $@

EXTRA_DIST = \
	$(public_icons)	\
	$(NULL)

###############################################################################

install-icons: $(public_icons)
	for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

uninstall-icons:
	-for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

install-data-local: install-icons

uninstall-local: uninstall-icons

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache: $(public_icons)
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

.PHONY: clean-png-icons
clean-png-icons:
	rm -f $(png_icons)
