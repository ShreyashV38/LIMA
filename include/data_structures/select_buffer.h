#ifndef SELECT_BUFFER_H
#define SELECT_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Select Buffer ADT — a simple dynamic char array used as
 *        a clipboard for cut/copy/paste operations in the editor.
 *
 * Internally backed by a resizable array of characters.
 * The buffer owns its memory and stores a contiguous copy of
 * the selected text, independent of the gap buffer contents.
 */

typedef struct SelectBuffer SelectBuffer;

/**
 * @brief Create a new, empty select buffer.
 * @return A pointer to the newly created select buffer, or NULL on failure.
 */
SelectBuffer *sb_create(void);

/**
 * @brief Destroy the select buffer and free all allocated memory.
 * @param sb Pointer to the select buffer.
 */
void sb_destroy(SelectBuffer *sb);

/**
 * @brief Store text into the select buffer, replacing any previous content.
 * @param sb   Pointer to the select buffer.
 * @param text Pointer to the character data to store.
 * @param len  Number of characters to store.
 * @return true on success, false on failure (allocation error).
 */
bool sb_store(SelectBuffer *sb, const char *text, size_t len);

/**
 * @brief Clear the select buffer, discarding its contents.
 * @param sb Pointer to the select buffer.
 */
void sb_clear(SelectBuffer *sb);

/**
 * @brief Get a read-only pointer to the stored text.
 * @param sb      Pointer to the select buffer.
 * @param out_len Optional pointer to receive the length of the stored text.
 * @return Pointer to the internal character data, or NULL if empty.
 *         The pointer is valid until the next sb_store / sb_clear / sb_destroy.
 */
const char *sb_get_text(const SelectBuffer *sb, size_t *out_len);

/**
 * @brief Check whether the select buffer is empty.
 * @param sb Pointer to the select buffer.
 * @return true if the buffer is empty or NULL, false otherwise.
 */
bool sb_is_empty(const SelectBuffer *sb);

/**
 * @brief Get the number of characters currently stored.
 * @param sb Pointer to the select buffer.
 * @return The length of the stored text (0 if empty or NULL).
 */
size_t sb_length(const SelectBuffer *sb);

#endif // SELECT_BUFFER_H
