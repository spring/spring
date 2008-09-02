CFLAGS = -I../../../rts
LD_FLAGS = -Llibmlsevenzip
LIBS = -lml_sevenzip

all: sevenzip.cmxa libml_sevenzip.a 

sevenzip.cmxa: sevenzip.cmx
	ocamlopt -cclib "$(LD_FLAGS) $(LIBS)" -a -o sevenzip.cmxa sevenzip.cmx

sevenzip.cmx: sevenzip.ml
	ocamlopt -o sevenzip.cmx -c sevenzip.ml

libsevenzip:
	$(MAKE) -C libsevenzip/Archive/7z -f makefile.gcc

libml_sevenzip.a: ml_sevenzip.o libsevenzip
	ar rc libml_sevenzip.a ml_sevenzip.o libsevenzip/Archive/7z/*.o
	ranlib libml_sevenzip.a

ml_sevenzip.o: ml_sevenzip.c
	ocamlopt -ccopt "$(CFLAGS)" -o ml_sevenzip.o ml_sevenzip.c

clean:
	$(MAKE) -C libsevenzip/Archive/7z -f makefile.gcc clean
	rm -rf *.cmx
	rm -rf *.cmi
	rm -rf *.o
	rm -rf *.a
	rm -rf *.cmxa
	rm -f test


.PHONY: all clean libsevenzip
