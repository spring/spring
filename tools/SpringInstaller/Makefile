SI_ML_SOURCES = hex.ml fileSystem.ml sdz.ml sd7.ml archive.ml datadir.ml springInstaller.ml
SI_ML_SEARCH = -I +lablgtk2  -I +zip -I +extlib -I libmlsevenzip
SI_ML_LIBS = unix.cmxa sevenzip.cmxa zip.cmxa extLib.cmxa lablgtk.cmxa gtkInit.cmx

CI_ML_SOURCES = hex.ml fileSystem.ml datadir.ml sdz.ml store.ml download.ml caInstaller.ml
CI_ML_SEARCH = -I +zip -I +lablgtk2 -I +netsys -I +pcre -I +equeue -I +equeue-gtk2 -I +netstring -I +extlib -I +netclient
CI_ML_LIBS = unix.cmxa threads.cmxa zip.cmxa extLib.cmxa lablgtk.cmxa gtkInit.cmx gtkThread.cmx netsys.cmxa pcre.cmxa equeue.cmxa netstring.cmxa netclient.cmxa

all: spring-installer ca-installer

spring-installer: libmlsevenzip $(SI_ML_SOURCES)
	ocamlopt -o spring-installer $(SI_ML_SEARCH) $(SI_ML_LIBS) $(SI_ML_SOURCES)

libmlsevenzip:
	$(MAKE) -C libmlsevenzip

install_spring-installer:
	install -m 755 spring-installer $(DESTDIR)/usr/bin/spring-installer
	install -m 644 debian/spring-installer.desktop $(DESTDIR)/usr/share/applications/spring-installer.desktop

install_ca-installer:
	install -m 755 ca-installer $(DESTDIR)/usr/bin/ca-installer
	install -m 644 debian/ca-installer.desktop $(DESTDIR)/usr/share/applications/ca-installer.desktop
	install -m 644 debian/ca-installer.png $(DESTDIR)/usr/share/icons/hicolor/128x128/apps/ca-installer.png

ca-installer: $(CI_ML_SOURCES)
	ocamlopt -thread -o ca-installer $(CI_ML_SEARCH) $(CI_ML_LIBS) $(CI_ML_SOURCES)

clean:
	$(MAKE) -C libmlsevenzip clean
	rm -f *.cmi  *.cmo  *.cmx  *.o spring-installer ca-installer

install: install_spring-installer install_ca-installer

.PHONY: all clean libmlsevenzip install install_spring-installer install_ca-installer
