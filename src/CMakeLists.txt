cmake_minimum_required(VERSION 3.14.3 FATAL_ERROR)

project(duoplot CXX)

set(REPO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    add_compile_definitions(PLATFORM_LINUX_M)
elseif(APPLE)
    set(PLATFORM_APPLE TRUE)
    add_compile_definitions(PLATFORM_APPLE_M)
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_compile_definitions(PLATFORM_WINDOWS_M)
else()
    message(FATAL_ERROR "Unknown platform!" )
endif()

if(PLATFORM_LINUX)
    include(cmake/linux.cmake)
elseif(PLATFORM_APPLE)
    include(cmake/macos.cmake)
elseif(PLATFORM_WINDOWS)
    include(cmake/windows.cmake)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -Wno-deprecated \
    -Wall -Wextra -Wno-deprecated-copy -pedantic -fdiagnostics-color")

# if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++17")
# endif()

include_directories(${REPO_DIR}/src/)

include_directories(${REPO_DIR}/src/common)

# Googletest
if(PLATFORM_WINDOWS)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/externals/googletest/build_windows/lib)
else()
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/externals/googletest/build/lib)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/externals/googletest/googletest/include)

# Qt6
# On macOS with Homebrew, Qt is not on the default search path.
if(PLATFORM_APPLE)
    set(CMAKE_PREFIX_PATH "/usr/local/opt/qt/lib/cmake")
endif()
find_package(Qt6 COMPONENTS Core Widgets OpenGL OpenGLWidgets SerialPort QUIET)

# Nlohmann
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/externals/nlohmann/include)

# GLM
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/externals/glm/)

# ****** wxWidgets ******
# On macOS: use vendored headers+libs (built for osx_cocoa).
# On Linux: use system wxWidgets found by linux.cmake's find_package call.
if(PLATFORM_APPLE)
    set(WX_CUSTOM_INC_PATH ${REPO_DIR}/src/externals/wxwidgets/include)
    set(WX_CUSTOM_LIB_PATH ${REPO_DIR}/src/externals/wxwidgets/lib)

    include_directories(${WX_CUSTOM_INC_PATH})
    link_directories(${WX_CUSTOM_LIB_PATH})
endif()


# ****** freetype ******
include_directories(${REPO_DIR}/src/externals/freetype/build/include)
include_directories(${REPO_DIR}/src/externals/freetype/include)

if(PLATFORM_WINDOWS)
    # MinGW/Windows: GTest is built with pthreads disabled, no pthread link needed.
    set(GTEST_LIB_FILES libgtest.a libgtest_main.a)
else()
    set(GTEST_LIB_FILES libgtest.a libgtest_main.a pthread)
endif()

# ****** OpenGL ******
# Not available in Windows cross-compilation; only test targets are built there.
if(NOT PLATFORM_WINDOWS)
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})
endif()

# Adding subdirectories
# On Windows (cross-compilation) only the test targets are built — no GUI/GL dependencies.
if(NOT PLATFORM_WINDOWS)
    add_subdirectory(applications/gui_testing)
    add_subdirectory(applications/text_rendering)
    add_subdirectory(applications/real_time_gui)
    add_subdirectory(interfaces/test/c)
    add_subdirectory(interfaces/test/cpp)
    add_subdirectory(demos/demo_app)
    add_subdirectory(demos/plot_stuff)
    add_subdirectory(main_application)
    # third_party/LumosConfig submodule now checked out; qt_application builds
    # for the Phase 0 audit of the prior Qt migration attempt.
    add_subdirectory(qt_application)
    # Fresh Qt migration attempt (Phase 1: framework-independent core only for now).
    add_subdirectory(new_qt_application)
    add_subdirectory(applications/module_application)
    # Temporarily disabled: requires a locally-vendored (gitignored, undocumented)
    # GLFW checkout at this path that isn't present on this machine. See conversation
    # with Claude on 2026-09-17 for details. Not needed to build the `duoplot` target.
    # add_subdirectory(applications/text_rendering/glfw)
endif()
add_subdirectory(system_test)
add_subdirectory(test)
