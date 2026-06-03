// Fake <CoreFoundation/CoreFoundation.h> for off-platform (Linux) test builds. Declares only the
// CoreFoundation surface libtether uses; the functions are intercepted at link time with
// -Wl,--wrap and implemented in tests/mocks.c. Data symbols (kCFAllocatorDefault, kCFBooleanTrue,
// ...) cannot be wrapped, so they are defined in tests/mocks.c and declared extern here.
#ifndef FAKE_COREFOUNDATION_H
#define FAKE_COREFOUNDATION_H

#include <MacTypes.h>
#include <stddef.h>

typedef const void *CFTypeRef;
typedef CFTypeRef CFAllocatorRef;
typedef CFTypeRef CFStringRef;
typedef CFTypeRef CFURLRef;
typedef CFTypeRef CFDictionaryRef;
typedef CFTypeRef CFMutableDictionaryRef;
typedef CFTypeRef CFArrayRef;
typedef CFTypeRef CFBooleanRef;
typedef CFTypeRef CFBundleRef;
typedef CFTypeRef CFRunLoopRef;
typedef CFTypeRef CFRunLoopMode;

typedef long CFIndex;
typedef unsigned long CFTypeID;
typedef uint32_t CFStringEncoding;
typedef double CFTimeInterval;
typedef SInt32 CFURLPathStyle;

typedef struct {
    int dummy;
} CFDictionaryKeyCallBacks;
typedef struct {
    int dummy;
} CFDictionaryValueCallBacks;

#define kCFStringEncodingUTF8 0x08000100
#define kCFURLPOSIXPathStyle 0

enum {
    kCFRunLoopRunFinished = 1,
    kCFRunLoopRunStopped = 2,
    kCFRunLoopRunTimedOut = 3,
    kCFRunLoopRunHandledSource = 4,
};

// CFSTR("x") evaluates to the underlying C string pointer cast to CFStringRef. The wrapped
// CFStringGetCString reads it straight back, which is all the tests need.
#define CFSTR(s) ((CFStringRef)(const void *)(s))

extern const CFAllocatorRef kCFAllocatorDefault;
extern const CFBooleanRef kCFBooleanTrue;
extern const CFBooleanRef kCFBooleanFalse;
extern const CFDictionaryKeyCallBacks kCFTypeDictionaryKeyCallBacks;
extern const CFDictionaryValueCallBacks kCFTypeDictionaryValueCallBacks;
extern const CFRunLoopMode kCFRunLoopDefaultMode;

void CFRelease(CFTypeRef cf);
CFTypeRef CFRetain(CFTypeRef cf);
CFTypeID CFGetTypeID(CFTypeRef cf);
CFTypeID CFArrayGetTypeID(void);
CFTypeID CFDictionaryGetTypeID(void);

CFMutableDictionaryRef CFDictionaryCreateMutable(CFAllocatorRef allocator,
                                                 CFIndex capacity,
                                                 const CFDictionaryKeyCallBacks *keyCallBacks,
                                                 const CFDictionaryValueCallBacks *valueCallBacks);
void CFDictionarySetValue(CFMutableDictionaryRef dict, const void *key, const void *value);
const void *CFDictionaryGetValue(CFDictionaryRef dict, const void *key);

CFIndex CFArrayGetCount(CFArrayRef array);
const void *CFArrayGetValueAtIndex(CFArrayRef array, CFIndex idx);

Boolean CFStringGetCString(CFStringRef s, char *buffer, CFIndex bufferSize, CFStringEncoding enc);

CFURLRef CFURLCreateFromFileSystemRepresentation(CFAllocatorRef allocator,
                                                 const UInt8 *buffer,
                                                 CFIndex bufLen,
                                                 Boolean isDirectory);
CFURLRef CFURLCreateWithFileSystemPath(CFAllocatorRef allocator,
                                       CFStringRef filePath,
                                       CFURLPathStyle pathStyle,
                                       Boolean isDirectory);

CFBundleRef CFBundleGetBundleWithIdentifier(CFStringRef bundleID);
CFBundleRef CFBundleCreate(CFAllocatorRef allocator, CFURLRef bundleURL);
void *CFBundleGetFunctionPointerForName(CFBundleRef bundle, CFStringRef functionName);

CFRunLoopRef CFRunLoopGetCurrent(void);
SInt32 CFRunLoopRunInMode(CFRunLoopMode mode, CFTimeInterval seconds, Boolean returnAfterSourceHandled);
void CFRunLoopStop(CFRunLoopRef rl);

#endif // FAKE_COREFOUNDATION_H
