#include <criterion/criterion.h>
#include "mbt/utils/str.h"
#include "mbt/utils/view.h"
#include "mbt/utils/utils.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Test mbt_str initialization and getters
Test(mbtutils, str_init_and_getters)
{
    struct mbt_str *str = mbt_str_init(10);
    cr_assert_not_null(str, "Failed to initialize mbt_str");

    cr_assert_eq(str->size, 0, "Initial size should be 0");
    cr_assert_geq(str->capacity, 10, "Initial capacity should be at least 10");

    mbt_str_free(str);
}

// Test mbt_str pushc and get data
Test(mbtutils, str_pushc)
{
    struct mbt_str *str = mbt_str_init(2);
    cr_assert_not_null(str, "Failed to initialize mbt_str");

    cr_assert(mbt_str_pushc(str, 'a'), "Failed to push character 'a'");
    cr_assert(mbt_str_pushc(str, 'b'), "Failed to push character 'b'");
    cr_assert(mbt_str_pushc(str, 'c'), "Failed to push character 'c' with resize");

    cr_assert_eq(str->size, 3, "Size should be 3");
    cr_assert_eq(str->data[0], 'a', "First character should be 'a'");
    cr_assert_eq(str->data[1], 'b', "Second character should be 'b'");
    cr_assert_eq(str->data[2], 'c', "Third character should be 'c'");

    mbt_str_free(str);
}

// Test mbt_str pushcstr and get data
Test(mbtutils, str_pushcstr)
{
    struct mbt_str *str = mbt_str_init(5);
    cr_assert_not_null(str, "Failed to initialize mbt_str");

    cr_assert(mbt_str_pushcstr(str, "hello"), "Failed to push C-string 'hello'");
    cr_assert_eq(str->size, 5, "Size should be 5");
    cr_assert_str_eq(str->data, "hello", "String data mismatch");

    cr_assert(mbt_str_pushcstr(str, " world"), "Failed to push C-string ' world'");
    cr_assert_eq(str->size, 11, "Size should be 11");
    cr_assert_str_eq(str->data, "hello world", "String data mismatch");

    mbt_str_free(str);
}

// Test mbt_str pushcv and get data
Test(mbtutils, str_pushcv)
{
    struct mbt_str *str = mbt_str_init(5);
    cr_assert_not_null(str, "Failed to initialize mbt_str");

    struct mbt_cview view = MBT_CVIEW_LIT("data");
    cr_assert(mbt_str_pushcv(str, view), "Failed to push cview 'data'");

    cr_assert_eq(str->size, 4, "Size should be 4");
    cr_assert_str_eq(str->data, "data", "String data mismatch");

    struct mbt_cview view2 = MBT_CVIEW_LIT("123");
    cr_assert(mbt_str_pushcv(str, view2), "Failed to push cview '123'");

    cr_assert_eq(str->size, 7, "Size should be 7");
    cr_assert_str_eq(str->data, "data123", "String data mismatch");

    mbt_str_free(str);
}

// Test mbt_cview_cmp
Test(mbtutils, cview_cmp)
{
    struct mbt_cview view1 = MBT_CVIEW_LIT("abc");
    struct mbt_cview view2 = MBT_CVIEW_LIT("abc");
    struct mbt_cview view3 = MBT_CVIEW_LIT("abd");
    struct mbt_cview view4 = MBT_CVIEW_LIT("ab");

    cr_assert_eq(mbt_cview_cmp(view1, view2), 0, "Views should be equal");
    cr_assert_gt(mbt_cview_cmp(view3, view1), 0, "view3 should be greater than view1");
    cr_assert_lt(mbt_cview_cmp(view4, view1), 0, "view4 should be less than view1");
}

// Test mbt_cview_contains
Test(mbtutils, cview_contains)
{
    struct mbt_cview view = MBT_CVIEW_LIT("hello");

    cr_assert(mbt_cview_contains(view, 'e'), "Should contain 'e'");
    cr_assert(mbt_cview_contains(view, 'h'), "Should contain 'h'");
    cr_assert(mbt_cview_contains(view, 'o'), "Should contain 'o'");
    cr_assert_not(mbt_cview_contains(view, 'a'), "Should not contain 'a'");
}

// Test mbt_cview_fprint
Test(mbtutils, cview_fprint)
{
    struct mbt_cview view = MBT_CVIEW_LIT("Hello\nWorld\t!");

    FILE *stream = tmpfile();
    cr_assert_not_null(stream, "Failed to create temporary file");

    mbt_cview_fprint(view, stream);

    rewind(stream);
    char buffer[256];
    size_t read_size = fread(buffer, 1, sizeof(buffer) - 1, stream);
    buffer[read_size] = '\0';

    cr_assert_str_eq(buffer, "{Hello\\u000aWorld\\u0009!}", "cview_fprint output mismatch");

    fclose(stream);
}

