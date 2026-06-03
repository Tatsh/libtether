// cmocka __wrap_ implementations of the CoreFoundation, DiskArbitration, and statfs functions the
// libtether sources call, plus the CoreFoundation data symbols (which cannot be wrapped). Most
// behaviour is driven by g_mock (reset per test); the sequenced print-path calls use cmocka
// will_return(). The CMake test target adds -Wl,--wrap for each function below.
#include "mocks.h"

#include <string.h>

#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DiskArbitration.h>
#include <sys/mount.h>

// --- CoreFoundation data symbols (referenced by address or value; not wrappable) ----------------
const CFAllocatorRef kCFAllocatorDefault = NULL;
const CFBooleanRef kCFBooleanTrue = (CFBooleanRef)0xB001;
const CFBooleanRef kCFBooleanFalse = (CFBooleanRef)0xB000;
const CFDictionaryKeyCallBacks kCFTypeDictionaryKeyCallBacks = {0};
const CFDictionaryValueCallBacks kCFTypeDictionaryValueCallBacks = {0};
const CFRunLoopMode kCFRunLoopDefaultMode = (CFRunLoopMode) "kCFRunLoopDefaultMode";

teth_mock_state g_mock;

void teth_mock_reset(void) {
    memset(&g_mock, 0, sizeof g_mock);
    g_mock.unmount_fire = 1;
    g_mock.eject_fire = 1;
}

// A DiskArbitration callback armed by DADiskUnmount/DADiskEject and delivered by the next
// CFRunLoopRunInMode, mirroring the real asynchronous behaviour.
static struct {
    DADiskUnmountCallback cb;
    DADiskRef disk;
    void *ctx;
    DADissenterRef dissenter;
    int armed;
} g_pending;

// --- CoreFoundation -----------------------------------------------------------------------------
void __wrap_CFRelease(CFTypeRef cf) {
    (void)cf;
}

CFTypeRef __wrap_CFRetain(CFTypeRef cf) {
    return cf;
}

CFTypeID __wrap_CFGetTypeID(CFTypeRef cf) {
    (void)cf;
    return mock_type(CFTypeID);
}

CFTypeID __wrap_CFArrayGetTypeID(void) {
    return TETH_ARRAY_TYPE_ID;
}

CFTypeID __wrap_CFDictionaryGetTypeID(void) {
    return TETH_DICT_TYPE_ID;
}

CFMutableDictionaryRef
__wrap_CFDictionaryCreateMutable(CFAllocatorRef allocator,
                                 CFIndex capacity,
                                 const CFDictionaryKeyCallBacks *keyCallBacks,
                                 const CFDictionaryValueCallBacks *valueCallBacks) {
    (void)allocator;
    (void)capacity;
    (void)keyCallBacks;
    (void)valueCallBacks;
    return (CFMutableDictionaryRef)TETH_FAKE_DICT;
}

void __wrap_CFDictionarySetValue(CFMutableDictionaryRef dict, const void *key, const void *value) {
    (void)dict;
    (void)key;
    (void)value;
}

const void *__wrap_CFDictionaryGetValue(CFDictionaryRef dict, const void *key) {
    (void)dict;
    (void)key;
    return mock_ptr_type(const void *);
}

CFIndex __wrap_CFArrayGetCount(CFArrayRef array) {
    (void)array;
    return mock_type(CFIndex);
}

const void *__wrap_CFArrayGetValueAtIndex(CFArrayRef array, CFIndex idx) {
    (void)array;
    (void)idx;
    return mock_ptr_type(const void *);
}

Boolean
__wrap_CFStringGetCString(CFStringRef s, char *buffer, CFIndex bufferSize, CFStringEncoding enc) {
    (void)enc;
    if (s == NULL || s == TETH_FAKE_BADSTR || bufferSize <= 0) {
        return 0;
    }
    const char *str = (const char *)s;
    CFIndex i = 0;
    for (; str[i] != '\0' && i < bufferSize - 1; ++i) {
        buffer[i] = str[i];
    }
    buffer[i] = '\0';
    return 1;
}

CFURLRef __wrap_CFURLCreateFromFileSystemRepresentation(CFAllocatorRef allocator,
                                                        const UInt8 *buffer,
                                                        CFIndex bufLen,
                                                        Boolean isDirectory) {
    (void)allocator;
    (void)buffer;
    (void)bufLen;
    (void)isDirectory;
    return (CFURLRef)TETH_FAKE_URL;
}

CFURLRef __wrap_CFURLCreateWithFileSystemPath(CFAllocatorRef allocator,
                                              CFStringRef filePath,
                                              CFURLPathStyle pathStyle,
                                              Boolean isDirectory) {
    (void)allocator;
    (void)filePath;
    (void)pathStyle;
    (void)isDirectory;
    return g_mock.url_null ? NULL : (CFURLRef)TETH_FAKE_URL;
}

