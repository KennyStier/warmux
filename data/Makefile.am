xdg_appsdir = $(datadir)/applications
xdg_apps_DATA = warmux.desktop warmux_files.desktop

# Install the icon into the freedesktop hicolor theme so launchers can
# pick whichever size they need (Icon=warmux in the .desktop file).
hicolordir = $(datadir)/icons/hicolor
ICON_SIZES = 32 64 128 256 512

install-data-local:
	for s in $(ICON_SIZES); do \
	  $(MKDIR_P) "$(DESTDIR)$(hicolordir)/$${s}x$${s}/apps" && \
	  $(INSTALL_DATA) "$(srcdir)/icon/warmux_$${s}x$${s}.png" \
	    "$(DESTDIR)$(hicolordir)/$${s}x$${s}/apps/warmux.png" || exit 1; \
	done
	$(MKDIR_P) "$(DESTDIR)$(hicolordir)/scalable/apps"
	$(INSTALL_DATA) "$(srcdir)/icon/warmux.svg" \
	  "$(DESTDIR)$(hicolordir)/scalable/apps/warmux.svg"

uninstall-local:
	for s in $(ICON_SIZES); do \
	  rm -f "$(DESTDIR)$(hicolordir)/$${s}x$${s}/apps/warmux.png"; \
	done
	rm -f "$(DESTDIR)$(hicolordir)/scalable/apps/warmux.svg"

nobase_dist_pkgdata_DATA = @DATA_FILES@
pkgdatadir = @DATADIR@
EXTRA_DIST = $(xdg_apps_DATA)
