#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Gap Buffer ADT, commonly used in text editors.
 */

typedef struct GapBuffer GapBuffer;

/**
 * @brief Create a new gap buffer.
 * @param initial_capacity The initial size of the buffer.
 * @return A pointer to the newly created gap buffer, or NULL on failure.
 */
GapBuffer *gb_create(size_t initial_capacity);

/**
 * @brief Destroy the gap buffer and free allocated memory.
 * @param gb Pointer to the gap buffer.
 */
void gb_destroy(GapBuffer *gb);

/**
 * @brief Insert character(s) at the current cursor position.
 * @param gb Pointer to the gap buffer.
 * @param data Array of characters to insert.
 * @param len Number of characters to insert.
 * @return true on success, false on failure.
 */
bool gb_insert(GapBuffer *gb, const char *data, size_t len);

/**
 * @brief Delete character(s) before the cursor position.
 * @param gb Pointer to the gap buffer.
 * @param len Number of characters to delete.
 */
void gb_delete_before(GapBuffer *gb, size_t len);

/**
 * @brief Delete character(s) after the cursor position.
 * @param gb Pointer to the gap buffer.
 * @param len Number of characters to delete.
 */
void gb_delete_after(GapBuffer *gb, size_t len);

/**
 * @brief Move the cursor to a specific position.
 * @param gb Pointer to the gap buffer.
 * @param pos The logical position to move the cursor to.
 */
void gb_move_cursor(GapBuffer *gb, size_t pos);

/**
 * @brief Move the cursor relative to its current position.
 * @param gb Pointer to the gap buffer.
 * @param offset The offset to move by (can be negative).
 */
void gb_move_cursor_relative(GapBuffer *gb, int offset);

/**
 * @brief Get the content of the gap buffer as a contiguous string.
 * @param gb Pointer to the gap buffer.
 * @param out_len Optional pointer to store the length of the string.
 * @return A newly allocated null-terminated string, or NULL on failure. Caller must free.
 */
char *gb_get_text(const GapBuffer *gb, size_t *out_len);

/**
 * @brief Get the current logical size (number of valid characters) in the gap buffer.
 * @param gb Pointer to the gap buffer.
 * @return The number of characters in the buffer.
 */
size_t gb_size(const GapBuffer *gb);

#endif // GAP_BUFFER_H
