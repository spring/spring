# common definitions
include ../Makefile.common

ifdef MINGDIR
TOUCH=echo >
else
TOUCH=touch
endif

TARGETS = flt-target dbl-target
ifndef STREFLOP_SSE
TARGETS += ldbl-target
endif

all: $(TARGETS)

flt-target:
	$(MAKE) -C flt-32
	$(TOUCH) flt-target

dbl-target:
	$(MAKE) -C dbl-64
	$(TOUCH) dbl-target

ldbl-target:
	$(MAKE) -C ldbl-96
	$(TOUCH) ldbl-target

clean:
	rm -f flt-32/*.o dbl-64/*.o ldbl-96/*.o flt-target dbl-target ldbl-target

clean-import:
	rm -rf flt-32 dbl-64 ldbl-96 headers
