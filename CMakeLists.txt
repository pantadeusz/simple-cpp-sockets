cmake_minimum_required(VERSION 3.9)
project(tepsoc LANGUAGES CXX VERSION 0.0.1 DESCRIPTION "tepsoc provides wrapper to sockets with API similar to nodejs")

find_package(Catch2)
find_package(Threads)
FIND_PACKAGE(PkgConfig)


include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")


add_library(tepsoc SHARED src/tepsoc.cpp)
set_target_properties(tepsoc PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/tepsoc.hpp)
target_include_directories(tepsoc PRIVATE include)
install(TARGETS tepsoc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tepsoc)
install(FILES ${CMAKE_BINARY_DIR}/tepsoc.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

file(GLOB_RECURSE tests_SOURCES "${PROJECT_SOURCE_DIR}/tests/*_test.cpp" "${PROJECT_SOURCE_DIR}/tests/*/*_test.cpp")
add_executable(tests ${tests_SOURCES} "tests/tests.cpp" )
add_executable(sample_server "sample_server.cpp" )
target_link_libraries(sample_server tepsoc ${CMAKE_THREAD_LIBS_INIT}  Catch2::Catch2)
target_link_libraries(tests tepsoc ${CMAKE_THREAD_LIBS_INIT}  Catch2::Catch2)
target_link_libraries(tests ${CMAKE_THREAD_LIBS_INIT})
include_directories("${PROJECT_SOURCE_DIR}/tests" "${PROJECT_SOURCE_DIR}/include")

SET(PKG_CONFIG_LIBDIR
    "\${prefix}/lib"
)
SET(PKG_CONFIG_INCLUDEDIR
    "\${prefix}/include/tepsoc"
)
SET(PKG_CONFIG_LIBS
    "-L\${libdir} -ltepsoc"
)
SET(PKG_CONFIG_CFLAGS
    "-I\${includedir}"
)
CONFIGURE_FILE("tepsoc.pc.in" "tepsoc.pc")
# @ONLY)


if(Catch2_FOUND)
  include(CTest)
  include(Catch)
  catch_discover_tests(tests)
endif()

SET(CPACK_PACKAGE_VERSION_MAJOR ${tepsoc_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${tepsoc_VERSION_MINOR})
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_GENERATOR "DEB" "TGZ")

SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tadeusz Puźniakowski")
INCLUDE(CPack)
