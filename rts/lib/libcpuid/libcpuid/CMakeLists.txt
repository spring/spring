set(cpuid_sources
    cpuid_main.c
    recog_intel.c
    recog_amd.c
    rdtsc.c
    libcpuid_util.c
    rdmsr.c
    asm-bits.c)

if(WIN32)
   list(APPEND cpuid_sources msrdriver.c)
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fvisibility=hidden ${PIC_FLAG} -Wall")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PIC_FLAG}")

if("${MSVC_CXX_ARCHITECTURE_ID}" MATCHES "x64")
  list(APPEND cpuid_sources masm-x64.asm)
endif()

add_library(cpuid ${cpuid_sources})
set_property(TARGET cpuid PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_include_directories(cpuid SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_compile_definitions(cpuid PRIVATE VERSION="${PROJECT_VERSION}")
set_target_properties(cpuid PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(cpuid PROPERTIES SOVERSION ${SOVERSION})
set_target_properties(cpuid PROPERTIES PUBLIC_HEADER "libcpuid.h;libcpuid_types.h;libcpuid_constants.h")

# Documentation
find_package(Doxygen)
option(ENABLE_DOCS "Enable building documentation." ON)
if(DOXYGEN_FOUND AND ENABLE_DOCS)
  set(top_srcdir ${PROJECT_SOURCE_DIR})
  configure_file(Doxyfile.in Doxyfile ESCAPE_QUOTES)
  add_custom_target(
    docs ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
  if(UNIX)
    install(
      DIRECTORY "${CMAKE_SOURCE_DIR}/docs/man/"
      DESTINATION "${CMAKE_INSTALL_MANDIR}"
      FILES_MATCHING
      PATTERN "cpu_*_t.3"
      PATTERN "libcpuid.3"
      PATTERN "cpuid_tool.3")
  endif(UNIX)
else()
  add_custom_target(
    docs
    echo
    "Doxygen was not installed when CMake was run or ENABLE_DOCS was OFF. Check that Doxygen is installed and rerun `cmake .`"
    VERBATIM)
endif()

# Configuration
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake' Use: * PROJECT_VERSION
write_basic_package_version_file("${version_config}" COMPATIBILITY SameMajorVersion)

# Configure '<PROJECT-NAME>Config.cmake' Use variables: * TARGETS_EXPORT_NAME * PROJECT_NAME
configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in" "${project_config}"
                              INSTALL_DESTINATION "${config_install_dir}")

# Installation

install(
  TARGETS cpuid
  EXPORT "${TARGETS_EXPORT_NAME}"
  LIBRARY DESTINATION "lib"
  ARCHIVE DESTINATION "lib"
  RUNTIME DESTINATION "bin"
  INCLUDES DESTINATION "include/libcpuid"
  PUBLIC_HEADER DESTINATION "include/libcpuid")

install(
  EXPORT "${TARGETS_EXPORT_NAME}"
  NAMESPACE "${namespace}"
  DESTINATION "${config_install_dir}")

install(FILES "${project_config}" "${version_config}" DESTINATION "${config_install_dir}")

add_custom_target(
  consistency
  COMMAND "./check-consistency.py" "./"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMENT "Check consistency"
  VERBATIM)
