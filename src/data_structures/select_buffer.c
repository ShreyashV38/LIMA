#include "../../include/data_structures/select_buffer.h"
#include <stdlib.h>
#include <string.h>

struct SelectBuffer {
    char  *data;      /* dynamically allocated char array */
    size_t length;    /* number of valid characters       */
    size_t capacity;  /* total allocated bytes             */
};

/* ------------------------------------------------------------------ */

SelectBuffer *sb_create(void) {
    SelectBuffer *sb = malloc(sizeof(SelectBuffer));
    if (!sb) return NULL;

    sb->capacity = 64;
    sb->length   = 0;
    sb->data     = malloc(sb->capacity);
    if (!sb->data) {
        free(sb);
        return NULL;
    }
    sb->data[0] = '\0';
    return sb;
}

void sb_destroy(SelectBuffer *sb) {
    if (!sb) return;
    free(sb->data);
    free(sb);
}

/* ------------------------------------------------------------------ */

bool sb_store(SelectBuffer *sb, const char *text, size_t len) {
    if (!sb || !text) return false;

    /* Ensure enough room (len + 1 for null terminator) */
    if (len + 1 > sb->capacity) {
        size_t new_cap = sb->capacity;
        while (new_cap < len + 1) {
            new_cap *= 2;
        }
        char *new_data = realloc(sb->data, new_cap);
        if (!new_data) return false;
        sb->data     = new_data;
        sb->capacity = new_cap;
    }

    memcpy(sb->data, text, len);
    sb->data[len] = '\0';
    sb->length    = len;
    return true;
}

void sb_clear(SelectBuffer *sb) {
    if (!sb) return;
    sb->length  = 0;
    if (sb->data) sb->data[0] = '\0';
}

/* ------------------------------------------------------------------ */

const char *sb_get_text(const SelectBuffer *sb, size_t *out_len) {
    if (!sb || sb->length == 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    if (out_len) *out_len = sb->length;
    return sb->data;
}

bool sb_is_empty(const SelectBuffer *sb) {
    return !sb || sb->length == 0;
}

size_t sb_length(const SelectBuffer *sb) {
    return sb ? sb->length : 0;
}
