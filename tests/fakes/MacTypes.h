// Fake <MacTypes.h> for off-platform (Linux) test builds. Provides only the few Apple scalar types
// libtether uses, so the production sources compile without the macOS SDK.
#ifndef FAKE_MACTYPES_H
#define FAKE_MACTYPES_H

#include <stdint.h>

typedef int32_t OSStatus;
typedef int32_t SInt32;
typedef uint32_t UInt32;
typedef unsigned char UInt8;
typedef unsigned char Boolean;

#endif // FAKE_MACTYPES_H
