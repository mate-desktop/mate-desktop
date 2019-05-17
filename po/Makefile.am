DOMAIN = $(PACKAGE_NAME)
COPYRIGHT_HOLDER = MATE Developers
MSGID_BUGS_ADDRESS = https://github.com/mate-desktop/mate-desktop/issues

LANGS := $(shell cat $(srcdir)/LINGUAS )
# LANGS := \
# 	af am an ar as ast az be bg bn bn_IN br bs ca ca@valencia cmn crh \
# 	cs cy da de de_CH dz el en_AU en_CA en_GB eo es es_AR es_CL \
# 	es_CO es_MX et eu fa fi fr frp fur fy ga gl gu he hi hr hu hy ia \
# 	id ig is it ja jv ka kk kn ko ku ku_IQ ky li lo lt lv mai mg mi \
# 	mk ml mn mr ms nb nds ne nl nn nso oc or pa pl ps pt pt_BR ro ru \
# 	sc si sk sl sq sr sr@latin sv ta te th tk tr tt ug uk ur uz vi wa \
# 	xh yi yo zh_CN zh_HK zh_TW zu

POTFILE_DEPS := $(shell $(SED) 's,^,$(top_srcdir)/,' $(srcdir)/POTFILES)
POTFILE := $(srcdir)/$(DOMAIN).pot
POFILES := $(LANGS:%=$(srcdir)/%.po)
GMOFILES := $(LANGS:%=$(srcdir)/%.gmo)

MAINTAINERCLEANFILES = $(POTFILE) $(POFILES) $(GMOFILES)

EXTRA_DIST = \
	POTFILES \
	$(POTFILE) \
	$(POFILES) \
	$(GMOFILES)

if HAVE_GNU_GETTEXT_TOOLS

XGETTEXT_ARGS = \
	--default-domain=$(DOMAIN) \
	--from-code=utf-8 \
	--add-comments=TRANSLATORS: \
        --keyword=_ --keyword=N_ \
	--copyright-holder='$(COPYRIGHT_HOLDER)' \
	--package-name="$(PACKAGE_NAME)" \
	--package-version="$(PACKAGE_VERSION)" \
	--msgid-bugs-address="$(MSGID_BUGS_ADDRESS)" \
	--directory=$(top_srcdir) \
	$(NULL)

SED_PO_FIXUP_ARGS = \
       -e "s|text/plain; charset=CHARSET|text/plain; charset=UTF-8|g" \
       -e "s|SOME DESCRIPTIVE TITLE|MATE Desktop package strings|g" \
       -e "s|Copyright (C) YEAR|Copyright (C) $$(date +'%Y')|" \
       -e "/^\#, fuzzy$$/d" \
       $(NULL)

update-po: $(POFILES)

update-gmo: $(GMOFILES)

update-mini-po: $(POTFILE)
	for lang in $(LANGS); do \
	  echo "Minimizing $$lang content" && \
	  $(MSGMERGE) --no-location --no-fuzzy-matching --sort-output \
	    $$lang.po $(POTFILE) | \
	  $(SED) $(SED_PO_FIXUP_ARGS) | \
	  $(PERL) $(top_srcdir)/utils/minimize-po.pl > \
	    $(srcdir)/$$lang.mini.po ; \
	done

push-pot: $(POTFILE)
	tx push --source

pull-po: $(POTFILE)
	parallel tx pull -f -l {} ::: $(LANGS)
	$(MAKE) update-mini-po

$(POTFILE): POTFILES $(POTFILE_DEPS)
	$(XGETTEXT) -o $@-t $(XGETTEXT_ARGS) \
	  --files-from=$(abs_srcdir)/POTFILES
	$(SED) $(SED_PO_FIXUP_ARGS) < $@-t > $@
	rm -f $@-t

$(srcdir)/%.po: $(srcdir)/%.mini.po $(POTFILE)
	$(MSGMERGE) --no-fuzzy-matching $< $(POTFILE) | \
	  $(SED) $(SED_PO_FIXUP_ARGS) > $@

$(srcdir)/%.gmo: $(srcdir)/%.po
	rm -f $@ $@-t
	$(MSGFMT) -c -o $@-t $<
	mv $@-t $@

.PRECIOUS: $(POTFILE) $(POFILES)

endif HAVE_GNU_GETTEXT_TOOLS

if ENABLE_NLS

# Cannot use 'localedir' since this conflicts with autoconf.
langinstdir = $(datadir)/locale

install-data-hook: $(GMOFILES)
	mkdir -p $(DESTDIR)$(langinstdir)
	for lang in $(LANGS); do \
	  d=$(DESTDIR)$(langinstdir)/$$lang/LC_MESSAGES; \
	  mkdir -p $$d; \
	  install -m 0644 $(srcdir)/$$lang.gmo $$d/$(DOMAIN).mo; \
	done

uninstall-hook:
	for lang in $(LANGS); do \
	  d=$(DESTDIR)$(langinstdir)/$$lang/LC_MESSAGES; \
	  rm -f $$d/$(DOMAIN).mo; \
	done

endif ENABLE_NLS
