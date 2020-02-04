#include <gtest/gtest.h>

#define TESTING 1
extern "C" {
#include <stdio.h>
#include <stdio.h>
#include <qpid/dispatch/router_core.h>
#include <../src/router_core/router_core_private.h>
int safe_snprintf(char *str, size_t size, const char *format, ...);
}

#define OUTPUT_SIZE 128
#define TEST_MESSAGE "something"
#define LEN strlen(TEST_MESSAGE)

TEST(test_safe_snprintf, valid_input) {
    //size_t size = OUTPUT_SIZE;
    size_t len;
    char output[OUTPUT_SIZE];

    len = safe_snprintf(output, LEN+10, TEST_MESSAGE);
    ASSERT_EQ(LEN, len);
    ASSERT_STREQ(output, TEST_MESSAGE);

    len = safe_snprintf(output, LEN+1, TEST_MESSAGE);
    ASSERT_EQ(LEN, len);
    ASSERT_STREQ(output, TEST_MESSAGE);

    len = safe_snprintf(output, LEN, TEST_MESSAGE);
    ASSERT_EQ(LEN-1, len);
    ASSERT_STREQ(output, "somethin");

    len = safe_snprintf(output, 0, TEST_MESSAGE);
    //ASSERT_EQ(LEN, len); // Is this correct? ???
    ASSERT_EQ(0, len); // Is this correct? ???

    len = safe_snprintf(output, 1, TEST_MESSAGE);
    ASSERT_EQ(0, len);

    len = safe_snprintf(output, (int)-1, TEST_MESSAGE);
    ASSERT_EQ(0, len); //or worst negative?
}

TEST(test_safe_snprintf, invalid_input) {
    //no way to make it fail, we can mock it if necessary.
}

TEST(test_qdr_terminus_format, coordinator) {
    qdr_terminus_t t;
#define SIZE 128
#define EXPECTED "{<coordinator>}"
#define EXPECTED_LEN strlen(EXPECTED)
    size_t size = SIZE;
    char output[SIZE];

    t.coordinator=true;

    qdr_terminus_format(&t, output, &size);
    ASSERT_STREQ(output, EXPECTED);
    //EXPECT_STREQ(output, "wrong_but_continues");
    ASSERT_EQ(size, SIZE - EXPECTED_LEN);

#undef SIZE
#undef EXPECTED
#undef EXPECTED_LEN
}

TEST(test_qdr_terminus_format, empty) {
    char output[3];
    size_t size = 3;
    output[2]='A';

    qdr_terminus_format(NULL, output, &size);

    ASSERT_STREQ(output, "{}");
    ASSERT_EQ(size, 1);
}

//int main(int argc, char **argv) {
    //testing::InitGoogleTest(&argc, argv);
    //return RUN_ALL_TESTS();
//}
