// Shared cmocka prelude and controllable mock state for the libtether test suite. Included first by
// every test translation unit and by mocks.c.
#ifndef TESTS_MOCKS_H
#define TESTS_MOCKS_H

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
// cmocka.h must come after the four headers above.
#include <cmocka.h>

#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DiskArbitration.h>

// Non-NULL sentinel handles returned by the fake "create"/"copy" wrappers.
#define TETH_FAKE_DICT ((void *)0xD1C70000)
#define TETH_FAKE_URL ((void *)0x012F0000)
#define TETH_FAKE_BUNDLE ((void *)0xB0540000)
#define TETH_FAKE_SESSION ((void *)0x5E550000)
#define TETH_FAKE_DISK ((void *)0xD15C0000)
#define TETH_FAKE_WHOLE ((void *)0x301E0000)
#define TETH_FAKE_DISSENTER ((const void *)0xD155E000)
// A CFStringRef the fake CFStringGetCString refuses to convert (returns false).
#define TETH_FAKE_BADSTR ((CFStringRef)0xBAD00000)
#define TETH_ARRAY_TYPE_ID 1001UL
#define TETH_DICT_TYPE_ID 1002UL

// Controllable behaviour for the wrapped CoreFoundation and DiskArbitration functions. Call
// teth_mock_reset() at the start of every test, then override the fields the test needs. The print
// path (CFGetTypeID, CFArrayGetCount, CFArrayGetValueAtIndex, CFDictionaryGetValue) is driven with
// cmocka will_return() instead, as those calls are sequenced.
typedef struct {
    int session_null;          // DASessionCreate returns NULL
    int url_null;              // CFURLCreateWithFileSystemPath returns NULL
    const void *bundle_by_id;  // CFBundleGetBundleWithIdentifier result
    const void *bundle_create; // CFBundleCreate result
    void *attach_fn;           // symbol returned for DIHLDiskImageAttach
    void *init_fn;             // symbol returned for DIInitialize
    void *deinit_fn;           // symbol returned for DIDeinitialize
    int disk_create_null;      // DADiskCreateFromBSDName returns NULL
    int unmount_fire;          // deliver the unmount callback during the run loop
    const void *unmount_dissenter;
    int eject_fire; // deliver the eject callback during the run loop
    const void *eject_dissenter;
    int dissenter_status;         // DADissenterGetStatus result
    const void *dissenter_string; // DADissenterGetStatusString result
} teth_mock_state;

extern teth_mock_state g_mock;
void teth_mock_reset(void);

#endif // TESTS_MOCKS_H
