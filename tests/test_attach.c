// Tests for teth_attach(): local error paths, the skip-verify option, the DiskImages-failure path,
// and the success path (with and without an out-parameter).
#include "mocks.h"

#include <libtether.h>

// Stand-in DIHLDiskImageAttach symbols resolved from the bundle by the mock loader.
static OSStatus fake_attach_ok(CFDictionaryRef options, void *cb, void *ctx, CFDictionaryRef *out) {
    (void)options;
    (void)cb;
    (void)ctx;
    if (out != NULL) {
        *out = (CFDictionaryRef)TETH_FAKE_DICT;
    }
    return 0;
}

static OSStatus
fake_attach_fail(CFDictionaryRef options, void *cb, void *ctx, CFDictionaryRef *out) {
    (void)options;
    (void)cb;
    (void)ctx;
    if (out != NULL) {
        *out = (CFDictionaryRef)TETH_FAKE_DICT;
    }
    return 1;
}

static void test_attach_null_path(void **state) {
    (void)state;
    teth_mock_reset();
    assert_int_equal(teth_attach(NULL, NULL, NULL), -2);
}

static void test_attach_empty_path(void **state) {
    (void)state;
    teth_mock_reset();
    assert_int_equal(teth_attach("", NULL, NULL), -2);
}

static void test_attach_framework_load_fails(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.bundle_by_id = NULL;
    g_mock.bundle_create = NULL;
    assert_int_equal(teth_attach("/tmp", NULL, NULL), -3);
}

static void test_attach_success(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.bundle_by_id = TETH_FAKE_BUNDLE;
    g_mock.attach_fn = (void *)fake_attach_ok;

    CFDictionaryRef results = NULL;
    assert_int_equal(teth_attach("/tmp", NULL, &results), 0);
    assert_non_null((void *)results);
}

static void test_attach_success_null_out(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.bundle_by_id = TETH_FAKE_BUNDLE;
    g_mock.attach_fn = (void *)fake_attach_ok;
    // No out-parameter: teth_attach must release the result itself.
    assert_int_equal(teth_attach("/tmp", NULL, NULL), 0);
}

static void test_attach_skip_verify(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.bundle_by_id = TETH_FAKE_BUNDLE;
    g_mock.attach_fn = (void *)fake_attach_ok;
    teth_attach_options opts = {.skip_verify = true, .verbose = true, .quiet = true};
    assert_int_equal(teth_attach("/tmp", &opts, NULL), 0);
}

static void test_attach_diskimages_failure(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.bundle_by_id = TETH_FAKE_BUNDLE;
    g_mock.attach_fn = (void *)fake_attach_fail;

    CFDictionaryRef results = NULL;
    assert_int_equal(teth_attach("/tmp", NULL, &results), 1);
    assert_null((void *)results);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_attach_null_path),
        cmocka_unit_test(test_attach_empty_path),
        cmocka_unit_test(test_attach_framework_load_fails),
        cmocka_unit_test(test_attach_success),
        cmocka_unit_test(test_attach_success_null_out),
        cmocka_unit_test(test_attach_skip_verify),
        cmocka_unit_test(test_attach_diskimages_failure),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
