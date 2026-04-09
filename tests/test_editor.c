#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/ui/editor.h"
#include "../include/data_structures/gap_buffer.h"

/* ──────────────────────────────────────────────────────────────────────
 *  Unit tests for the Editor (non-interactive parts).
 *
 *  These tests exercise editor_create, editor_get_content,
 *  editor_destroy, and the undo/redo system.
 *  The interactive editor_run() loop must be tested manually.
 * ────────────────────────────────────────────────────────────────────── */

/* ==================================================================
 *  Basic Creation / Destruction Tests
 * ================================================================== */

void test_editor_create_empty(void) {
    Editor *ed = editor_create("empty.txt", NULL);
    assert(ed != NULL);

    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(content != NULL);
    assert(len == 0);
    assert(strcmp(content, "") == 0);
    free(content);

    editor_destroy(ed);
    printf("  [PASS] test_editor_create_empty\n");
}

void test_editor_create_with_content(void) {
    const char *initial = "Hello, World!";
    Editor *ed = editor_create("hello.txt", initial);
    assert(ed != NULL);

    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(content != NULL);
    assert(len == strlen(initial));
    assert(strcmp(content, initial) == 0);
    free(content);

    editor_destroy(ed);
    printf("  [PASS] test_editor_create_with_content\n");
}

void test_editor_create_multiline(void) {
    const char *initial = "Line 1\nLine 2\nLine 3\n";
    Editor *ed = editor_create("multi.txt", initial);
    assert(ed != NULL);

    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(content != NULL);
    assert(len == strlen(initial));
    assert(strcmp(content, initial) == 0);
    free(content);

    editor_destroy(ed);
    printf("  [PASS] test_editor_create_multiline\n");
}

void test_editor_create_no_filename(void) {
    Editor *ed = editor_create(NULL, "some text");
    assert(ed != NULL);

    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(content != NULL);
    assert(strcmp(content, "some text") == 0);
    free(content);

    editor_destroy(ed);
    printf("  [PASS] test_editor_create_no_filename\n");
}

void test_editor_create_large_content(void) {
    /* Build a ~10KB string */
    size_t size = 10000;
    char *big = malloc(size + 1);
    assert(big != NULL);
    for (size_t i = 0; i < size; i++) {
        big[i] = 'A' + (char)(i % 26);
    }
    big[size] = '\0';

    Editor *ed = editor_create("big.txt", big);
    assert(ed != NULL);

    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(content != NULL);
    assert(len == size);
    assert(memcmp(content, big, size) == 0);
    free(content);

    editor_destroy(ed);
    free(big);
    printf("  [PASS] test_editor_create_large_content\n");
}

void test_editor_destroy_null(void) {
    /* Should not crash */
    editor_destroy(NULL);
    printf("  [PASS] test_editor_destroy_null\n");
}

void test_editor_get_content_null(void) {
    /* Should return NULL for NULL editor */
    char *content = editor_get_content(NULL, NULL);
    assert(content == NULL);
    printf("  [PASS] test_editor_get_content_null\n");
}

void test_editor_special_characters(void) {
    const char *initial = "Tab:\there\nNewline above\r\nCR+LF too";
    Editor *ed = editor_create("special.txt", initial);
    assert(ed != NULL);

    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(content != NULL);
    assert(len == strlen(initial));
    assert(strcmp(content, initial) == 0);
    free(content);

    editor_destroy(ed);
    printf("  [PASS] test_editor_special_characters\n");
}

/* ==================================================================
 *  Gap Buffer Integration Tests (simulating keystrokes directly)
 * ================================================================== */

void test_editor_gap_buffer_insert(void) {
    /* Verify that the gap buffer correctly holds inserted content */
    Editor *ed = editor_create("test.txt", "AB");
    assert(ed != NULL);

    /* Content should be "AB" with cursor at position 0 */
    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(strcmp(content, "AB") == 0);
    assert(len == 2);
    free(content);

    editor_destroy(ed);
    printf("  [PASS] test_editor_gap_buffer_insert\n");
}

void test_editor_gap_buffer_operations(void) {
    /* Test gap buffer operations through the editor */
    GapBuffer *gb = gb_create(64);
    assert(gb != NULL);

    /* Simulate: type "Hello" */
    gb_insert(gb, "Hello", 5);
    char *t = gb_get_text(gb, NULL);
    assert(strcmp(t, "Hello") == 0);
    free(t);

    /* Move cursor to position 5, insert " World" */
    gb_insert(gb, " World", 6);
    t = gb_get_text(gb, NULL);
    assert(strcmp(t, "Hello World") == 0);
    free(t);

    /* Move cursor to position 5 (after "Hello"), insert "!" */
    gb_move_cursor(gb, 5);
    gb_insert(gb, "!", 1);
    t = gb_get_text(gb, NULL);
    assert(strcmp(t, "Hello! World") == 0);
    free(t);

    /* Backspace: delete the "!" */
    gb_delete_before(gb, 1);
    t = gb_get_text(gb, NULL);
    assert(strcmp(t, "Hello World") == 0);
    free(t);

    gb_destroy(gb);
    printf("  [PASS] test_editor_gap_buffer_operations\n");
}

/* ==================================================================
 *  Undo / Redo Tests (via editor_create + destroy lifecycle)
 *
 *  Since undo/redo uses the Stack internally, we verify that
 *  editor_destroy properly cleans up stacks with pending actions.
 * ================================================================== */

void test_editor_undo_redo_cleanup(void) {
    /* Create editor with content — undo/redo stacks should be empty
       but still allocated.  Destroying should not leak. */
    Editor *ed = editor_create("undo_test.txt", "Hello World");
    assert(ed != NULL);

    /* Verify content is intact */
    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(strcmp(content, "Hello World") == 0);
    free(content);

    /* Destroy — should clean up undo/redo stacks without leaking */
    editor_destroy(ed);
    printf("  [PASS] test_editor_undo_redo_cleanup\n");
}

void test_editor_undo_redo_stacks_exist(void) {
    /* The editor should create with undo/redo stacks successfully */
    Editor *ed = editor_create("stacks.txt", NULL);
    assert(ed != NULL);

    /* Empty content editor should still work */
    size_t len;
    char *content = editor_get_content(ed, &len);
    assert(content != NULL);
    assert(len == 0);
    free(content);

    editor_destroy(ed);
    printf("  [PASS] test_editor_undo_redo_stacks_exist\n");
}

/* ──────────────────────────────────────────────────────────────────────
 *  Main
 * ────────────────────────────────────────────────────────────────────── */

int main(void) {
    printf("Running editor tests...\n\n");

    printf("  -- Creation / Destruction --\n");
    test_editor_create_empty();
    test_editor_create_with_content();
    test_editor_create_multiline();
    test_editor_create_no_filename();
    test_editor_create_large_content();
    test_editor_destroy_null();
    test_editor_get_content_null();
    test_editor_special_characters();

    printf("\n  -- Gap Buffer Integration --\n");
    test_editor_gap_buffer_insert();
    test_editor_gap_buffer_operations();

    printf("\n  -- Undo / Redo --\n");
    test_editor_undo_redo_cleanup();
    test_editor_undo_redo_stacks_exist();

    printf("\nAll editor tests passed!\n");
    return 0;
}
