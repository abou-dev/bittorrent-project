#include <ctype.h>
#include <mbt/utils/str.h>
#include <mbt/utils/utils.h>
#include <mbt/utils/view.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool mbt_str_ctor(struct mbt_str *str, size_t capacity)
{
    str->data = (capacity > 0) ? malloc(capacity + 1) : NULL;
    if (capacity > 0 && !str->data)
    {
        return false;
    }

    str->size = 0;
    str->capacity = capacity;
    if (str->data)
    {
        str->data[0] = '\0';
    }
    return true;
}

void mbt_str_dtor(struct mbt_str *str)
{
    if (!str)
    {
        return;
    }

    free(str->data);
    str->data = NULL;
    str->size = 0;
    str->capacity = 0;
}

struct mbt_str *mbt_str_init(size_t capacity)
{
    struct mbt_str *str = malloc(sizeof(struct mbt_str));
    if (!str || !mbt_str_ctor(str, capacity))
    {
        free(str);
        return NULL;
    }
    return str;
}

void mbt_str_free(struct mbt_str *str)
{
    if (!str)
    {
        return;
    }

    mbt_str_dtor(str);
    free(str);
}

bool mbt_str_pushc(struct mbt_str *str, char c)
{
    if (str->size >= str->capacity)
    {
        size_t new_capacity = (str->capacity == 0) ? 1 : str->capacity * 2;
        char *new_data = realloc(str->data, new_capacity + 1);
        if (!new_data)
        {
            return false;
        }

        str->data = new_data;
        str->capacity = new_capacity;
    }

    str->data[str->size++] = c;
    str->data[str->size] = '\0';
    return true;
}

bool mbt_str_pushcstr(struct mbt_str *str, const char *cstr)
{
    if (!cstr)
    {
        return false;
    }

    size_t len = strlen(cstr);
    if (str->size + len >= str->capacity)
    {
        size_t new_capacity = str->size + len;
        char *new_data = realloc(str->data, new_capacity + 1);
        if (!new_data)
        {
            return false;
        }

        str->data = new_data;
        str->capacity = new_capacity;
    }

    memcpy(str->data + str->size, cstr, len);
    str->size += len;
    str->data[str->size] = '\0';
    return true;
}

bool mbt_str_pushcv(struct mbt_str *str, struct mbt_cview view)
{
    if (!view.data)
    {
        return false;
    }

    if (str->size + view.size >= str->capacity)
    {
        size_t new_capacity = str->size + view.size;
        char *new_data = realloc(str->data, new_capacity + 1);
        if (!new_data)
        {
            return false;
        }

        str->data = new_data;
        str->capacity = new_capacity;
    }

    memcpy(str->data + str->size, view.data, view.size);
    str->size += view.size;
    str->data[str->size] = '\0';
    return true;
}

int mbt_cview_cmp(struct mbt_cview lhs, struct mbt_cview rhs)
{
    size_t min_size = lhs.size < rhs.size ? lhs.size : rhs.size;
    int cmp = memcmp(lhs.data, rhs.data, min_size);
    if (cmp == 0)
    {
        return (int)(lhs.size - rhs.size);
    }
    return cmp;
}

bool mbt_cview_contains(struct mbt_cview view, char c)
{
    if (!view.data)
    {
        return false;
    }

    for (size_t i = 0; i < view.size; ++i)
    {
        if (view.data[i] == c)
        {
            return true;
        }
    }
    return false;
}

void mbt_cview_fprint(struct mbt_cview view, FILE *stream)
{
    if (!view.data)
    {
        return;
    }

    for (size_t i = 0; i < view.size; ++i)
    {
        unsigned char c = (unsigned char)view.data[i];

        if (isprint(c))
        {
            fputc(c, stream);
        }
        else
        {
            fprintf(stream, "U+00%02X", c);
        }
    }
}

/*void mbt_cview_fprint(struct mbt_cview view, FILE *stream)
{
    if (!view.data || view.size == 0)
    {
        return;
    }

    for (size_t i = 0; i < view.size; ++i)
    {
        unsigned char c = (unsigned char)view.data[i];

        if (c == '\0'|| c == '\r' || c == '\n')
        {
            fprintf(stream, "U+00%02X", c);
        }
        else if (isprint(c))
        {
            fputc(c, stream);
        }
        else
        {
            fprintf(stream, "U+%02X", c);
        }
    }
}*/
