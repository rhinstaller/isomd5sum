cmake_minimum_required(VERSION 2.6)
project(isomd5sum)

if(NOT DEFINED BINDIR)
  set(BINDIR bin)
endif()

if(MINGW)
  if(NOT DEFINED LIBDIR)
    set(LIBDIR lib)
  endif()
  if(NOT DEFINED DATADIR)
    set(DATADIR ${LIBDIR})
  endif()
else()
  if(NOT DEFINED LIBDIR)
    # Provides CMAKE_INSTALL_LIBDIR.
    include(GNUInstallDirs)
    set(LIBDIR ${CMAKE_INSTALL_LIBDIR})
  endif()
  if(NOT DEFINED DATADIR)
    set(DATADIR share)
  endif()
endif()

execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/scripts/version.sh
  OUTPUT_VARIABLE VERSION)
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -std=gnu11 -Wall -fPIC")

configure_file(${CMAKE_PROJECT_NAME}.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.pc @ONLY)

include_directories(.)

add_library(libimplantisomd5 STATIC
  src/md5.c src/utilities.c src/libimplantisomd5.c
  include/md5.h include/utilities.h include/libimplantisomd5.h)
set_target_properties(libimplantisomd5 PROPERTIES
  OUTPUT_NAME implantisomd5
  PUBLIC_HEADER include/libimplantisomd5.h)

add_library(libcheckisomd5 STATIC
  src/md5.c src/utilities.c src/libcheckisomd5.c
  include/md5.h include/utilities.h include/libcheckisomd5.h)
set_target_properties(libcheckisomd5 PROPERTIES
  OUTPUT_NAME checkisomd5
  PUBLIC_HEADER include/libcheckisomd5.h)

# Check if popt exists because it doesn't exist in mingw.
find_path(POPT_INCLUDE_DIR NAMES popt.h)
find_library(POPT_LIBRARY NAMES popt)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(popt DEFAULT_MSG
  POPT_LIBRARY POPT_INCLUDE_DIR)
if(popt_FOUND)
  add_executable(implantisomd5 src/implantisomd5.c)
  target_link_libraries(implantisomd5 libimplantisomd5 popt)

  add_executable(checkisomd5 src/checkisomd5.c)
  target_link_libraries(checkisomd5 libcheckisomd5 popt)

  install(TARGETS implantisomd5 checkisomd5
    EXPORT bin
    RUNTIME DESTINATION ${BINDIR})

  install(FILES ${CMAKE_SOURCE_DIR}/man/implantisomd5.1
    ${CMAKE_SOURCE_DIR}/man/checkisomd5.1
    DESTINATION ${DATADIR}/man/man1)
endif()

find_package(PythonLibs ${PYTHON_VERSION})
if(PythonLibs_FOUND)
  include_directories(${PYTHON_INCLUDE_DIRS})
  # FIXME(squimrel): Library should be named pyisomd5sum.so insteead of libpyisomd5sum.so.
  add_library(pyisomd5sum SHARED src/pyisomd5sum.c)
  set_target_properties(pyisomd5sum PROPERTIES LIBRARY_OUTPUT_NAME pyisomd5sum)
  target_link_libraries(pyisomd5sum libimplantisomd5 libcheckisomd5)

  find_package(PythonInterp ${PYTHON_VERSION})
  if(PythonInterp_FOUND)
    execute_process(COMMAND
      ${PYTHON_EXECUTABLE} -c "\
import sys\n\
import site\n\
sitepackages = site.getsitepackages()[0][1:]\n\
if sitepackages.startswith('usr/'):\n\
  sitepackages = sitepackages.split('/', 1)[1]\n\
sys.stdout.write(sitepackages)"
      OUTPUT_VARIABLE PYTHON_SITE_PACKAGES)

    install(TARGETS pyisomd5sum
      EXPORT python
      LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PYTHON_SITE_PACKAGES}
      RUNTIME DESTINATION ${BINDIR})
  endif()
endif()

install(TARGETS libimplantisomd5 libcheckisomd5
  EXPORT devel
  ARCHIVE DESTINATION ${LIBDIR}
  PUBLIC_HEADER DESTINATION include)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.pc
  DESTINATION ${DATADIR}/pkgconfig)
