#include "../../include/data_structures/gap_buffer.h"
#include <stdlib.h>
#include <string.h>

struct GapBuffer {
    char *buf;
    size_t capacity;
    size_t gap_start;
    size_t gap_end;
};

static bool gb_expand(GapBuffer *gb, size_t min_needed) {
    size_t old_gap_size = gb->gap_end - gb->gap_start;
    if (old_gap_size >= min_needed) return true;

    size_t new_capacity = gb->capacity == 0 ? 128 : gb->capacity * 2;
    while (new_capacity - (gb->capacity - old_gap_size) < min_needed) {
        new_capacity *= 2;
    }

    char *new_buf = realloc(gb->buf, new_capacity);
    if (!new_buf) return false;
    
    gb->buf = new_buf;
    
    // Move the text after the gap to the end of the new buffer
    size_t move_len = gb->capacity - gb->gap_end;
    size_t new_gap_end = new_capacity - move_len;
    
    if (move_len > 0) {
        memmove(gb->buf + new_gap_end, gb->buf + gb->gap_end, move_len);
    }
    
    gb->gap_end = new_gap_end;
    gb->capacity = new_capacity;
    
    return true;
}

GapBuffer *gb_create(size_t initial_capacity) {
    GapBuffer *gb = malloc(sizeof(GapBuffer));
    if (!gb) return NULL;
    
    if (initial_capacity < 16) initial_capacity = 16;
    
    gb->buf = malloc(initial_capacity);
    if (!gb->buf) {
        free(gb);
        return NULL;
    }
    
    gb->capacity = initial_capacity;
    gb->gap_start = 0;
    gb->gap_end = initial_capacity;
    
    return gb;
}

void gb_destroy(GapBuffer *gb) {
    if (gb) {
        free(gb->buf);
        free(gb);
    }
}

bool gb_insert(GapBuffer *gb, const char *data, size_t len) {
    if (!gb_expand(gb, len)) return false;
    
    memcpy(gb->buf + gb->gap_start, data, len);
    gb->gap_start += len;
    return true;
}

void gb_delete_before(GapBuffer *gb, size_t len) {
    if (gb->gap_start < len) {
        gb->gap_start = 0;
    } else {
        gb->gap_start -= len;
    }
}

void gb_delete_after(GapBuffer *gb, size_t len) {
    size_t chars_after = gb->capacity - gb->gap_end;
    if (chars_after < len) {
        gb->gap_end = gb->capacity;
    } else {
        gb->gap_end += len;
    }
}

void gb_move_cursor(GapBuffer *gb, size_t pos) {
    size_t current_len = gb_size(gb);
    if (pos > current_len) pos = current_len;
    
    if (pos < gb->gap_start) {
        size_t dist = gb->gap_start - pos;
        memmove(gb->buf + gb->gap_end - dist, gb->buf + pos, dist);
        gb->gap_start -= dist;
        gb->gap_end -= dist;
    } else if (pos > gb->gap_start) {
        size_t dist = pos - gb->gap_start;
        memmove(gb->buf + gb->gap_start, gb->buf + gb->gap_end, dist);
        gb->gap_start += dist;
        gb->gap_end += dist;
    }
}

void gb_move_cursor_relative(GapBuffer *gb, int offset) {
    if (offset == 0) return;
    
    if (offset < 0) {
        size_t abs_offset = (size_t)(-offset);
        if (abs_offset > gb->gap_start) {
            gb_move_cursor(gb, 0);
        } else {
            gb_move_cursor(gb, gb->gap_start - abs_offset);
        }
    } else {
        size_t total_size = gb_size(gb);
        size_t new_pos = gb->gap_start + offset;
        if (new_pos > total_size) {
            gb_move_cursor(gb, total_size);
        } else {
            gb_move_cursor(gb, new_pos);
        }
    }
}

char *gb_get_text(const GapBuffer *gb, size_t *out_len) {
    size_t s = gb_size(gb);
    char *text = malloc(s + 1);
    if (!text) return NULL;
    
    if (gb->gap_start > 0) {
        memcpy(text, gb->buf, gb->gap_start);
    }
    
    size_t end_len = gb->capacity - gb->gap_end;
    if (end_len > 0) {
        memcpy(text + gb->gap_start, gb->buf + gb->gap_end, end_len);
    }
    
    text[s] = '\0';
    if (out_len) *out_len = s;
    
    return text;
}

size_t gb_size(const GapBuffer *gb) {
    return gb->capacity - (gb->gap_end - gb->gap_start);
}
