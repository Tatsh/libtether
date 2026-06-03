// Shared cmocka prelude and test fixtures for the libtether test suite. Included first by every
// test translation unit and by mocks.c.
#ifndef TESTS_MOCKS_H
#define TESTS_MOCKS_H

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
// cmocka.h must come after the four headers above.
#include <cmocka.h>

// Non-NULL sentinel handles returned by the fake "create"/"copy" wrappers, so the code under test
// proceeds past null checks. Their exact values are irrelevant.
#define TETH_FAKE_DICT ((void *)0xD1C70000)
#define TETH_FAKE_URL ((void *)0x012F0000)
#define TETH_FAKE_BUNDLE ((void *)0xB0540000)
#define TETH_FAKE_SESSION ((void *)0x5E550000)
#define TETH_FAKE_DISK ((void *)0xD15C0000)
#define TETH_FAKE_WHOLE ((void *)0x301E0000)
#define TETH_ARRAY_TYPE_ID 1001UL
#define TETH_DICT_TYPE_ID 1002UL

// Test-provided DIHLDiskImageAttach implementation. __wrap_CFBundleGetFunctionPointerForName
// returns this for the "DIHLDiskImageAttach" symbol; NULL means "symbol not found".
extern void *g_diskimages_attach_fn;

#endif // TESTS_MOCKS_H
