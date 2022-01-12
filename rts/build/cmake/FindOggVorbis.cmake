# Downloaded from: http://websvn.kde.org/trunk/KDE/kdelibs/cmake/modules/
# License: see the accompanying COPYING-CMAKE-SCRIPTS file
#
# Modifications:
# 2008.01.16 Tobi Vollebregt -- Moved ${OGG_LIBRARY} to the back of OGGVORBIS_LIBRARIES,
#                               this allows vorbis to link to ogg on MinGW.
#                            -- Moved ${VORBIS_LIBRARY} just before OGGVORBIS_LIBRARIES,
#                               this allows vorbis{file,enc} to link to vorbis on MinGW.

# - Try to find the OggVorbis libraries
# Once done this will define
#
#  OGGVORBIS_FOUND - system has OggVorbis
#  OGGVORBIS_VERSION - set either to 1 or 2
#  OGGVORBIS_INCLUDE_DIR - the OggVorbis include directory
#  OGGVORBIS_LIBRARIES - The libraries needed to use OggVorbis
#  OGG_LIBRARY         - The Ogg library
#  VORBIS_LIBRARY      - The Vorbis library
#  VORBISFILE_LIBRARY  - The VorbisFile library
#  VORBISENC_LIBRARY   - The VorbisEnc library

# Copyright (c) 2006, Richard Laerkaeng, <richard@goteborg.utfors.se>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


include (CheckLibraryExists)

if (VORBIS_INCLUDE_DIR AND OGG_INCLUDE_DIR AND OGG_LIBRARY AND VORBIS_LIBRARY AND VORBISFILE_LIBRARY)
	set (OggVorbis_FIND_QUIETLY TRUE)
endif ()

find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h)
find_path(OGG_INCLUDE_DIR ogg/ogg.h)

find_library(OGG_LIBRARY NAMES ogg ogg-0)
find_library(VORBIS_LIBRARY NAMES vorbis vorbis-0)
find_library(VORBISFILE_LIBRARY NAMES vorbisfile vorbisfile-3)
find_library(VORBISENC_LIBRARY NAMES vorbisenc vorbisenc-2)


if (VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY AND VORBISFILE_LIBRARY AND VORBISENC_LIBRARY)
   set(OGGVORBIS_FOUND TRUE)

   set(OGGVORBIS_LIBRARIES ${OGG_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBISENC_LIBRARY})

   set(_CMAKE_REQUIRED_LIBRARIES_TMP ${CMAKE_REQUIRED_LIBRARIES})
   set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${OGGVORBIS_LIBRARIES})
   check_library_exists(vorbis vorbis_bitrate_addblock "" HAVE_LIBVORBISENC2)
   set(CMAKE_REQUIRED_LIBRARIES ${_CMAKE_REQUIRED_LIBRARIES_TMP})

   if (HAVE_LIBVORBISENC2)
      set (OGGVORBIS_VERSION 2)
   else ()
      set (OGGVORBIS_VERSION 1)
   endif ()

else ()
   set (OGGVORBIS_VERSION)
   set(OGGVORBIS_FOUND FALSE)
endif ()


if (OGGVORBIS_FOUND)
   if (NOT OggVorbis_FIND_QUIETLY)
      message (STATUS "Found OggVorbis: ${OGGVORBIS_LIBRARIES}")
   endif ()
else ()
   if (OggVorbis_FIND_REQUIRED)
      message (FATAL_ERROR "Could NOT find OggVorbis libraries")
   endif ()
   if (NOT OggVorbis_FIND_QUITELY)
      message (STATUS "Could NOT find OggVorbis libraries")
   endif ()
endif ()

mark_as_advanced(VORBIS_INCLUDE_DIR OGG_INCLUDE_DIR OGG_LIBRARY VORBIS_LIBRARY VORBISFILE_LIBRARY VORBISENC_LIBRARY)

#check_include_files(vorbis/vorbisfile.h HAVE_VORBISFILE_H)
#check_library_exists(ogg ogg_page_version "" HAVE_LIBOGG)
#check_library_exists(vorbis vorbis_info_init "" HAVE_LIBVORBIS)
#check_library_exists(vorbisfile ov_open "" HAVE_LIBVORBISFILE)
#check_library_exists(vorbisenc vorbis_info_clear "" HAVE_LIBVORBISENC)
#check_library_exists(vorbis vorbis_bitrate_addblock "" HAVE_LIBVORBISENC2)

#if (HAVE_LIBOGG AND HAVE_VORBISFILE_H AND HAVE_LIBVORBIS AND HAVE_LIBVORBISFILE AND HAVE_LIBVORBISENC)
#    message (STATUS "Ogg/Vorbis found")
#    set (VORBIS_LIBS "-lvorbis -logg")
#    set (VORBISFILE_LIBS "-lvorbisfile")
#    set (VORBISENC_LIBS "-lvorbisenc")
#    set (OGGVORBIS_FOUND TRUE)
#    if (HAVE_LIBVORBISENC2)
#        set (HAVE_VORBIS 2)
#    else ()
#        set (HAVE_VORBIS 1)
#    endif ()
#else ()
#    message (STATUS "Ogg/Vorbis not found")
#endif ()

