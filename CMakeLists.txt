cmake_minimum_required(VERSION 3.9)
project(simplecppsockets VERSION 1.0.1 DESCRIPTION "simplecppsockets provides wrapper to sockets that allow for automatic cleanup and mocking")
include(GNUInstallDirs)
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra")
add_library(simplecppsockets SHARED src/simplecppsockets.cpp)
set_target_properties(simplecppsockets PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/simplecppsockets.hpp)
configure_file(simplecppsockets.pc.in simplecppsockets.pc @ONLY)
target_include_directories(simplecppsockets PRIVATE include)
install(TARGETS simplecppsockets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ssockets)
install(FILES ${CMAKE_BINARY_DIR}/simplecppsockets.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)


SET(CPACK_PACKAGE_VERSION_MAJOR ${simplecppsockets_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${simplecppsockets_VERSION_MINOR})
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_GENERATOR "DEB" "TGZ")

SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tadeusz Puźniakowski")
INCLUDE(CPack)
