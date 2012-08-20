
## Note, the `CACHE ...` prefix is needed
## else CMakeLists.txt overrides our given values!

SET(PREFER_STATIC_LIBS TRUE CACHE BOOL "")

SET(BUILD_spring-headless  FALSE CACHE BOOL "")
SET(BUILD_spring-dedicated FALSE CACHE BOOL "")
SET(AI_TYPES "NONE" CACHE STRING "")

SET(CMAKE_INSTALL_PREFIX "" CACHE STRING "")
SET(BINDIR  "./" CACHE STRING "")
SET(LIBDIR  "./" CACHE STRING "")
SET(DATADIR "./" CACHE STRING "")
SET(MANDIR  "share/man" CACHE STRING "")
SET(DOCDIR  "share/doc/spring" CACHE STRING "")

#SET(APPLICATIONS_DIR "share/applications" CACHE STRING "")
#SET(PIXMAPS_DIR      "share/pixmaps" CACHE STRING "")
#SET(MIME_DIR         "share/mime" CACHE STRING "")
