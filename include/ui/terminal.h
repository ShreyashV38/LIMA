#ifndef TERMINAL_H
#define TERMINAL_H

/**
 * @brief Terminal abstraction layer for the plain-text editor.
 *
 * Provides raw-mode terminal I/O and escape-sequence key decoding.
 * Supports both POSIX (termios) and Windows (Console API) platforms.
 */

/**
 * @brief Special key codes returned by term_read_key().
 * Printable ASCII characters are returned as-is (0–127).
 * Special keys use values >= 1000 to avoid collisions.
 */
enum EditorKey {
    KEY_CTRL_Y      = 25,        /* Ctrl+Y = Redo */
    KEY_CTRL_Z      = 26,        /* Ctrl+Z = Undo */
    KEY_ESCAPE      = 27,
    KEY_BACKSPACE   = 127,
    KEY_ARROW_LEFT  = 1000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_DELETE,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN
};

/**
 * @brief Enable raw mode on the terminal.
 *
 * Disables line buffering, echo, and signal processing so that
 * every keystroke is delivered immediately to the application.
 * Registers an atexit() handler to restore the original settings.
 */
void term_enable_raw_mode(void);

/**
 * @brief Disable raw mode and restore original terminal settings.
 */
void term_disable_raw_mode(void);

/**
 * @brief Read a single key from the terminal.
 *
 * Blocks until a key is pressed. Decodes multi-byte escape sequences
 * (arrow keys, Home, End, Delete, Page Up/Down) into EditorKey values.
 *
 * @return The key code (ASCII for printable chars, EditorKey enum for specials).
 */
int term_read_key(void);

/**
 * @brief Get the current terminal window dimensions.
 * @param rows Pointer to store the number of rows.
 * @param cols Pointer to store the number of columns.
 */
void term_get_window_size(int *rows, int *cols);

/**
 * @brief Clear the entire screen and move cursor to home position.
 */
void term_clear_screen(void);

#endif /* TERMINAL_H */
