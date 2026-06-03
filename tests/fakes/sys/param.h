// Fake <sys/param.h> for off-platform (Linux) test builds. Provides MNAMELEN, which Linux's real
// header does not define but libtether's detach path needs.
#ifndef FAKE_SYS_PARAM_H
#define FAKE_SYS_PARAM_H

#define MNAMELEN 1024

#endif // FAKE_SYS_PARAM_H
