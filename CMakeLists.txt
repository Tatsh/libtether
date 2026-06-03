cmake_minimum_required(VERSION 3.16)
project(tether VERSION 0.0.1 LANGUAGES C)

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
# Instrument the tests with --coverage (gcov) for Coveralls reporting in CI.
option(ENABLE_COVERAGE "Build the tests with coverage instrumentation" OFF)
# Build the Doxygen documentation with -DBUILD_DOCS=ON (used by the GitHub Pages workflow).
option(BUILD_DOCS "Build the Doxygen documentation" OFF)

# ---------------------------------------------------------------------------
# Platform guard: the library and tools use macOS frameworks (DiskArbitration, CoreFoundation,
# IOKit) and the private DiskImages.framework, so they only build on macOS. The tests and the docs
# do not, so a non-Apple configure is allowed when BUILD_TESTS or BUILD_DOCS is on.
# ---------------------------------------------------------------------------
if(NOT APPLE AND NOT BUILD_TESTS AND NOT BUILD_DOCS)
    message(FATAL_ERROR "libtether targets macOS only (requires DiskArbitration and the private DiskImages.framework). Configure with -DBUILD_TESTS=ON or -DBUILD_DOCS=ON to build off-platform.")
endif()

# The library (and optional tools) only build on macOS.
if(APPLE)
    add_subdirectory(src)
    if(BUILD_TOOLS)
        add_subdirectory(tools)
    endif()

    # Packaging: the `package` target (or `cpack`) produces a downloadable ZIP of the installed
    # library, header, and tools. CI sets CPACK_SYSTEM_NAME per architecture (x86_64, arm64).
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_NAME "libtether")
    set(CPACK_VERBATIM_VARIABLES ON)
    include(CPack)
endif()

# The tests build on any platform.
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# The documentation builds on any platform; the GitHub Pages workflow builds the `docs` target and
# publishes the generated build/html directory.
if(BUILD_DOCS)
    set(DOXYGEN_GENERATE_TAGFILE "${CMAKE_BINARY_DIR}/html/${CMAKE_PROJECT_NAME}.tags")
    set(DOXYGEN_INTERACTIVE_SVG YES)
    set(DOXYGEN_MARKDOWN_ID_STYLE GITHUB)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
    find_package(Doxygen REQUIRED COMPONENTS dot)
    doxygen_add_docs(docs README.md include src ALL)
endif()
