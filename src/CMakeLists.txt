#AUX_SOURCE_DIRECTORY(. SRC_LIST)
FILE(GLOB SRC_LIST "*.c")

INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)

Include(CheckStructHasMember)
CHECK_STRUCT_HAS_MEMBER("struct dirent" d_namlen dirent.h HAVE_STRUCT_DIRENT_D_NAMLEN)
CHECK_STRUCT_HAS_MEMBER("struct dirent" d_type dirent.h HAVE_STRUCT_DIRENT_D_TYPE)

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(GNUC 1)
    SET(CMAKE_REQUIRED_FLAGS "-fms-extensions --std=c99")
    ADD_DEFINITIONS("-fms-extensions --std=c99")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# - Check if given C source compiles and links into an executable
# CHECK_C_SOURCE_COMPILES(<code> <var> [FAIL_REGEX <fail-regex>])
#  <code>       - source code to try to compile, must define 'main'
#  <var>        - variable to store whether the source code compiled
#  <fail-regex> - fail if test output matches this regex
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
#  CMAKE_REQUIRED_LIBRARIES = list of libraries to link
INCLUDE(CheckCSourceCompiles)

CHECK_C_SOURCE_COMPILES("#define _GNU_SOURCE

struct a{ int i1;};
struct b{ struct a; int i2;};

int main(int argc, char * argv[]) {
 struct b h;
 h.i1=2;
}
" _ANONYMOUS_STRUCT)

#configure_file(src/deps/config.h.in config.h)
configure_file (
  "${PROJECT_SOURCE_DIR}/src/deps/config.h.in"
  "${PROJECT_SOURCE_DIR}/src/deps/config.h"
  )

#message("INCLUDE_DIRECTORIES:${PROJECT_SOURCE_DIR}/src")
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/src")

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
add_library(idb STATIC ${SRC_LIST}
        deps/sds.c
        deps/zmalloc.c
        deps/utf8proc.c
    )

#file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#make hello executable program (in windows would be hello.exe)
#link_directories(${PROJECT_BINARY_DIR}/idb)
ADD_EXECUTABLE(idb_benchmark benchmark.c)
target_link_libraries(idb_benchmark idb)

