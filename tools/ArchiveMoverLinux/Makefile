ML_SOURCES = fileSystem.ml sdz.ml sd7.ml archive.ml datadir.ml spring_installer.ml
ML_SEARCH = -I +lablgtk2  -I +zip -I +extlib -I libmlsevenzip
ML_LIBS = unix.cmxa sevenzip.cmxa zip.cmxa extLib.cmxa lablgtk.cmxa gtkInit.cmx

all: spring-installer

spring-installer: libmlsevenzip $(ML_SOURCES)
	ocamlopt -o spring-installer $(ML_SEARCH) $(ML_LIBS) $(ML_SOURCES)

libmlsevenzip:
	$(MAKE) -C libmlsevenzip

clean:
	$(MAKE) -C libmlsevenzip clean
	rm -f *.cmi  *.cmo  *.cmx  *.o spring-installer

install:
	install -m 755 spring-installer $(DESTDIR)/usr/bin/spring-installer
 	install -m 644 debian/spring-installer.desktop $(DESTDIR)/usr/share/applications/spring-installer.desktop

.PHONY: all clean libmlsevenzip
