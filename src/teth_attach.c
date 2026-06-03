//
// teth_attach.c - attach (mount) a disk image via the private DiskImages.framework.
//
// Mirrors hdiutil's attach path: build a CFDictionary of options keyed exactly as hdiutil does
// ("main-url", "read-only", "agent" == "framework", ...) and call
// DIHLDiskImageAttach(options, NULL, NULL, &results).
//
#include "diskimages_private.h"
#include "libtether.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// DIHLDiskImageAttach handles any image format DiskImages recognises (UDIF/.dmg, sparseimage,
// ISO/.cdr, NDIF, encrypted, etc.), so this works beyond .dmg. A .sparsebundle is a directory,
// so we must set isDirectory correctly or the URL resolution will fail for bundles.
static CFURLRef create_file_url(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    struct stat st;
    Boolean is_dir = false;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        is_dir = true;
    }
    return CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault, (const UInt8 *)path, (CFIndex)strlen(path), is_dir);
}

static void dict_set_bool(CFMutableDictionaryRef dict, CFStringRef key, bool value) {
    CFDictionarySetValue(dict, key, value ? kCFBooleanTrue : kCFBooleanFalse);
}

int teth_attach(const char *image_path,
                const teth_attach_options *opts,
                CFDictionaryRef *out_results) {
    teth_attach_options defaults = {.read_only = true};
    if (opts == NULL) {
        opts = &defaults;
    }

    CFURLRef url = create_file_url(image_path);
    if (url == NULL) {
        fprintf(
            stderr, "attach: cannot convert \"%s\" to a URL\n", image_path ? image_path : "(null)");
        return -2;
    }

    const char *err = NULL;
    diskimages_api di;
    if (diskimages_loader_init(&di, &err) != 0) {
        fprintf(stderr, "attach: %s\n", err ? err : "DiskImages load failed");
        CFRelease(url);
        return -3;
    }

    CFMutableDictionaryRef options = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    // Required: the image URL and the agent identity hdiutil uses.
    CFDictionarySetValue(options, kDIAttachKeyMainURL, url);
    CFDictionarySetValue(options, kDIAttachKeyAgent, kDIAttachValueFramework);

    dict_set_bool(options, kDIAttachKeyReadOnly, opts->read_only);
    dict_set_bool(options, kDIAttachKeyVerbose, opts->verbose);
    dict_set_bool(options, kDIAttachKeyQuiet, opts->quiet);
    if (opts->skip_verify) {
        dict_set_bool(options, kDIAttachKeySkipVerify, true);
        dict_set_bool(options, kDIAttachKeySkipVerifyLckd, true);
        dict_set_bool(options, kDIAttachKeySkipVerifyRmt, true);
    }

    // Verified call shape (hdiutil and an independent reference): callback and context are NULL;
    // results come back as a CFDictionaryRef out-param.
    CFDictionaryRef results = NULL;
    int status = di.DIHLDiskImageAttach(options, NULL, NULL, &results);

    if (status != 0) {
        fprintf(stderr, "attach: DIHLDiskImageAttach failed: %d\n", status);
        if (results) {
            CFRelease(results);
            results = NULL;
        }
    }

    if (status == 0 && out_results != NULL) {
        *out_results = results; // transfer ownership to caller
    } else if (results != NULL) {
        CFRelease(results);
    }

    CFRelease(options);
    CFRelease(url);
    // Note: we intentionally do NOT DIDeinitialize here; the attached device must outlive this
    // call. diskimages_loader_dispose would tear it down. We leave the framework loaded for the
    // process lifetime, matching hdiutil's behaviour.
    return status;
}

static const char *cf_string_to_c(CFStringRef s, char *buf, size_t buflen) {
    if (s == NULL) {
        return NULL;
    }
    if (CFStringGetCString(s, buf, (CFIndex)buflen, kCFStringEncodingUTF8)) {
        return buf;
    }
    return NULL;
}

void teth_print_attach_results(CFDictionaryRef results) {
    if (results == NULL) {
        return;
    }
    CFArrayRef entities = (CFArrayRef)CFDictionaryGetValue(results, kDIAttachResultSystemEntities);
    if (entities == NULL || CFGetTypeID(entities) != CFArrayGetTypeID()) {
        return;
    }

    CFIndex n = CFArrayGetCount(entities);
    for (CFIndex i = 0; i < n; ++i) {
        CFDictionaryRef e = (CFDictionaryRef)CFArrayGetValueAtIndex(entities, i);
        if (e == NULL || CFGetTypeID(e) != CFDictionaryGetTypeID()) {
            continue;
        }

        char dev[256] = "", hint[256] = "", mp[1024] = "";
        const char *cdev = cf_string_to_c(
            (CFStringRef)CFDictionaryGetValue(e, kDIEntityKeyDevEntry), dev, sizeof dev);
        const char *chint = cf_string_to_c(
            (CFStringRef)CFDictionaryGetValue(e, kDIEntityKeyContentHint), hint, sizeof hint);
        const char *cmp = cf_string_to_c(
            (CFStringRef)CFDictionaryGetValue(e, kDIEntityKeyMountPoint), mp, sizeof mp);

        printf("%-16s\t%-24s\t%s\n", cdev ? cdev : "", chint ? chint : "", cmp ? cmp : "");
    }
}
