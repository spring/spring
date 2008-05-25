######################################################
# You should not need to change anything in this file
# See the Makefile.common file
######################################################

include Makefile.common
include Makefile.libm_objects

ifdef MINGDIR
EXE_SUFFIX=.exe
# heuristic: don't set RM to del on MSYS/CYGWIN shells
# doesn't work probably because scons doesn't pass all env vars
#ifndef BASH
#RM=del
#endif
else
EXE_SUFFIX=
endif

ifdef STREFLOP_SOFT
USE_SOFT_BINARY=SoftFloatWrapperSimple.o SoftFloatWrapperDouble.o SoftFloatWrapperExtended.o softfloat/softfloat.o
else
USE_SOFT_BINARY=
endif

TARGETS = libm/flt-target
LIBM_OBJECTS = $(flt-32-objects)

all: streflop.a

Random.o: Random.cpp Random.h Makefile FPUSettings.h SMath.h streflop.h
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) Random.cpp -o Random.o

SMath.o: SMath.cpp SMath.h Makefile FPUSettings.h streflop.h
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) SMath.cpp -o SMath.o

SoftFloatWrapperSimple.o: SoftFloatWrapper.cpp SoftFloatWrapper.h Makefile FPUSettings.h streflop.h
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -DN_SPECIALIZED=32 SoftFloatWrapper.cpp -o $@

SoftFloatWrapperDouble.o: SoftFloatWrapper.cpp SoftFloatWrapper.h Makefile FPUSettings.h streflop.h
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -DN_SPECIALIZED=64 SoftFloatWrapper.cpp -o $@

SoftFloatWrapperExtended.o: SoftFloatWrapper.cpp SoftFloatWrapper.h Makefile FPUSettings.h streflop.h
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -DN_SPECIALIZED=96 SoftFloatWrapper.cpp -o $@

streflop.a: SMath.o Random.o ${USE_SOFT_BINARY}
	$(MAKE) -C libm
	$(RM) streflop.a
# just link twice, there are too much issues in attempting a platform independent copy...
# (or rather, detecting the platform)
ifeq (i586-mingw32msvc-gcc,$(findstring i586-mingw32msvc-gcc,$(CC)))
	i586-mingw32msvc-ar rs streflop.a $(LIBM_OBJECTS) SMath.o Random.o ${USE_SOFT_BINARY}
	i586-mingw32msvc-ar rs libstreflop.a $(LIBM_OBJECTS) SMath.o Random.o ${USE_SOFT_BINARY}
else
	$(AR) rs streflop.a $(LIBM_OBJECTS) SMath.o Random.o ${USE_SOFT_BINARY}
	$(AR) rs libstreflop.a $(LIBM_OBJECTS) SMath.o Random.o ${USE_SOFT_BINARY}
endif

arithmeticTest$(EXE_SUFFIX): arithmeticTest.cpp streflop.a
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) arithmeticTest.cpp streflop.a -o $@

randomTest$(EXE_SUFFIX): randomTest.cpp streflop.a
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) randomTest.cpp streflop.a -o $@

.PHONY : clean package
clean:
	-rm -f *.o streflop.a libstreflop.a arithmeticTest$(EXE_SUFFIX) randomTest$(EXE_SUFFIX) ${USE_SOFT_BINARY}
	$(MAKE) -C libm clean


# Prepare source files, so it's possible to make a package even when the directory is cluttered

LIBM_STREFLOP = libm/import.pl libm/Makefile libm/streflop_libm_bridge.h libm/README.txt libm/e_expf.c libm/w_expf.c

SOFTFLOAT_STREFLOP = softfloat/milieu.h softfloat/softfloat.h softfloat/SoftFloat-README.txt softfloat/SoftFloat.txt softfloat/README.txt softfloat/SoftFloat-history.txt softfloat/SoftFloat-source.txt softfloat/softfloat.cpp softfloat/softfloat-macros softfloat/softfloat-specialize

BASE_STREFLOP = arithmeticTest.cpp randomTest.cpp FPUSettings.h IntegerTypes.h LGPL.txt Makefile Makefile.common Makefile.libm_objects SMath.cpp SMath.h Random.cpp Random.h README.txt SoftFloatWrapper.cpp SoftFloatWrapper.h streflop.h System.h X87DenormalSquasher.h

# Tar only once for both archive formats
package:
	@echo "preparing temporary subdir streflop-$(STREFLOP_VERSION)"
	@rm -rf streflop-$(STREFLOP_VERSION)
	@mkdir streflop-$(STREFLOP_VERSION)
	@echo "use tar to create subdirs"
	@tar cf - $(BASE_STREFLOP) $(SOFTFLOAT_STREFLOP) $(LIBM_STREFLOP) $(libm-src) | tar xf - -C streflop-$(STREFLOP_VERSION)
	@echo $(STREFLOP_VERSION) > streflop-$(STREFLOP_VERSION)/VERSION.txt
	@echo "use tar again to create package, with full directory name included"
	@tar cf streflop-$(STREFLOP_VERSION).tar streflop-$(STREFLOP_VERSION)
	@echo "remove temporary directory"
	@rm -rf streflop-$(STREFLOP_VERSION)
	@echo "make best gzip archive"
	@cat streflop-$(STREFLOP_VERSION).tar | gzip --best > streflop-$(STREFLOP_VERSION).tar.gz
	@echo "convert tar to bz2"
	@bzip2 -f streflop-$(STREFLOP_VERSION).tar


# debug rules

arithmeticTest_x870$(EXE_SUFFIX): arithmeticTest.cpp
	$(CXX) $(CXXFLAGS) -DSTREFLOP_X87=1 $(LDFLAGS) arithmeticTest.cpp streflop_x870.a -o $@

arithmeticTest_soft0$(EXE_SUFFIX): arithmeticTest.cpp
	$(CXX) $(CXXFLAGS) -DSTREFLOP_SOFT=1 $(LDFLAGS) arithmeticTest.cpp streflop_soft0.a -o $@

%.hex : %.bin
	hexdump -C $< > $@

