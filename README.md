# libtether — native `hdiutil attach` / `detach` equivalents

<!-- WISWA-GENERATED-README:START -->

[![C](https://img.shields.io/badge/C-00599C?logo=c)](<https://en.wikipedia.org/wiki/C_(programming_language)>)
[![GitHub tag (with filter)](https://img.shields.io/github/v/tag/Tatsh/libtether)](https://github.com/Tatsh/libtether/tags)
[![License](https://img.shields.io/github/license/Tatsh/libtether)](https://github.com/Tatsh/libtether/blob/master/LICENSE.txt)
[![GitHub commits since latest release (by SemVer including pre-releases)](https://img.shields.io/github/commits-since/Tatsh/libtether/v0.0.1/master)](https://github.com/Tatsh/libtether/compare/v0.0.1...master)
[![CodeQL](https://github.com/Tatsh/libtether/actions/workflows/codeql.yml/badge.svg)](https://github.com/Tatsh/libtether/actions/workflows/codeql.yml)
[![Tests](https://github.com/Tatsh/libtether/actions/workflows/tests.yml/badge.svg)](https://github.com/Tatsh/libtether/actions/workflows/tests.yml)
[![Coverage Status](https://coveralls.io/repos/github/Tatsh/libtether/badge.svg?branch=master)](https://coveralls.io/github/Tatsh/libtether?branch=master)
[![Dependabot](https://img.shields.io/badge/Dependabot-enabled-blue?logo=dependabot)](https://github.com/dependabot)
[![Stargazers](https://img.shields.io/github/stars/Tatsh/libtether?logo=github&style=flat)](https://github.com/Tatsh/libtether/stargazers)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit)](https://github.com/pre-commit/pre-commit)
[![CMake](https://img.shields.io/badge/CMake-6E6E6E?logo=cmake)](https://cmake.org/)
[![Prettier](https://img.shields.io/badge/Prettier-black?logo=prettier)](https://prettier.io/)

[![@Tatsh](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fpublic.api.bsky.app%2Fxrpc%2Fapp.bsky.actor.getProfile%2F%3Factor=did%3Aplc%3Auq42idtvuccnmtl57nsucz72&query=%24.followersCount&label=Follow+%40Tatsh&logo=bluesky&style=social)](https://bsky.app/profile/Tatsh.bsky.social)
[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Tatsh-black?logo=buymeacoffee)](https://buymeacoffee.com/Tatsh)
[![Libera.Chat](https://img.shields.io/badge/Libera.Chat-Tatsh-black?logo=liberadotchat)](irc://irc.libera.chat/Tatsh)
[![Mastodon Follow](https://img.shields.io/mastodon/follow/109370961877277568?domain=hostux.social&style=social)](https://hostux.social/@Tatsh)
[![Patreon](https://img.shields.io/badge/Patreon-Tatsh2-F96854?logo=patreon)](https://www.patreon.com/Tatsh2)

<!-- WISWA-GENERATED-README:STOP -->

A small C library (`libtether`) plus two CLIs — **`attach-dmg`** and **`detach-dmg`** —
that mount and unmount disk images the same way `hdiutil` does, by calling the
same underlying macOS APIs.

> _Named for what it does: it tethers a disk image to the running system. Attach mounts the image
> as a `/dev/diskN` device; detach unmounts and releases it. That is the whole library — a tether
> you fasten and cast off, with no image creation or format manipulation._
>
> **macOS only.** This depends on `DiskArbitration.framework`, `CoreFoundation`,
> `IOKit`, and the **private** `DiskImages.framework`. The CMake build hard-fails
> on non-Apple platforms.

## How it works (and why)

Attaching a `.dmg` is **not** a DiskArbitration operation. DiskArbitration only
mounts/unmounts/ejects devices that already exist. Creating the `/dev/diskN`
backing device from an image file is done by the **private** `DiskImages.framework`.
So:

| Operation                          | Framework                            | Key call                                |
| ---------------------------------- | ------------------------------------ | --------------------------------------- |
| **attach** (create device + mount) | `DiskImages.framework` (private)     | `DIHLDiskImageAttach`                   |
| **detach** (unmount + tear down)   | `DiskArbitration.framework` (public) | `DADiskUnmount` (whole) + `DADiskEject` |

This split is exactly how `hdiutil` itself is built (its detach path is literally
named `unmount_and_eject`).

### The reverse-engineered attach API

The signature was recovered from three independent sources that all agree:

1. **The `hdiutil` binary** (Ghidra) — call site decompiles to
   `DIHLDiskImageAttach(optionsDict, progressCb, 0, &outDict)` returning `int` (0 = ok).
2. **An in-the-wild caller** — `_dihlDiskImageAttach((CFDictionaryRef)dict, nil, nil, (CFDictionaryRef*)&results)`
   (callback + context may be `nil`; out-param is a `CFDictionaryRef`).
3. **Apple's open-source Darwin `libsecurity_filevault`** (`FVDIHLInterface.h`) — the
   authoritative typedefs:

   ```c
   typedef OSStatus _dihlDiskImageAttachProc(CFDictionaryRef inOptions,
                                             void *inStatusProc,
                                             void *inContext,
                                             CFDictionaryRef *outResults);
   typedef int  _dihlDIInitializeProc();
   typedef void _dihlDIDeinitializeProc();
   ```

   Note `inStatusProc` is plain `void *` in Apple's header, `DIInitialize` returns
   `int`, and `DIDeinitialize` returns `void`.

We declare these ourselves in [`include/diskimages_private.h`](include/diskimages_private.h)
and resolve them at runtime via `CFBundle` (see below).

#### Options dictionary (keys verified verbatim in `hdiutil`'s strings)

| Key                                                       | Type        | Notes                                                    |
| --------------------------------------------------------- | ----------- | -------------------------------------------------------- |
| `main-url`                                                | `CFURLRef`  | the image to attach (required)                           |
| `shadow-url`                                              | `CFURLRef`  | optional shadow file (read-write overlay)                |
| `read-only`                                               | `CFBoolean` | force read-only                                          |
| `agent`                                                   | `CFString`  | set to `framework` (the value hdiutil uses on this path) |
| `verbose` / `quiet`                                       | `CFBoolean` | diagnostics level                                        |
| `skip-verify`, `skip-verify-locked`, `skip-verify-remote` | `CFBoolean` | skip checksum verification                               |

#### Result dictionary (`*outResults`)

A `CFDictionaryRef` whose `system-entities` key is a `CFArray` of dictionaries,
each describing one attached node:

| Entity key     | Example        |
| -------------- | -------------- |
| `dev-entry`    | `/dev/disk4s1` |
| `mount-point`  | `/Volumes/Foo` |
| `content-hint` | `Apple_HFS`    |

### Why `CFBundle` instead of linking `-framework DiskImages`

On current macOS, private frameworks live in the **dyld shared cache** with no
on-disk `.tbd` stub, so link-time `-framework DiskImages` typically fails. We
therefore resolve the needed symbols (`DIInitialize`, `DIDeinitialize`,
`DIHLDiskImageAttach`) at runtime — using **`CFBundle`**, exactly as Apple's own
`libsecurity_filevault` (`FVDIHLInterface.cpp`) does:

```c
CFURLRef url = CFURLCreateWithFileSystemPath(NULL,
    CFSTR("/System/Library/PrivateFrameworks/DiskImages.framework"),
    kCFURLPOSIXPathStyle, false);
CFBundleRef bundle = CFBundleCreate(NULL, url);
fp = (_dihlDiskImageAttachProc *)
     CFBundleGetFunctionPointerForName(bundle, CFSTR("DIHLDiskImageAttach"));
```

We also try `CFBundleGetBundleWithIdentifier(CFSTR("com.apple.DiskImagesFramework"))`
first (that bundle identifier is present in hdiutil's strings). This needs no
private headers or SDK stubs. (`hdiutil` itself _hard-links_ DiskImages — which is
why its import table lists the `_DIHL*` symbols directly — but we can't hard-link
without a stub.)

## Supported image formats

Because `attach-dmg` only hands `main-url` to `DIHLDiskImageAttach`, it attaches
**any format DiskImages recognizes**, not just `.dmg`: UDIF (`.dmg`),
sparse images (`.sparseimage`), sparse bundles (`.sparsebundle`, a _directory_ —
handled), ISO/CD masters (`.iso`, `.cdr`), NDIF, and encrypted images. The
framework sniffs the format itself.

## Build

```sh
cd native
cmake -S . -B build
cmake --build build
```

By default this builds only the library, `build/libtether.a`. The `attach-dmg` and
`detach-dmg` CLIs are **optional** — enable them with `-DBUILD_TOOLS=ON`:

```sh
cmake -S . -B build -DBUILD_TOOLS=ON
cmake --build build
```

which additionally produces `build/attach-dmg` and `build/detach-dmg`.

## Usage

```sh
# Attach (mount). Prints one line per attached entity: dev-entry, hint, mount-point.
./attach-dmg /path/to/Image.dmg
./attach-dmg -readwrite /path/to/Image.sparseimage
./attach-dmg -noverify -quiet /path/to/Image.dmg

# Detach (unmount + eject the whole disk). Accepts a BSD name, /dev path, or mount point.
./detach-dmg disk4
./detach-dmg /dev/disk4s1
./detach-dmg /Volumes/Foo
./detach-dmg -force -timeout 60 disk4
```

## Layout

```text
native/
├── CMakeLists.txt              # project, options, add_subdirectory(src/tools)
├── include/
│   ├── libtether.h             # public library API
│   └── diskimages_private.h    # reverse-engineered private DiskImages decls
├── src/
│   ├── CMakeLists.txt           # builds libtether (always)
│   ├── teth_attach.c            # attach via DIHLDiskImageAttach
│   ├── teth_detach.c            # detach via DiskArbitration unmount+eject
│   └── diskimages_private.c     # CFBundle loader for DiskImages.framework
└── tools/
    ├── CMakeLists.txt           # builds attach-dmg/detach-dmg (BUILD_TOOLS=ON)
    ├── attach_dmg.c             # attach-dmg CLI
    └── detach_dmg.c             # detach-dmg CLI
```

## Caveats / residual risk

- Some images may require entitlements or admin rights to attach; run with
  appropriate privileges if attach returns a permission error.
- The status callback (`inStatusProc`) is plain `void *` in Apple's header and we
  always pass `NULL`, which all observed callers (filevault) do. `hdiutil` passes a
  real callback; if a future OS requires a non-NULL one, its prototype would need
  to be recovered.
- Not yet compiled on macOS in this environment (developed/verified for API
  correctness on Linux, which has no macOS SDK). Build on macOS to validate.
