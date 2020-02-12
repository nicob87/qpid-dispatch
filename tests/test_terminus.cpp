#include "qdr_doctest.h"

#define TESTING 1 //not used?
extern "C" {
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <qpid/dispatch/router_core.h>
#include <../src/router_core/router_core_private.h>
int safe_snprintf(char *str, size_t size, const char *format, ...);
void mock(bool m);
void mocked_value(int v);
}

#define OUTPUT_SIZE 128
#define TEST_MESSAGE "something"
#define LEN strlen(TEST_MESSAGE)

TEST_CASE("test_safe_snprintf_valid_input") {
    //size_t size = OUTPUT_SIZE;
    size_t len;
    char output[OUTPUT_SIZE];

    len = safe_snprintf(output, LEN+10, TEST_MESSAGE);
    CHECK(LEN == len);
    CHECK(output == TEST_MESSAGE);

    len = safe_snprintf(output, LEN+1, TEST_MESSAGE);
    CHECK(LEN == len);
    CHECK(output == TEST_MESSAGE);

    len = safe_snprintf(output, LEN, TEST_MESSAGE);
    CHECK(LEN-1 == len);
    CHECK(output == "somethin");

    len = safe_snprintf(output, 0, TEST_MESSAGE);
    CHECK(0 == len);

    len = safe_snprintf(output, 1, TEST_MESSAGE);
    CHECK(0 == len);

    len = safe_snprintf(output, (int)-1, TEST_MESSAGE);
    CHECK(0 == len); //or worst negative?

    mock(true);
    mocked_value(-1);

    len = safe_snprintf(output, LEN+10, TEST_MESSAGE);
    CHECK(0 == len);
    mock(false);
}

TEST_CASE("test_safe_snprintf_invalid_input") {
    //no way to make it fail, we can mock it if necessary.
}

TEST_CASE("test_qdr_terminus_format_coordinator") {
    qdr_terminus_t t;
#define SIZE 128
#define EXPECTED "{<coordinator>}"
#define EXPECTED_LEN strlen(EXPECTED)
    size_t size = SIZE;
    char output[SIZE];

    t.coordinator=true;

    qdr_terminus_format(&t, output, &size);
    CHECK(output == EXPECTED);
    //EXPECT_STREQ(output, "wrong_but_continues");
    CHECK(size == SIZE - EXPECTED_LEN);

#undef SIZE
#undef EXPECTED
#undef EXPECTED_LEN
}

TEST_CASE("test_qdr_terminus_format_empty") {
    char output[3];
    size_t size = 3;
    output[2]='A';

    qdr_terminus_format(NULL, output, &size);

    SUBCASE("Sample subcase 1") {
    CHECK(output == "{}");
    }
    SUBCASE("Sample subcase 2") {
    CHECK(size == 1);
    }
}
