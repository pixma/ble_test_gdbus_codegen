cmake_minimum_required(VERSION 3.0.0)
project(ble_test_codegen VERSION 0.1.0)

# Project version #########################################################
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

set(CMAKE_VERBOSE_MAKEFILE ON)

# A short message.
MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# require C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# C standard to ~2011 becasue of Cmake 3.18.4
set(CMAKE_C_STANDARD 11)

#Find packages which are required.
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE (PkgConfig REQUIRED)
PKG_CHECK_MODULES (GLIB2 REQUIRED glib-2.0>=2.40)
PKG_CHECK_MODULES (GIO REQUIRED gio-2.0>=2.40)
PKG_CHECK_MODULES (GIO-UNIX REQUIRED gio-unix-2.0>=2.40)
PKG_CHECK_MODULES (LIBCONFIG_PLUSPLUS libconfig++ REQUIRED)

#Add Flags
add_definitions(${GLIB2_CFLAGS_OTHER})
add_definitions(${GIO_CFLAGS_OTHER})
add_definitions(${GIO-UNIX_CFLAGS_OTHER})

#Include directories
include_directories(include)
include_directories(  
  ${GLIB2_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}  
  ${GIO-UNIX_INCLUDE_DIRS}
)

file(GLOB SOURCES "src/*.cpp" "src/*.c")

include_directories("/usr/local/include")
link_directories("/usr/local/lib")

include(CTest)
enable_testing()

add_executable(ble_test_codegen ${SOURCES})

target_link_libraries(
    ${PROJECT_NAME} 
    ${CMAKE_THREAD_LIBS_INIT}
    ${GLIB2_LIBRARIES}
    ${GIO_LIBRARIES}
    ${GIO-UNIX_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} pthread)     # For pthread library linking.
target_link_libraries(${PROJECT_NAME} config++)  # For config lib.

# Change Binary Output directory to output folder.
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output")

print_all_variables()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
