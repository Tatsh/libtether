cmake_minimum_required(VERSION 3.16)
project(tether C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

# Standard install destinations (CMAKE_INSTALL_BINDIR/LIBDIR/INCLUDEDIR).
include(GNUInstallDirs)

# The CLI tools are optional; build them with -DBUILD_TOOLS=ON.
option(BUILD_TOOLS "Build the attach-dmg and detach-dmg command-line tools" OFF)
# The tests mock the macOS frameworks with cmocka, so they build and run on any platform with
# GCC or Clang; enable them with -DBUILD_TESTS=ON.
option(BUILD_TESTS "Build the mocked unit tests" OFF)

# ---------------------------------------------------------------------------
# Platform guard: the library and tools use macOS frameworks (DiskArbitration, CoreFoundation,
# IOKit) and the private DiskImages.framework, so they only build on macOS. The tests do not, so a
# non-Apple configure is allowed when BUILD_TESTS is on.
# ---------------------------------------------------------------------------
if(NOT APPLE AND NOT BUILD_TESTS)
    message(FATAL_ERROR "libtether targets macOS only (requires DiskArbitration and the private DiskImages.framework). Configure with -DBUILD_TESTS=ON to build the mocked tests off-platform.")
endif()

# The library (and optional tools) only build on macOS.
if(APPLE)
    add_subdirectory(src)
    if(BUILD_TOOLS)
        add_subdirectory(tools)
    endif()
endif()

# The tests build on any platform.
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
