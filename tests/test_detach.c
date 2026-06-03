// Tests for teth_detach(): argument resolution (resolve_bsd_name) observed through the BSD name
// handed to DADiskCreateFromBSDName, plus the full unmount/eject success path.
#include "mocks.h"

#include <libtether.h>

// Expect DADiskCreateFromBSDName to receive expected_bsd, then return NULL so teth_detach reports
// "no such device" and returns -1. This makes the resolved name observable.
static void expect_bsd_then_fail(const char *expected_bsd) {
    expect_string(__wrap_DADiskCreateFromBSDName, name, expected_bsd);
    will_return(__wrap_DADiskCreateFromBSDName, NULL);
}

static void test_detach_null_arg(void **state) {
    (void)state;
    assert_int_equal(teth_detach(NULL, false, 0.0), -2);
}

static void test_detach_empty_arg(void **state) {
    (void)state;
    assert_int_equal(teth_detach("", false, 0.0), -2);
}

static void test_detach_dev_path_strips_dev(void **state) {
    (void)state;
    expect_bsd_then_fail("disk4s1");
    assert_int_equal(teth_detach("/dev/disk4s1", false, 0.0), -1);
}

static void test_detach_rdisk_becomes_disk(void **state) {
    (void)state;
    expect_bsd_then_fail("disk4");
    assert_int_equal(teth_detach("/dev/rdisk4", false, 0.0), -1);
}

static void test_detach_bare_name(void **state) {
    (void)state;
    expect_bsd_then_fail("disk7");
    assert_int_equal(teth_detach("disk7", false, 0.0), -1);
}

static void test_detach_mount_point_uses_statfs(void **state) {
    (void)state;
    will_return(__wrap_statfs, "/dev/disk9s1"); // f_mntfromname
    will_return(__wrap_statfs, 0);              // statfs return value
    expect_bsd_then_fail("disk9s1");
    assert_int_equal(teth_detach("/Volumes/Foo", false, 0.0), -1);
}

static void test_detach_success(void **state) {
    (void)state;
    expect_string(__wrap_DADiskCreateFromBSDName, name, "disk4");
    will_return(__wrap_DADiskCreateFromBSDName, TETH_FAKE_DISK);
    assert_int_equal(teth_detach("/dev/disk4", false, 0.0), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_detach_null_arg),
        cmocka_unit_test(test_detach_empty_arg),
        cmocka_unit_test(test_detach_dev_path_strips_dev),
        cmocka_unit_test(test_detach_rdisk_becomes_disk),
        cmocka_unit_test(test_detach_bare_name),
        cmocka_unit_test(test_detach_mount_point_uses_statfs),
        cmocka_unit_test(test_detach_success),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