CFBundleRef __wrap_CFBundleGetBundleWithIdentifier(CFStringRef bundleID) {
    (void)bundleID;
    return (CFBundleRef)g_mock.bundle_by_id;
}

CFBundleRef __wrap_CFBundleCreate(CFAllocatorRef allocator, CFURLRef bundleURL) {
    (void)allocator;
    (void)bundleURL;
    return (CFBundleRef)g_mock.bundle_create;
}

void *__wrap_CFBundleGetFunctionPointerForName(CFBundleRef bundle, CFStringRef functionName) {
    (void)bundle;
    const char *name = (const char *)functionName;
    if (name == NULL) {
        return NULL;
    }
    if (strcmp(name, "DIHLDiskImageAttach") == 0) {
        return g_mock.attach_fn;
    }
    if (strcmp(name, "DIInitialize") == 0) {
        return g_mock.init_fn;
    }
    if (strcmp(name, "DIDeinitialize") == 0) {
        return g_mock.deinit_fn;
    }
    return NULL;
}

CFRunLoopRef __wrap_CFRunLoopGetCurrent(void) {
    return (CFRunLoopRef)0x5210;
}

SInt32 __wrap_CFRunLoopRunInMode(CFRunLoopMode mode, CFTimeInterval seconds, Boolean handled) {
    (void)mode;
    (void)seconds;
    (void)handled;
    if (g_pending.armed) {
        g_pending.armed = 0;
        if (g_pending.cb != NULL) {
            g_pending.cb(g_pending.disk, g_pending.dissenter, g_pending.ctx);
        }
        return kCFRunLoopRunStopped;
    }
    return kCFRunLoopRunTimedOut;
}

void __wrap_CFRunLoopStop(CFRunLoopRef rl) {
    (void)rl;
}

// --- DiskArbitration ----------------------------------------------------------------------------
DASessionRef __wrap_DASessionCreate(CFAllocatorRef allocator) {
    (void)allocator;
    return g_mock.session_null ? NULL : (DASessionRef)TETH_FAKE_SESSION;
}

void __wrap_DASessionScheduleWithRunLoop(DASessionRef session,
                                         CFRunLoopRef runLoop,
                                         CFRunLoopMode mode) {
    (void)session;
    (void)runLoop;
    (void)mode;
}

void __wrap_DASessionUnscheduleFromRunLoop(DASessionRef session,
                                           CFRunLoopRef runLoop,
                                           CFRunLoopMode mode) {
    (void)session;
    (void)runLoop;
    (void)mode;
}

DADiskRef
__wrap_DADiskCreateFromBSDName(CFAllocatorRef allocator, DASessionRef session, const char *name) {
    (void)allocator;
    (void)session;
    check_expected(name);
    return g_mock.disk_create_null ? NULL : (DADiskRef)TETH_FAKE_DISK;
}

DADiskRef __wrap_DADiskCopyWholeDisk(DADiskRef disk) {
    (void)disk;
    return (DADiskRef)TETH_FAKE_WHOLE;
}

void __wrap_DADiskUnmount(DADiskRef disk,
                          DADiskUnmountOptions options,
                          DADiskUnmountCallback callback,
                          void *context) {
    (void)options;
    if (g_mock.unmount_fire) {
        g_pending.cb = callback;
        g_pending.disk = disk;
        g_pending.ctx = context;
        g_pending.dissenter = (DADissenterRef)g_mock.unmount_dissenter;
        g_pending.armed = 1;
    }
}

void __wrap_DADiskEject(DADiskRef disk,
                        DADiskEjectOptions options,
                        DADiskEjectCallback callback,
                        void *context) {
    (void)options;
    if (g_mock.eject_fire) {
        g_pending.cb = callback;
        g_pending.disk = disk;
        g_pending.ctx = context;
        g_pending.dissenter = (DADissenterRef)g_mock.eject_dissenter;
        g_pending.armed = 1;
    }
}

DAReturn __wrap_DADissenterGetStatus(DADissenterRef dissenter) {
    (void)dissenter;
    return g_mock.dissenter_status;
}

CFStringRef __wrap_DADissenterGetStatusString(DADissenterRef dissenter) {
    (void)dissenter;
    return (CFStringRef)g_mock.dissenter_string;
}

// --- statfs -------------------------------------------------------------------------------------
int __wrap_statfs(const char *path, struct statfs *buf) {
    (void)path;
    const char *from = mock_ptr_type(const char *);
    if (from != NULL && buf != NULL) {
        size_t i = 0;
        for (; from[i] != '\0' && i < MNAMELEN - 1; ++i) {
            buf->f_mntfromname[i] = from[i];
        }
        buf->f_mntfromname[i] = '\0';
    }
    return mock_type(int);
}
