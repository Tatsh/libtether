//
// diskimages_private.c
//
// Runtime loader for the private DiskImages.framework, using the same mechanism Apple's own
// libsecurity_filevault (FVDIHLInterface.cpp) uses: CFBundleCreate on the framework URL, then
// CFBundleGetFunctionPointerForName per symbol. This avoids a link-time dependency (the framework
// has no public .tbd and lives in the dyld shared cache) and correctly honours framework bundle
// structure.
//
#include "diskimages_private.h"

#include <stddef.h>
#include <string.h>

// The path Apple's filevault code passes to CFURLCreateWithFileSystemPath.
#define DISKIMAGES_FRAMEWORK_PATH CFSTR("/System/Library/PrivateFrameworks/DiskImages.framework")
// Bundle identifier, recovered from hdiutil __cstring at 0x10002c0a9.
#define DISKIMAGES_BUNDLE_ID CFSTR("com.apple.DiskImagesFramework")

static CFBundleRef create_diskimages_bundle(void) {
    // Prefer an already-loaded copy by identifier; otherwise create from path, mirroring
    // FVDIHLInterface.cpp.
    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(DISKIMAGES_BUNDLE_ID);
    if (bundle != NULL) {
        CFRetain(bundle); // GetBundleWithIdentifier returns a non-owned ref
        return bundle;
    }

    CFURLRef url = CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault, DISKIMAGES_FRAMEWORK_PATH, kCFURLPOSIXPathStyle, false);
    if (url == NULL) {
        return NULL;
    }

    bundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFRelease(url);
    return bundle;
}

int diskimages_loader_init(diskimages_api *api, const char **err) {
    if (api == NULL) {
        if (err) {
            *err = "diskimages_loader_init: NULL api";
        }
        return -1;
    }
    memset(api, 0, sizeof(*api));

    api->bundle = create_diskimages_bundle();
    if (api->bundle == NULL) {
        if (err) {
            *err = "could not create CFBundle for DiskImages.framework";
        }
        return -1;
    }

    // CFBundleGetFunctionPointerForName loads the bundle's executable on demand.
    api->DIInitialize =
        (DIInitialize_fn)CFBundleGetFunctionPointerForName(api->bundle, CFSTR("DIInitialize"));
    api->DIDeinitialize =
        (DIDeinitialize_fn)CFBundleGetFunctionPointerForName(api->bundle, CFSTR("DIDeinitialize"));
    api->DIHLDiskImageAttach = (DIHLDiskImageAttach_fn)CFBundleGetFunctionPointerForName(
        api->bundle, CFSTR("DIHLDiskImageAttach"));

    if (api->DIHLDiskImageAttach == NULL) {
        if (err) {
            *err = "DIHLDiskImageAttach not found in DiskImages.framework";
        }
        diskimages_loader_dispose(api);
        return -1;
    }

    // hdiutil initialises the framework before use (see "failed during framework init" at
    // 0x10003125b). Apple's filevault code does not, so treat a failure as non-fatal: attempt it if
    // present, but proceed regardless.
    if (api->DIInitialize != NULL) {
        (void)api->DIInitialize();
    }

    if (err) {
        *err = NULL;
    }
    return 0;
}

void diskimages_loader_dispose(diskimages_api *api) {
    if (api == NULL) {
        return;
    }
    if (api->DIDeinitialize != NULL) {
        api->DIDeinitialize();
    }
    if (api->bundle != NULL) {
        CFRelease(api->bundle);
    }
    memset(api, 0, sizeof(*api));
}
