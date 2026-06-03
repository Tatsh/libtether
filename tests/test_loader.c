// Tests for the DiskImages CFBundle loader: diskimages_loader_init() and
// diskimages_loader_dispose().
#include "mocks.h"

#include <string.h>

#include "diskimages_private.h"

static int fake_init(void) {
    return 0;
}

static void fake_deinit(void) {
}

static OSStatus fake_attach(CFDictionaryRef options, void *cb, void *ctx, CFDictionaryRef *out) {
    (void)options;
    (void)cb;
    (void)ctx;
    (void)out;
    return 0;
}

static void test_loader_null_api_with_err(void **state) {
    (void)state;
    teth_mock_reset();
    const char *err = NULL;
    assert_int_equal(diskimages_loader_init(NULL, &err), -1);
    assert_non_null(err);
}

static void test_loader_null_api_no_err(void **state) {
    (void)state;
    teth_mock_reset();
    assert_int_equal(diskimages_loader_init(NULL, NULL), -1);
}

static void test_loader_bundle_url_fails(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.bundle_by_id = NULL;
    g_mock.url_null = 1; // CFURLCreateWithFileSystemPath returns NULL, so no bundle
    diskimages_api api;
    const char *err = NULL;
    assert_int_equal(diskimages_loader_init(&api, &err), -1);
    assert_non_null(err);
}

static void test_loader_symbol_missing(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.bundle_by_id = TETH_FAKE_BUNDLE;
    g_mock.attach_fn = NULL; // DIHLDiskImageAttach not found -> dispose and fail
    diskimages_api api;
    const char *err = NULL;
    assert_int_equal(diskimages_loader_init(&api, &err), -1);
    assert_non_null(err);
}

static void test_loader_success_runs_initialize(void **state) {
    (void)state;
    teth_mock_reset();
    g_mock.bundle_by_id = TETH_FAKE_BUNDLE;
    g_mock.attach_fn = (void *)fake_attach;
    g_mock.init_fn = (void *)fake_init; // present, so DIInitialize is called
    diskimages_api api;
    const char *err = NULL;
    assert_int_equal(diskimages_loader_init(&api, &err), 0);
    assert_null(err);
    assert_non_null((void *)api.DIHLDiskImageAttach);
}

static void test_loader_dispose_null(void **state) {
    (void)state;
    teth_mock_reset();
    diskimages_loader_dispose(NULL); // safe no-op
}

static void test_loader_dispose_releases(void **state) {
    (void)state;
    teth_mock_reset();
    diskimages_api api;
    memset(&api, 0, sizeof api);
    api.bundle = (CFBundleRef)TETH_FAKE_BUNDLE;
    api.DIDeinitialize = fake_deinit; // present, so DIDeinitialize is called
    diskimages_loader_dispose(&api);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_loader_null_api_with_err),
        cmocka_unit_test(test_loader_null_api_no_err),
        cmocka_unit_test(test_loader_bundle_url_fails),
        cmocka_unit_test(test_loader_symbol_missing),
        cmocka_unit_test(test_loader_success_runs_initialize),
        cmocka_unit_test(test_loader_dispose_null),
        cmocka_unit_test(test_loader_dispose_releases),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
