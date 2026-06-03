//
// teth_detach.c - detach (unmount and eject) a disk image via DiskArbitration.
//
// Mirrors hdiutil's detach path (process_detach_deventry -> unmount_and_eject): resolve the target
// to its WHOLE disk, DADiskUnmount(...Whole...), then DADiskEject. Ejecting the whole disk of an
// attached image tears the image down, which is the "detach" / umount-equivalent the CLI promises.
//
#include "libtether.h"

#include <DiskArbitration/DiskArbitration.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/param.h>

typedef struct {
    int done;
    int status; // 0 == success; otherwise DADissenter status
    char message[256];
} da_op;

static void capture_dissenter(da_op *op, DADissenterRef dissenter) {
    if (dissenter == NULL) {
        op->status = 0;
        op->message[0] = '\0';
    } else {
        op->status = (int)DADissenterGetStatus(dissenter);
        CFStringRef s = DADissenterGetStatusString(dissenter);
        if (s == NULL ||
            !CFStringGetCString(s, op->message, sizeof op->message, kCFStringEncodingUTF8)) {
            snprintf(op->message, sizeof op->message, "status 0x%08x", op->status);
        }
    }
    op->done = 1;
    CFRunLoopStop(CFRunLoopGetCurrent());
}

static void unmount_cb(DADiskRef disk, DADissenterRef dissenter, void *ctx) {
    (void)disk;
    capture_dissenter((da_op *)ctx, dissenter);
}

static void eject_cb(DADiskRef disk, DADissenterRef dissenter, void *ctx) {
    (void)disk;
    capture_dissenter((da_op *)ctx, dissenter);
}

// Run the current run loop until op->done or the deadline elapses.
static int run_until_done(da_op *op, double timeout_seconds) {
    if (timeout_seconds <= 0) {
        timeout_seconds = 30.0;
    }
    while (!op->done) {
        SInt32 r = CFRunLoopRunInMode(kCFRunLoopDefaultMode, timeout_seconds, true);
        if (op->done) {
            break;
        }
        if (r == kCFRunLoopRunTimedOut || r == kCFRunLoopRunFinished) {
            snprintf(op->message, sizeof op->message, "timed out waiting for DiskArbitration");
            return -1;
        }
    }
    return 0;
}

// Turn a user argument into a BSD disk name (e.g. "disk4"). Accepts:
//   - "/Volumes/Foo" or any path on a mounted volume  -> statfs f_mntfromname
//   - "/dev/disk4s1" / "/dev/rdisk4"                   -> strip the /dev/ (and r)
//   - "disk4s1"                                        -> as-is
// Returns 0 on success, filling out[] (size >= MNAMELEN).
static int resolve_bsd_name(const char *arg, char *out, size_t out_len) {
    if (arg == NULL || arg[0] == '\0') {
        return -1;
    }

    // Explicit device path: "/dev/disk4", "/dev/rdisk4s1". Handle this BEFORE statfs(): statfs of
    // "/dev/disk4" *succeeds* but describes the devfs mount (f_mntfromname == "devfs"), not the
    // disk, so we must special-case it.
    if (strncmp(arg, "/dev/", 5) == 0) {
        const char *p = arg + 5;
        if (strncmp(p, "rdisk", 5) == 0) {
            p += 1; // rdiskN -> diskN
        }
        strlcpy(out, p, out_len);
        return 0;
    }

    // Bare BSD name already: "disk4", "disk4s1".
    if (strncmp(arg, "disk", 4) == 0) {
        strlcpy(out, arg, out_len);
        return 0;
    }

    // Otherwise treat as a path on a mounted volume, e.g. "/Volumes/Foo"; f_mntfromname is like
    // "/dev/disk4s1".
    struct statfs sfs;
    if (statfs(arg, &sfs) == 0) {
        const char *from = sfs.f_mntfromname;
        if (strncmp(from, "/dev/", 5) == 0) {
            from += 5;
        }
        if (strncmp(from, "rdisk", 5) == 0) {
            from += 1; // rdisk -> disk
        }
        strlcpy(out, from, out_len);
        return 0;
    }
    return -1;
}

int teth_detach(const char *dev_or_mount, bool force, double timeout_seconds) {
    char bsd[MNAMELEN];
    if (resolve_bsd_name(dev_or_mount, bsd, sizeof bsd) != 0) {
        fprintf(
            stderr, "detach: cannot interpret \"%s\"\n", dev_or_mount ? dev_or_mount : "(null)");
        return -2;
    }

    DASessionRef session = DASessionCreate(kCFAllocatorDefault);
    if (session == NULL) {
        fprintf(stderr, "detach: DASessionCreate failed\n");
        return -3;
    }
    DASessionScheduleWithRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    int rc = -1;
    DADiskRef disk = DADiskCreateFromBSDName(kCFAllocatorDefault, session, bsd);
    if (disk == NULL) {
        fprintf(stderr, "detach: no such device \"%s\"\n", bsd);
        goto cleanup_session;
    }

    // Operate on the whole disk so the image is fully detached, not just one partition unmounted.
    DADiskRef whole = DADiskCopyWholeDisk(disk);
    DADiskRef target = whole ? whole : disk;

    DADiskUnmountOptions umount_opts = kDADiskUnmountOptionWhole;
    if (force) {
        umount_opts |= kDADiskUnmountOptionForce;
    }

    da_op op = {0};
    DADiskUnmount(target, umount_opts, unmount_cb, &op);
    if (run_until_done(&op, timeout_seconds) != 0 || op.status != 0) {
        fprintf(stderr,
                "detach: unmount of %s failed: %s\n",
                bsd,
                op.message[0] ? op.message : "unknown error");
        rc = op.status ? op.status : -1;
        goto cleanup_disk;
    }

    // Eject the whole disk to tear down the disk-image backing device.
    da_op ej = {0};
    DADiskEject(target, kDADiskEjectOptionDefault, eject_cb, &ej);
    if (run_until_done(&ej, timeout_seconds) != 0 || ej.status != 0) {
        fprintf(stderr,
                "detach: eject of %s failed: %s\n",
                bsd,
                ej.message[0] ? ej.message : "unknown error");
        rc = ej.status ? ej.status : -1;
        goto cleanup_disk;
    }

    printf("\"%s\" detached successfully.\n", bsd);
    rc = 0;

cleanup_disk:
    if (whole) {
        CFRelease(whole);
    }
    if (disk) {
        CFRelease(disk);
    }
cleanup_session:
    DASessionUnscheduleFromRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    CFRelease(session);
    return rc;
}
