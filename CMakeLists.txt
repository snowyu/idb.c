CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(iDB)
set(PACKAGE_NAME idb)
set(PACKAGE_VERSION 0.1)
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "riceball.lee@gmail.com")
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
  message(FATAL_ERROR "In-source builds are not allowed.
CMake would overwrite the makefiles distributed with iDB.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif()

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(GNUC 1)
    SET(CMAKE_REQUIRED_FLAGS "-fms-extensions --std=gnu99")
    ADD_DEFINITIONS("-fms-extensions --std=gnu99")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

add_subdirectory(src)
#add_subdirectory(bin)

#SET(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
#SET(CMAKE_INSTALL_PREFIX /usr/local)
 
#INSTALL(PROGRAMS idb_benchmark DESTINATION bin)
 
#INSTALL(FILES COPYRIGHT README DESTINATION share/doc/idb)
 
#INSTALL(DIRECTORY doc/ DESTINATION share/doc/idb)
