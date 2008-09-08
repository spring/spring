LD_FLAGS = -Llibmlsevenzip
LIBS = -lml_sevenzip

all: libml_sevenzip.a sevenzip.cmxa

sevenzip.cmxa: sevenzip.cmx
	ocamlopt -cclib "$(LD_FLAGS) $(LIBS)" -a -o sevenzip.cmxa sevenzip.cmx

sevenzip.cmx: sevenzip.ml
	ocamlopt -o sevenzip.cmx -c sevenzip.ml

libsevenzip:
	$(MAKE) -C libsevenzip/Archive/7z -f makefile.gcc

libml_sevenzip.a: libsevenzip ml_sevenzip.o
	ar rc libml_sevenzip.a ml_sevenzip.o libsevenzip/Archive/7z/*.o
	ranlib libml_sevenzip.a

ml_sevenzip.o: ml_sevenzip.c
	ocamlopt -o ml_sevenzip.o ml_sevenzip.c

clean:
	$(MAKE) -C libsevenzip/Archive/7z -f makefile.gcc clean
	rm -rf *.cmx
	rm -rf *.cmi
	rm -rf *.o
	rm -rf *.a
	rm -rf *.cmxa

.PHONY: all clean libsevenzip
