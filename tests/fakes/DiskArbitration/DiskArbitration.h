// Fake <DiskArbitration/DiskArbitration.h> for off-platform (Linux) test builds. Declares only the
// DiskArbitration surface libtether uses; functions are intercepted with -Wl,--wrap and
// implemented in tests/mocks.c.
#ifndef FAKE_DISKARBITRATION_H
#define FAKE_DISKARBITRATION_H

#include <CoreFoundation/CoreFoundation.h>

typedef CFTypeRef DASessionRef;
typedef CFTypeRef DADiskRef;
typedef CFTypeRef DADissenterRef;
typedef SInt32 DAReturn;
typedef uint32_t DADiskUnmountOptions;
typedef uint32_t DADiskEjectOptions;

enum {
    kDADiskUnmountOptionDefault = 0x00000000,
    kDADiskUnmountOptionForce = 0x00080000,
    kDADiskUnmountOptionWhole = 0x00000001,
};
enum {
    kDADiskEjectOptionDefault = 0x00000000,
};

typedef void (*DADiskUnmountCallback)(DADiskRef disk, DADissenterRef dissenter, void *context);
typedef void (*DADiskEjectCallback)(DADiskRef disk, DADissenterRef dissenter, void *context);

DASessionRef DASessionCreate(CFAllocatorRef allocator);
void DASessionScheduleWithRunLoop(DASessionRef session, CFRunLoopRef runLoop, CFRunLoopMode mode);
void DASessionUnscheduleFromRunLoop(DASessionRef session, CFRunLoopRef runLoop, CFRunLoopMode mode);

DADiskRef DADiskCreateFromBSDName(CFAllocatorRef allocator, DASessionRef session, const char *name);
DADiskRef DADiskCopyWholeDisk(DADiskRef disk);

void DADiskUnmount(DADiskRef disk,
                   DADiskUnmountOptions options,
                   DADiskUnmountCallback callback,
                   void *context);
void DADiskEject(DADiskRef disk,
                 DADiskEjectOptions options,
                 DADiskEjectCallback callback,
                 void *context);

DAReturn DADissenterGetStatus(DADissenterRef dissenter);
CFStringRef DADissenterGetStatusString(DADissenterRef dissenter);

#endif // FAKE_DISKARBITRATION_H
