// Tests for teth_print_attach_results() and its cf_string_to_c helper.
#include "mocks.h"

#include <libtether.h>

static void test_print_null(void **state) {
    (void)state;
    teth_mock_reset();
    teth_print_attach_results(NULL); // no-op
}

static void test_print_not_an_array(void **state) {
    (void)state;
    teth_mock_reset();
    will_return(__wrap_CFDictionaryGetValue, TETH_FAKE_DICT); // system-entities present
    will_return(__wrap_CFGetTypeID, TETH_DICT_TYPE_ID);       // ... but not an array
    teth_print_attach_results((CFDictionaryRef)TETH_FAKE_DICT);
}

static void test_print_entities(void **state) {
    (void)state;
    teth_mock_reset();
    // system-entities lookup returns a fake array.
    will_return(__wrap_CFDictionaryGetValue, (void *)0xA22A);
    will_return(__wrap_CFGetTypeID, TETH_ARRAY_TYPE_ID);
    will_return(__wrap_CFArrayGetCount, 2);
    // Entity 0 is a dictionary: a normal dev-entry, a NULL content-hint, and a mount-point that
    // cannot be converted. This exercises all three cf_string_to_c outcomes.
    will_return(__wrap_CFArrayGetValueAtIndex, (void *)0xE0);
    will_return(__wrap_CFGetTypeID, TETH_DICT_TYPE_ID);
    will_return(__wrap_CFDictionaryGetValue, (void *)"disk2s1");
    will_return(__wrap_CFDictionaryGetValue, NULL);
    will_return(__wrap_CFDictionaryGetValue, (void *)TETH_FAKE_BADSTR);
    // Entity 1 is not a dictionary and is skipped.
    will_return(__wrap_CFArrayGetValueAtIndex, (void *)0xE1);
    will_return(__wrap_CFGetTypeID, TETH_ARRAY_TYPE_ID);
    teth_print_attach_results((CFDictionaryRef)TETH_FAKE_DICT);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_print_null),
        cmocka_unit_test(test_print_not_an_array),
        cmocka_unit_test(test_print_entities),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
