cmake_minimum_required(VERSION 3.16)
project(tether C)

# ---------------------------------------------------------------------------
# Platform guard: this uses macOS frameworks (DiskArbitration, CoreFoundation,
# IOKit) and the private DiskImages.framework, so it only builds on macOS.
# ---------------------------------------------------------------------------
if(NOT APPLE)
    message(FATAL_ERROR "libtether targets macOS only (requires DiskArbitration and the private DiskImages.framework).")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

# Standard install destinations (CMAKE_INSTALL_BINDIR/LIBDIR/INCLUDEDIR).
include(GNUInstallDirs)

# The CLI tools are optional; build them with -DBUILD_TOOLS=ON.
option(BUILD_TOOLS "Build the attach-dmg and detach-dmg command-line tools" OFF)

# The library is always built.
add_subdirectory(src)

# The tools are built only when requested.
if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()
