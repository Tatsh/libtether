// Fake <sys/mount.h> for off-platform (Linux) test builds. Provides a macOS-style struct statfs
// (with f_mntfromname) and statfs(); the real statfs is intercepted with -Wl,--wrap.
#ifndef FAKE_SYS_MOUNT_H
#define FAKE_SYS_MOUNT_H

#include <sys/param.h> // MNAMELEN

struct statfs {
    char f_mntonname[MNAMELEN];
    char f_mntfromname[MNAMELEN];
};

int statfs(const char *path, struct statfs *buf);

#endif // FAKE_SYS_MOUNT_H
