ML_SOURCES = fileSystem.ml sdz.ml sd7.ml archive.ml datadir.ml mover.ml
ML_SEARCH = -I +lablgtk2  -I +zip -I +extlib -I libmlsevenzip
ML_LIBS = unix.cmxa sevenzip.cmxa zip.cmxa extLib.cmxa lablgtk.cmxa gtkInit.cmx

all: mover

mover: $(ML_SOURCES) libmlsevenzip
	ocamlopt -o mover $(ML_SEARCH) $(ML_LIBS) $(ML_SOURCES)

libmlsevenzip:
	$(MAKE) -C libmlsevenzip

clean:
	$(MAKE) -C libmlsevenzip clean
	rm -f *.cmi  *.cmo  *.cmx  *.o mover


.PHONY: all clean libmlsevenzip
