//
// attach-dmg - attach (mount) a disk image, like `hdiutil attach`.
//
// Usage: attach-dmg [-readonly | -readwrite] [-noverify] [-quiet] [-verbose] <image>
//
// Works for any DiskImages-supported format (.dmg, .sparseimage, .sparsebundle, .iso/.cdr, etc.),
// not just .dmg.
//
#include "libtether.h"

#include <stdio.h>
#include <string.h>

static void usage(const char *argv0) {
    fprintf(stderr,
            "usage: %s [-readonly|-readwrite] [-noverify] [-quiet] [-verbose] <image>\n",
            argv0);
}

int main(int argc, char **argv) {
    teth_attach_options opts = {.read_only = true};
    const char *image = NULL;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "-readonly") == 0) {
            opts.read_only = true;
        } else if (strcmp(a, "-readwrite") == 0) {
            opts.read_only = false;
        } else if (strcmp(a, "-noverify") == 0) {
            opts.skip_verify = true;
        } else if (strcmp(a, "-quiet") == 0) {
            opts.quiet = true;
        } else if (strcmp(a, "-verbose") == 0) {
            opts.verbose = true;
        } else if (strcmp(a, "-help") == 0 || strcmp(a, "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (a[0] == '-') {
            fprintf(stderr, "%s: unknown option \"%s\"\n", argv[0], a);
            usage(argv[0]);
            return 2;
        } else if (image == NULL) {
            image = a;
        } else {
            fprintf(stderr, "%s: only one image may be specified\n", argv[0]);
            return 2;
        }
    }

    if (image == NULL) {
        usage(argv[0]);
        return 2;
    }

    CFDictionaryRef results = NULL;
    int status = teth_attach(image, &opts, &results);
    if (status != 0) {
        return (status > 0 && status < 256) ? status : 1;
    }

    teth_print_attach_results(results);
    if (results) {
        CFRelease(results);
    }
    return 0;
}
