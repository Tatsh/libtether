//
// diskimages_private.h
//
// Reverse-engineered declarations for the PRIVATE DiskImages.framework
// (/System/Library/PrivateFrameworks/DiskImages.framework).
//
// These are NOT public API and have no Apple-provided header. Every symbol and signature here was
// recovered from the macOS `hdiutil` binary:
//
//   - Import table lists _DIInitialize, _DIDeinitialize, _DIHLDiskImageAttach.
//   - The call site in hdiutil decompiles to:
//         uVar3 = _DIHLDiskImageAttach(optionsDict, progressCb, 0, &outDict);
//     i.e. (CFDictionaryRef, callback, context, CFDictionaryRef *) -> int (0 == success).
//   - An independent GitHub reference shows the same call with nil callback and context:
//         _dihlDiskImageAttach((CFDictionaryRef)dict, nil, nil, (CFDictionaryRef *)&results);
//     confirming the callback and context may be NULL and the out-param is a CFDictionaryRef.
//   - Option-dictionary keys are literal CFStrings found verbatim in hdiutil's __cstring.
//
// Because the framework lives in the dyld shared cache (no on-disk .tbd stub on recent macOS), we
// resolve these symbols at runtime via CFBundle -- CFBundleGetFunctionPointerForName -- exactly as
// Apple's own libsecurity_filevault (FVDIHLInterface.cpp) does:
//
//     CFURLRef url = CFURLCreateWithFileSystemPath(NULL,
//         CFSTR("/System/Library/PrivateFrameworks/DiskImages.framework"),
//         kCFURLPOSIXPathStyle, false);
//     CFBundleRef b = CFBundleCreate(NULL, url);
//     fp = (_proc *)CFBundleGetFunctionPointerForName(b, CFSTR("DIHLDiskImageAttach"));
//
// (hdiutil hard-links the framework, hence the direct _DIHL* imports; we cannot hard-link without a
// stub, so we follow the filevault pattern.) See diskimages_private.c.
//
#ifndef DISKIMAGES_PRIVATE_H
#define DISKIMAGES_PRIVATE_H

#include <CoreFoundation/CoreFoundation.h>
#include <MacTypes.h> // OSStatus

#ifdef __cplusplus
extern "C" {
#endif

// Authoritative typedefs, transcribed verbatim from Apple's open-source Darwin
// libsecurity_filevault (FVDIHLInterface.h):
//
//   typedef OSStatus _dihlDiskImageAttachProc(CFDictionaryRef inOptions,
//                                             void *inStatusProc,
//                                             void *inContext,
//                                             CFDictionaryRef *outResults);
//   typedef int  _dihlDIInitializeProc();
//   typedef void _dihlDIDeinitializeProc();
//
// OSStatus is SInt32 (== int32_t); 0 (noErr) means success. inStatusProc is a status callback
// declared simply as void* in Apple's header and may be NULL (hdiutil passes a real one; filevault
// passes nil). DIInitialize returns int; DIDeinitialize returns void.

// Function-pointer types matching the authoritative signatures.
typedef int (*DIInitialize_fn)(void);
typedef void (*DIDeinitialize_fn)(void);
typedef OSStatus (*DIHLDiskImageAttach_fn)(CFDictionaryRef inOptions,
                                           void *inStatusProc,
                                           void *inContext,
                                           CFDictionaryRef *outResults);

// Resolved entry points (populated by diskimages_loader_init).
typedef struct {
    CFBundleRef bundle; // DiskImages.framework bundle
    DIInitialize_fn DIInitialize;
    DIDeinitialize_fn DIDeinitialize;
    DIHLDiskImageAttach_fn DIHLDiskImageAttach;
} diskimages_api;

// Load DiskImages.framework via CFBundle and resolve the symbols above. Returns 0 on success;
// non-zero on failure with *err (if non-NULL) pointing at a static description. On success, also
// calls DIInitialize() once.
int diskimages_loader_init(diskimages_api *api, const char **err);

// Release the bundle and zero the struct. Safe to call with a zeroed struct.
void diskimages_loader_dispose(diskimages_api *api);

// Option-dictionary keys for DIHLDiskImageAttach, exactly as hdiutil builds them. The values are
// literal CFStrings found verbatim in hdiutil's __cstring. Verified __cstring addresses:
//   main-url 0x10002cdf1, shadow-url 0x10002ce16, read-only 0x10002c462, agent 0x10002c43b,
//   verbose 0x10002cdbc, quiet 0x10002cdca, skip-verify 0x10002c6a1,
//   skip-verify-locked 0x10002c6b5, skip-verify-remote 0x10002c6c8.
// The "agent" key is set to the value "framework"; main-url and shadow-url are CFURLRef and the
// rest are CFBoolean.
#define kDIAttachKeyMainURL CFSTR("main-url")
#define kDIAttachKeyShadowURL CFSTR("shadow-url")
#define kDIAttachKeyReadOnly CFSTR("read-only")
#define kDIAttachKeyAgent CFSTR("agent")
#define kDIAttachValueFramework CFSTR("framework")
#define kDIAttachKeyVerbose CFSTR("verbose")
#define kDIAttachKeyQuiet CFSTR("quiet")
#define kDIAttachKeySkipVerify CFSTR("skip-verify")
#define kDIAttachKeySkipVerifyLckd CFSTR("skip-verify-locked")
#define kDIAttachKeySkipVerifyRmt CFSTR("skip-verify-remote")

// Result-dictionary keys returned via the out-param. The result is a CFDictionaryRef whose
// kDIAttachResultSystemEntities entry is a CFArray of CFDictionary, each describing one attached
// node. Verified __cstring addresses: system-entities 0x10002cf6d, image-path 0x1000320da,
// dev-entry 0x10002cf9c (e.g. "/dev/diskNsM"), mount-point 0x10002ca1b (e.g. "/Volumes/..."),
// content-hint 0x10002cfb1 (e.g. "Apple_HFS").
#define kDIAttachResultSystemEntities CFSTR("system-entities")
#define kDIAttachResultImagePath CFSTR("image-path")
#define kDIEntityKeyDevEntry CFSTR("dev-entry")
#define kDIEntityKeyMountPoint CFSTR("mount-point")
#define kDIEntityKeyContentHint CFSTR("content-hint")

#ifdef __cplusplus
}
#endif

#endif // DISKIMAGES_PRIVATE_H
