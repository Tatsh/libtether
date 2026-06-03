# Mocked unit tests. The library sources are compiled against the fake framework headers in
# tests/fakes/, and every CoreFoundation/DiskArbitration/statfs call is intercepted with
# -Wl,--wrap and implemented in mocks.c, so these build and run on any platform with GCC or Clang.

# cmocka: prefer the CONFIG package (e.g. from vcpkg) and fall back to pkg-config (system install).
find_package(cmocka CONFIG QUIET)
if(TARGET cmocka::cmocka)
    set(cmocka_target cmocka::cmocka)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CMOCKA REQUIRED IMPORTED_TARGET cmocka)
    set(cmocka_target PkgConfig::CMOCKA)
endif()

# Every framework/libc function the sources call is wrapped by mocks.c.
set(mocked_functions
    CFArrayGetCount
    CFArrayGetTypeID
    CFArrayGetValueAtIndex
    CFBundleCreate
    CFBundleGetBundleWithIdentifier
    CFBundleGetFunctionPointerForName
    CFDictionaryCreateMutable
    CFDictionaryGetTypeID
    CFDictionaryGetValue
    CFDictionarySetValue
    CFGetTypeID
    CFRelease
    CFRetain
    CFRunLoopGetCurrent
    CFRunLoopRunInMode
    CFRunLoopStop
    CFStringGetCString
    CFURLCreateFromFileSystemRepresentation
    CFURLCreateWithFileSystemPath
    DADiskCopyWholeDisk
    DADiskCreateFromBSDName
    DADiskEject
    DADiskUnmount
    DADissenterGetStatus
    DADissenterGetStatusString
    DASessionCreate
    DASessionScheduleWithRunLoop
    DASessionUnscheduleFromRunLoop
    statfs)
set(wrap_flags "")
foreach(fn ${mocked_functions})
    list(APPEND wrap_flags "-Wl,--wrap,${fn}")
endforeach()

# Library sources under test, compiled into each test executable alongside mocks.c.
set(under_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/teth_attach.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/teth_detach.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/diskimages_private.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks.c)

foreach(category attach detach)
    add_executable(test-${category} test_${category}.c ${under_test})
    # fakes/ must be searched before the system headers so framework includes resolve to the stubs.
    target_include_directories(test-${category} BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR})
    # cmocka 2.x deprecates the generic check_expected; silence that one warning.
    target_compile_options(test-${category} PRIVATE -Wno-deprecated-declarations)
    target_link_options(test-${category} PRIVATE ${wrap_flags})
    if(ENABLE_COVERAGE)
        target_compile_options(test-${category} PRIVATE --coverage)
        target_link_options(test-${category} PRIVATE --coverage)
    endif()
    target_link_libraries(test-${category} PRIVATE ${cmocka_target})
    add_test(NAME ${category} COMMAND test-${category})
endforeach()
