#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/ui/editor.h"

/* ──────────────────────────────────────────────────────────────────────
 *  Unit tests for the Editor (non-interactive parts only).
 *
 *  These tests exercise editor_create, editor_get_content, and
 *  editor_destroy — i.e. the gap-buffer integration layer.
 *  The interactive editor_run() loop must be tested manually.
 * ────────────────────────────────────────────────────────────────────── */

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

/* ──────────────────────────────────────────────────────────────────────
 *  Main
 * ────────────────────────────────────────────────────────────────────── */

int main(void) {
    printf("Running editor tests...\n");

    test_editor_create_empty();
    test_editor_create_with_content();
    test_editor_create_multiline();
    test_editor_create_no_filename();
    test_editor_create_large_content();
    test_editor_destroy_null();
    test_editor_get_content_null();
    test_editor_special_characters();

    printf("All editor tests passed!\n");
    return 0;
}
