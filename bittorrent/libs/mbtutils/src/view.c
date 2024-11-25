#include "mbt/utils/view.h"
#include "mbt/utils/utils.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int mbt_cview_cmp(struct mbt_cview lhs, struct mbt_cview rhs)
{
    size_t min_size = lhs.size < rhs.size ? lhs.size : rhs.size;
    int cmp = memcmp(lhs.data, rhs.data, min_size);
    if (cmp != 0)
        return cmp;
    if (lhs.size < rhs.size)
        return -1;
    if (lhs.size > rhs.size)
        return 1;
    return 0;
}

bool mbt_cview_contains(struct mbt_cview view, char c)
{
    if (view.data == NULL)
        return false;

    for (size_t i = 0; i < view.size; i++) {
        if (view.data[i] == c)
            return true;
    }
    return false;
}

void mbt_cview_fprint(struct mbt_cview view, FILE *stream)
{
    if (stream == NULL)
        return;

    fputc('{', stream);
    for (size_t i = 0; i < view.size; i++) {
        char c = view.data[i];
        if (isprint((unsigned char)c)) {
            if (c == '\\' || c == '\"') {
                fputc('\\', stream);
                fputc(c, stream);
            } else {
                fputc(c, stream);
            }
        } else {
            fprintf(stream, "\\u%04x", (unsigned char)c);
        }
    }
    fputc('}', stream);
}

