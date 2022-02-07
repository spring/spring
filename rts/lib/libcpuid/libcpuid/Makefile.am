EXTRA_DIST =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =

lib_LTLIBRARIES += libcpuid.la
libcpuid_la_LDFLAGS = \
	-export-symbols $(srcdir)/libcpuid.sym \
	-no-undefined -version-info @LIBCPUID_VERSION_INFO@
libcpuid_la_SOURCES =		\
	cpuid_main.c		\
	recog_intel.c		\
	recog_amd.c		\
	rdtsc.c			\
	asm-bits.c		\
	libcpuid_util.c		\
	rdmsr.c

if WINDOWS
libcpuid_la_SOURCES += msrdriver.c
endif

libcpuid_la_DEPENDENCIES = \
	$(srcdir)/libcpuid.sym

libcpuidincludedir = $(includedir)/libcpuid
libcpuidinclude_HEADERS = 	\
	libcpuid.h		\
	libcpuid_constants.h	\
	libcpuid_types.h

noinst_HEADERS = 		\
	amd_code_t.h		\
	asm-bits.h		\
	intel_code_t.h		\
	libcpuid_internal.h	\
	libcpuid_util.h		\
	recog_intel.h		\
	recog_amd.h		\
	rdtsc.h

EXTRA_DIST += libcpuid.sym libcpuid_vc71.vcproj libcpuid_vc10.vcxproj libcpuid_vc10.vcxproj.filters

if HAVE_DOXYGEN
directory = $(top_srcdir)/libcpuid/docs/man/man3

dist_man_MANS = $(directory)/cpu_id_t.3		\
	$(directory)/cpu_list_t.3		\
	$(directory)/cpu_mark_t.3		\
	$(directory)/cpu_raw_data_t.3		\
	$(directory)/libcpuid.3			\
	$(directory)/cpuid_tool.3
$(directory)/cpu_id_t.3: doxyfile.stamp
$(directory)/cpu_list_t.3: doxyfile.stamp
$(directory)/cpu_mark_t.3: doxyfile.stamp
$(directory)/cpu_raw_data_t.3: doxyfile.stamp
$(directory)/libcpuid.3: doxyfile.stamp
$(directory)/cpuid_tool.3: doxyfile.stamp

doxyfile.stamp:
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp

clean-local:
	rm -rf $(top_srcdir)/libcpuid/docs
endif
