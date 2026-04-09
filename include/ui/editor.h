#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Plain-text editor engine backed by a Gap Buffer.
 *
 * Provides a raw-mode, terminal-based text editor that uses a Gap Buffer
 * for efficient insert/delete operations. The editor is entered from the
 * CLI when a user opens a file and exited by pressing Escape (save & quit).
 */

typedef struct Editor Editor;

/**
 * @brief Create a new editor instance.
 *
 * Initializes the gap buffer with the given content and prepares the
 * editor state (cursor position, scroll offsets, etc.).
 *
 * @param filename      Display name shown in the status bar (can be NULL).
 * @param initial_content  The text to load into the editor (can be NULL for empty).
 * @return A new Editor instance, or NULL on failure.
 */
Editor *editor_create(const char *filename, const char *initial_content);

/**
 * @brief Enter the interactive editor loop.
 *
 * Switches the terminal to raw mode and enters the keystroke-processing
 * loop. The loop continues until the user presses Escape to save & quit.
 *
 * @param ed Pointer to the editor instance.
 */
void editor_run(Editor *ed);

/**
 * @brief Get the current content of the editor as a string.
 *
 * Extracts the text from the gap buffer. The caller is responsible
 * for freeing the returned string.
 *
 * @param ed      Pointer to the editor instance.
 * @param out_len Optional pointer to receive the length of the string.
 * @return A newly allocated null-terminated string, or NULL on failure.
 */
char *editor_get_content(const Editor *ed, size_t *out_len);

/**
 * @brief Destroy the editor and free all associated memory.
 * @param ed Pointer to the editor instance.
 */
void editor_destroy(Editor *ed);

#endif /* EDITOR_H */
