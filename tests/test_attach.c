// Tests for teth_attach(): the local error paths (bad path, framework load failure) and the full
// success path, where the DIHLDiskImageAttach symbol resolved from the bundle is a test stub.
#include "mocks.h"

#include <libtether.h>

// Stand-in for the private DIHLDiskImageAttach. __wrap_CFBundleGetFunctionPointerForName hands this
// back for the "DIHLDiskImageAttach" symbol; it reports success and returns a fake result dict.
static OSStatus fake_attach(CFDictionaryRef options, void *cb, void *ctx, CFDictionaryRef *out) {
    (void)options;
    (void)cb;
    (void)ctx;
    if (out != NULL) {
        *out = (CFDictionaryRef)TETH_FAKE_DICT;
    }
    return 0;
}

static void test_attach_null_path(void **state) {
    (void)state;
    assert_int_equal(teth_attach(NULL, NULL, NULL), -2);
}

static void test_attach_empty_path(void **state) {
    (void)state;
    assert_int_equal(teth_attach("", NULL, NULL), -2);
}

static void test_attach_framework_load_fails(void **state) {
    (void)state;
    will_return(__wrap_CFBundleGetBundleWithIdentifier, NULL);
    will_return(__wrap_CFBundleCreate, NULL);
    assert_int_equal(teth_attach("/tmp", NULL, NULL), -3);
}

static void test_attach_success(void **state) {
    (void)state;
    g_diskimages_attach_fn = (void *)fake_attach;
    will_return(__wrap_CFBundleGetBundleWithIdentifier, TETH_FAKE_BUNDLE);

    CFDictionaryRef results = NULL;
    assert_int_equal(teth_attach("/tmp", NULL, &results), 0);
    assert_non_null((void *)results);

    g_diskimages_attach_fn = NULL;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_attach_null_path),
        cmocka_unit_test(test_attach_empty_path),
        cmocka_unit_test(test_attach_framework_load_fails),
        cmocka_unit_test(test_attach_success),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
