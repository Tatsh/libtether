// cmocka __wrap_ implementations of the CoreFoundation, DiskArbitration, and statfs functions the
// libtether sources call, plus definitions of the CoreFoundation data symbols (which cannot be
// wrapped). The CMake test target adds -Wl,--wrap for each function below.
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

void *g_diskimages_attach_fn = NULL;

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
    const char *str = (const char *)s;
    if (str == NULL || bufferSize <= 0) {
        return 0;
    }
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
    return (CFURLRef)TETH_FAKE_URL;
}

CFBundleRef __wrap_CFBundleGetBundleWithIdentifier(CFStringRef bundleID) {
    (void)bundleID;
    return mock_ptr_type(CFBundleRef);
}

CFBundleRef __wrap_CFBundleCreate(CFAllocatorRef allocator, CFURLRef bundleURL) {
    (void)allocator;
    (void)bundleURL;
    return mock_ptr_type(CFBundleRef);
}

void *__wrap_CFBundleGetFunctionPointerForName(CFBundleRef bundle, CFStringRef functionName) {
    (void)bundle;
    const char *name = (const char *)functionName;
    if (name != NULL && strcmp(name, "DIHLDiskImageAttach") == 0) {
        return g_diskimages_attach_fn;
    }
    return NULL;
}

CFRunLoopRef __wrap_CFRunLoopGetCurrent(void) {
    return (CFRunLoopRef)0x5210;
}

SInt32 __wrap_CFRunLoopRunInMode(CFRunLoopMode mode,
                                 CFTimeInterval seconds,
                                 Boolean returnAfterSourceHandled) {
    (void)mode;
    (void)seconds;
    (void)returnAfterSourceHandled;
    return kCFRunLoopRunTimedOut;
}

void __wrap_CFRunLoopStop(CFRunLoopRef rl) {
    (void)rl;
}

// --- DiskArbitration ----------------------------------------------------------------------------
DASessionRef __wrap_DASessionCreate(CFAllocatorRef allocator) {
    (void)allocator;
    return (DASessionRef)TETH_FAKE_SESSION;
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
    return mock_ptr_type(DADiskRef);
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
    if (callback != NULL) {
        callback(disk, NULL, context); // NULL dissenter == success.
    }
}

void __wrap_DADiskEject(DADiskRef disk,
                        DADiskEjectOptions options,
                        DADiskEjectCallback callback,
                        void *context) {
    (void)options;
    if (callback != NULL) {
        callback(disk, NULL, context); // NULL dissenter == success.
    }
}

DAReturn __wrap_DADissenterGetStatus(DADissenterRef dissenter) {
    (void)dissenter;
    return mock_type(DAReturn);
}

CFStringRef __wrap_DADissenterGetStatusString(DADissenterRef dissenter) {
    (void)dissenter;
    return mock_ptr_type(CFStringRef);
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
