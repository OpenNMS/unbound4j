cmake_minimum_required (VERSION 2.6.4)
project (unbound4j)

# JNI header name
set (JNI_HEADER "unbound4j_java_interface.h")

# Output folder
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/../dist")

# Java JNI headers
find_package(JNI REQUIRED)
message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
include_directories(${JNI_INCLUDE_DIRS})

# libunbound
INCLUDE(FindPkgConfig)
pkg_check_modules (LIBUNBOUND REQUIRED libunbound>=1.8.0)
message (STATUS "LIBUNBOUND_LIBRARIES=${LIBUNBOUND_LIBRARIES}")
message (STATUS "LIBUNBOUND_LIBRARY_DIRS=${LIBUNBOUND_LIBRARY_DIRS}")
if (NOT "${LIBUNBOUND_LIBRARY_DIRS}" STREQUAL "")
  link_directories(${LIBUNBOUND_LIBRARY_DIRS})
endif()
message (STATUS "LIBUNBOUND_INCLUDE_DIRS=${LIBUNBOUND_INCLUDE_DIRS}")
if (NOT "${LIBUNBOUND_INCLUDE_DIRS}" STREQUAL "")
  include_directories(${LIBUNBOUND_INCLUDE_DIRS})
endif()

# Generic includes
INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES (getopt.h HAVE_GETOPT_H)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h")

# Turn all warnings into errors
IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# Build the shared library
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(unbound4j MODULE src/unbound4j_jinterface.c src/sldns.c src/jniutils.c src/unbound4j.c)

IF(APPLE)
	SET_TARGET_PROPERTIES(unbound4j PROPERTIES PREFIX "lib" SUFFIX ".jnilib" INSTALL_NAME_DIR "/usr/local/lib")
ELSEIF(WIN32)
	SET_TARGET_PROPERTIES(unbound4j PROPERTIES PREFIX "" SUFFIX ".dll")
ENDIF(APPLE)

target_link_libraries(unbound4j unbound)

# Main
add_executable(unbound4j_main src/main.c src/sldns.c src/unbound4j.c)
target_link_libraries(unbound4j_main unbound)
target_link_libraries(unbound4j_main pthread)

