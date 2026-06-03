// Tests for teth_detach(): BSD-name resolution (observed via DADiskCreateFromBSDName), the full
// unmount/eject success path, and the failure paths (dissent, timeout, no session, bad target).
#include "mocks.h"

#include <libtether.h>

// Expect DADiskCreateFromBSDName to receive expected_bsd.
static void expect_bsd(const char *expected_bsd) {
    expect_string(__wrap_DADiskCreateFromBSDName, name, expected_bsd);
}

static void test_detach_null_arg(void **state) {
    (void)state;
    teth_mock_reset();
    assert_int_equal(teth_detach(NULL, false, 0.0), -2);
}

static void test_detach_empty_arg(void **state) {
    (void)state;
    teth_mock_reset();
    assert_int_equal(teth_detach("", false, 0.0), -2);
}

static void test_detach_dev_path_strips_dev(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.disk_create_null = 1; // no such device -> -1
    expect_bsd("disk4s1");
    assert_int_equal(teth_detach("/dev/disk4s1", false, 0.0), -1);
}

static void test_detach_rdisk_becomes_disk(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.disk_create_null = 1;
    expect_bsd("disk4");
    assert_int_equal(teth_detach("/dev/rdisk4", false, 0.0), -1);
}

static void test_detach_bare_name(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.disk_create_null = 1;
    expect_bsd("disk7");
    assert_int_equal(teth_detach("disk7", false, 0.0), -1);
}

static void test_detach_mount_point_uses_statfs(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.disk_create_null = 1;
    will_return(__wrap_statfs, "/dev/disk9s1"); // f_mntfromname
    will_return(__wrap_statfs, 0);              // statfs return value
    expect_bsd("disk9s1");
    assert_int_equal(teth_detach("/Volumes/Foo", false, 0.0), -1);
}

static void test_detach_mount_point_rdisk(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.disk_create_null = 1;
    will_return(__wrap_statfs, "/dev/rdisk9s1"); // exercises the rdisk -> disk strip
    will_return(__wrap_statfs, 0);
    expect_bsd("disk9s1");
    assert_int_equal(teth_detach("/Volumes/Bar", false, 0.0), -1);
}

static void test_detach_unresolvable_target(void **state) {
    (void)state;
    teth_mock_reset();
    will_return(__wrap_statfs, NULL); // f_mntfromname unused
    will_return(__wrap_statfs, -1);   // statfs fails
    assert_int_equal(teth_detach("not-a-device", false, 0.0), -2);
}

static void test_detach_session_create_fails(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.session_null = 1; // DASessionCreate returns NULL after a good resolve
    assert_int_equal(teth_detach("/dev/disk4", false, 0.0), -3);
}

static void test_detach_success(void **state) {
    (void)state;
    teth_mock_reset();
    expect_bsd("disk4");
    assert_int_equal(teth_detach("/dev/disk4", false, 0.0), 0);
}

static void test_detach_force(void **state) {
    (void)state;
    teth_mock_reset();
    expect_bsd("disk4");
    assert_int_equal(teth_detach("/dev/disk4", true, 0.0), 0);
}

static void test_detach_unmount_dissents(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.unmount_dissenter = TETH_FAKE_DISSENTER;
    g_mock.dissenter_status = 5;
    g_mock.dissenter_string = (const void *)"unmount refused";
    expect_bsd("disk4");
    assert_int_equal(teth_detach("/dev/disk4", false, 0.0), 5);
}

static void test_detach_unmount_dissents_no_message(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.unmount_dissenter = TETH_FAKE_DISSENTER;
    g_mock.dissenter_status = 6;
    g_mock.dissenter_string = NULL; // forces the snprintf fallback
    expect_bsd("disk4");
    assert_int_equal(teth_detach("/dev/disk4", false, 0.0), 6);
}

static void test_detach_unmount_times_out(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.unmount_fire = 0; // callback never delivered -> run loop times out
    expect_bsd("disk4");
    assert_int_equal(teth_detach("/dev/disk4", false, 1.0), -1);
}

static void test_detach_eject_dissents(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.eject_dissenter = TETH_FAKE_DISSENTER; // unmount succeeds, eject fails
    g_mock.dissenter_status = 7;
    g_mock.dissenter_string = (const void *)"eject refused";
    expect_bsd("disk4");
    assert_int_equal(teth_detach("/dev/disk4", false, 0.0), 7);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_detach_null_arg),
        cmocka_unit_test(test_detach_empty_arg),
        cmocka_unit_test(test_detach_dev_path_strips_dev),
        cmocka_unit_test(test_detach_rdisk_becomes_disk),
        cmocka_unit_test(test_detach_bare_name),
        cmocka_unit_test(test_detach_mount_point_uses_statfs),
        cmocka_unit_test(test_detach_mount_point_rdisk),
        cmocka_unit_test(test_detach_unresolvable_target),
        cmocka_unit_test(test_detach_session_create_fails),
        cmocka_unit_test(test_detach_success),
        cmocka_unit_test(test_detach_force),
        cmocka_unit_test(test_detach_unmount_dissents),
        cmocka_unit_test(test_detach_unmount_dissents_no_message),
        cmocka_unit_test(test_detach_unmount_times_out),
        cmocka_unit_test(test_detach_eject_dissents),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
