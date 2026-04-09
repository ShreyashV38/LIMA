#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/data_structures/gap_buffer.h"

void test_gb_basic() {
    GapBuffer *gb = gb_create(16);
    assert(gb != NULL);
    assert(gb_size(gb) == 0);

    // Insert "Hello"
    assert(gb_insert(gb, "Hello", 5));
    size_t len;
    char *text = gb_get_text(gb, &len);
    assert(strcmp(text, "Hello") == 0);
    assert(len == 5);
    free(text);

    // Insert " World" at end
    assert(gb_insert(gb, " World", 6));
    text = gb_get_text(gb, &len);
    assert(strcmp(text, "Hello World") == 0);
    assert(len == 11);
    free(text);

    // Move cursor and insert
    gb_move_cursor(gb, 5);
    assert(gb_insert(gb, "!", 1));
    text = gb_get_text(gb, &len);
    assert(strcmp(text, "Hello! World") == 0);
    assert(len == 12);
    free(text);

    // Delete before cursor
    gb_move_cursor(gb, 6); // After the '!'
    gb_delete_before(gb, 1);
    text = gb_get_text(gb, &len);
    assert(strcmp(text, "Hello World") == 0);
    free(text);

    // Delete after cursor
    gb_move_cursor(gb, 5); // After "Hello"
    gb_delete_after(gb, 6); // Delete " World"
    text = gb_get_text(gb, &len);
    assert(strcmp(text, "Hello") == 0);
    free(text);

    gb_destroy(gb);
    printf("test_gb_basic passed!\n");
}

int main() {
    printf("Running gap buffer tests...\n");
    test_gb_basic();
    printf("All gap buffer tests passed!\n");
    return 0;
}
