//
// detach-dmg - detach (unmount + eject) a disk image, like `hdiutil detach`.
//
// Usage: detach-dmg [-force] [-timeout <seconds>] <device | mountpoint>
//
// Accepts a BSD name ("disk4"), a /dev path ("/dev/disk4s1"), or a mount point ("/Volumes/Foo");
// always operates on the whole disk.
//
#include "libtether.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s [-force] [-timeout <seconds>] <device|mountpoint>\n", argv0);
}

int main(int argc, char **argv) {
    bool force = false;
    double timeout = 0.0; // 0 -> library default
    const char *target = NULL;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "-force") == 0) {
            force = true;
        } else if (strcmp(a, "-timeout") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: -timeout needs an argument\n", argv[0]);
                return 2;
            }
            timeout = atof(argv[++i]);
        } else if (strcmp(a, "-help") == 0 || strcmp(a, "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (a[0] == '-') {
            fprintf(stderr, "%s: unknown option \"%s\"\n", argv[0], a);
            usage(argv[0]);
            return 2;
        } else if (target == NULL) {
            target = a;
        } else {
            fprintf(stderr, "%s: only one device may be specified\n", argv[0]);
            return 2;
        }
    }

    if (target == NULL) {
        usage(argv[0]);
        return 2;
    }

    int status = teth_detach(target, force, timeout);
    if (status != 0) {
        return (status > 0 && status < 256) ? status : 1;
    }
    return 0;
}
