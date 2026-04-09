#include <stdio.h>
#include <stdlib.h>
#include "../include/ui/editor.h"

/**
 * Standalone demo to test the text editor interactively.
 * 
 * Build:  gcc -Wall -g -I./include demos/editor_demo.c src/ui/editor.c src/ui/terminal.c src/data_structures/gap_buffer.c src/data_structures/stack.c -o editor_demo.exe
 * Run:    .\editor_demo.exe
 *
 * Controls:
 *   - Type to insert text
 *   - Arrow keys to navigate
 *   - Backspace / Delete to remove characters
 *   - Ctrl+Z = Undo,  Ctrl+Y = Redo
 *   - Escape = Save & Quit
 */
int main(int argc, char *argv[]) {
    const char *sample_text =
        "Welcome to the LIMA Text Editor!\n"
        "\n"
        "This editor uses a Gap Buffer for O(1) inserts.\n"
        "Try typing, deleting, and using arrow keys.\n"
        "\n"
        "Ctrl+Z = Undo\n"
        "Ctrl+Y = Redo\n"
        "Escape = Save & Quit\n";

    char *initial_text = NULL;
    const char *filename = "demo.txt";

    if (argc > 1) {
        filename = argv[1];
        FILE *f = fopen(filename, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            if (fsize > 0) {
                initial_text = malloc(fsize + 1);
                if (initial_text) {
                    size_t read_bytes = fread(initial_text, 1, fsize, f);
                    initial_text[read_bytes] = '\0';
                }
            }
            fclose(f);
        }
    }

    const char *start_content = sample_text;
    if (argc > 1) {
        start_content = initial_text ? initial_text : "";
    }

    Editor *ed = editor_create(filename, start_content);
    if (!ed) {
        fprintf(stderr, "Failed to create editor\n");
        free(initial_text);
        return 1;
    }

    editor_run(ed);

    /* After Escape, print the final content or save it */
    size_t len;
    char *content = editor_get_content(ed, &len);
    if (content) {
        if (argc > 1) {
            FILE *f = fopen(filename, "wb");
            if (f) {
                fwrite(content, 1, len, f);
                fclose(f);
                printf("\nSaved %lu bytes to %s\n", (unsigned long)len, filename);
            } else {
                fprintf(stderr, "\nFailed to save to %s\n", filename);
            }
        } else {
            printf("\n--- Final content (%lu bytes) ---\n%s\n", (unsigned long)len, content);
        }
        free(content);
    }

    editor_destroy(ed);
    if (initial_text) free(initial_text);
    return 0;
}
