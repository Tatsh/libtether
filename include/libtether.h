/**
 * @file libtether.h
 * @brief Public interface of libtether: attach and detach macOS disk images.
 *
 * libtether tethers a disk image to the running system: attach mounts it as a /dev/diskN device,
 * and detach unmounts and releases it again. It does only those two things — there is no creation
 * or format manipulation. It replicates what `hdiutil attach` and `hdiutil detach` do:
 * - attach uses the private DiskImages.framework (`DIHLDiskImageAttach`);
 * - detach uses the public DiskArbitration.framework (unmount and eject of the whole disk),
 *   mirroring hdiutil's `unmount_and_eject` path.
 */
#ifndef LIBTETHER_H
#define LIBTETHER_H

#include <CoreFoundation/CoreFoundation.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Options controlling how teth_attach() mounts an image.
 *
 * Each field maps to an entry hdiutil sets in the DiskImages attach options dictionary. A NULL
 * options pointer passed to teth_attach() is treated as a read-only attach with verification
 * enabled.
 */
typedef struct {
    bool read_only;   /**< Set the `read-only` attach option; a plain attach is read-only. */
    bool skip_verify; /**< Set the `skip-verify` family of attach options. */
    bool verbose;     /**< Set the `verbose` attach option. */
    bool quiet;       /**< Set the `quiet` attach option. */
} teth_attach_options;

/**
 * @brief Attach (mount) the disk image at @p image_path.
 *
 * Builds the DiskImages option dictionary exactly as `hdiutil attach` does and calls
 * `DIHLDiskImageAttach`. Works for any image format DiskImages recognises (UDIF `.dmg`,
 * `.sparseimage`, `.sparsebundle`, `.iso`/`.cdr`, NDIF, encrypted), not just `.dmg`.
 *
 * @param image_path Filesystem path to the disk image. May be a regular file or, for a
 *                   `.sparsebundle`, a directory.
 * @param opts Attach options, or NULL for the defaults (read-only, verified).
 * @param[out] out_results If non-NULL, receives a retained `CFDictionaryRef` on success: the
 *                   attach result whose `system-entities` key is a `CFArray` of
 *                   `{dev-entry, mount-point, content-hint}` dictionaries. The caller takes
 *                   ownership and must release it with `CFRelease()`.
 * @return 0 on success; a non-zero DiskImages status code on attach failure; or a negative value
 *         for local errors (-2 invalid path, -3 DiskImages framework load failure).
 * @note On success the DiskImages framework is left loaded for the lifetime of the process so that
 *       the attached device outlives this call.
 * @sa teth_detach(), teth_print_attach_results()
 */
int teth_attach(const char *image_path,
                const teth_attach_options *opts,
                CFDictionaryRef *out_results);

/**
 * @brief Print the `system-entities` of an attach result to stdout.
 *
 * Writes one line per entity formatted as `<dev-entry>\t<content-hint>\t<mount-point>`, matching
 * the spirit of `hdiutil attach` output. Entities without a mount point (for example a whole-disk
 * partition scheme) print an empty mount-point field.
 *
 * @param results An attach result dictionary as produced by teth_attach(). Passing NULL is a
 *                no-op.
 * @sa teth_attach()
 */
void teth_print_attach_results(CFDictionaryRef results);

/**
 * @brief Detach (unmount and eject) a previously attached image.
 *
 * Resolves @p dev_or_mount to its whole disk, then performs a whole-disk `DADiskUnmount` followed
 * by `DADiskEject` through DiskArbitration, mirroring hdiutil's `unmount_and_eject`. Operating on
 * the whole disk tears the image down, equivalent to `hdiutil detach`.
 *
 * @param dev_or_mount Target as a BSD device name (`disk4`), a `/dev` path (`/dev/disk4`,
 *                   `/dev/disk4s1`, `/dev/rdisk4s1`), or a mount point (`/Volumes/Foo`). A
 *                   partition spec still detaches the whole disk.
 * @param force If true, add the force option to the unmount.
 * @param timeout_seconds Upper bound, in seconds, on the wait for each DiskArbitration callback; a
 *                   value <= 0 selects a sane default.
 * @return 0 on success; a non-zero `DADissenter` status on unmount/eject failure; or a negative
 *         value for local errors (-2 unparseable target, -3 DiskArbitration session-create
 *         failure).
 * @sa teth_attach()
 */
int teth_detach(const char *dev_or_mount, bool force, double timeout_seconds);

#ifdef __cplusplus
}
#endif

#endif /* LIBTETHER_H */
