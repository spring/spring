ML_SOURCES = mover.ml
ML_SEARCH = -I +lablgtk2  -I +zip -I +extlib
ML_LIBS = lablgtk.cmxa gtkInit.cmx unix.cmxa zip.cmxa extLib.cmxa

all: mover

mover: $(ML_SOURCES)
	ocamlopt -o mover $(ML_SEARCH) $(ML_LIBS) $(ML_SOURCES)

clean:
	rm -f *.cmi  *.cmo  *.cmx  *.o mover


.PHONY: all clean
