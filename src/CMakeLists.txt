# libtether - the attach/detach library (always built).

# ---------------------------------------------------------------------------
# Required public frameworks. find_library locates these in the SDK.
# ---------------------------------------------------------------------------
find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
find_library(DISK_ARBITRATION_FRAMEWORK DiskArbitration REQUIRED)
find_library(IOKIT_FRAMEWORK IOKit REQUIRED)

# ---------------------------------------------------------------------------
# Private DiskImages.framework.
#
# We do NOT link against it: on current macOS it lives in the dyld shared cache
# with no public .tbd stub, so a link-time -framework would fail. Instead the
# library loads it at runtime via CFBundle (see diskimages_private.c). We only
# probe for it here to emit a friendly note if it is missing.
# ---------------------------------------------------------------------------
find_library(DISKIMAGES_FRAMEWORK DiskImages
    PATHS /System/Library/PrivateFrameworks
    NO_DEFAULT_PATH)
if(NOT DISKIMAGES_FRAMEWORK)
    message(STATUS "DiskImages.framework not found on disk (expected on modern macOS; it lives in the dyld shared cache and is loaded via CFBundle at runtime).")
else()
    message(STATUS "Found DiskImages.framework: ${DISKIMAGES_FRAMEWORK}")
endif()

add_library(tether STATIC
    teth_attach.c
    teth_detach.c
    diskimages_private.c)

# Public headers live one level up, in ../include.
target_include_directories(tether PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(tether PUBLIC
    ${CORE_FOUNDATION_FRAMEWORK}
    ${DISK_ARBITRATION_FRAMEWORK}
    ${IOKIT_FRAMEWORK})

# Install the static library and its public header (from ../include).
install(TARGETS tether ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/libtether.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
