#include "../../include/ui/editor.h"
#include "../../include/ui/terminal.h"
#include "../../include/data_structures/gap_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────────────────────────────────────────────
 *  Constants
 * ────────────────────────────────────────────────────────────────────── */

#define EDITOR_TAB_STOP     4
#define EDITOR_INITIAL_CAP  4096       /* initial gap-buffer capacity          */
#define STATUS_BAR_ROWS     2          /* status bar + message bar             */

/* ──────────────────────────────────────────────────────────────────────
 *  Append Buffer — accumulates output so we can flush in one write()
 * ────────────────────────────────────────────────────────────────────── */

typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
} AppendBuffer;

static void ab_init(AppendBuffer *ab) {
    ab->buf = NULL;
    ab->len = 0;
    ab->cap = 0;
}

static void ab_append(AppendBuffer *ab, const char *s, size_t slen) {
    if (ab->len + slen > ab->cap) {
        size_t new_cap = (ab->cap == 0) ? 1024 : ab->cap * 2;
        while (new_cap < ab->len + slen) new_cap *= 2;
        char *new_buf = realloc(ab->buf, new_cap);
        if (!new_buf) return;
        ab->buf = new_buf;
        ab->cap = new_cap;
    }
    memcpy(ab->buf + ab->len, s, slen);
    ab->len += slen;
}

static void ab_free(AppendBuffer *ab) {
    free(ab->buf);
    ab->buf = NULL;
    ab->len = 0;
    ab->cap = 0;
}

/* ──────────────────────────────────────────────────────────────────────
 *  Editor State
 * ────────────────────────────────────────────────────────────────────── */

struct Editor {
    GapBuffer *buffer;          /* the gap-buffer holding the file text      */
    char      *filename;        /* display name for the status bar           */

    int        cursor_row;      /* cursor row in file coordinates (0-based)  */
    int        cursor_col;      /* cursor col in file coordinates (0-based)  */

    int        row_offset;      /* vertical scroll (first visible row)       */
    int        col_offset;      /* horizontal scroll (first visible col)     */

    int        screen_rows;     /* usable rows (terminal rows - status bar)  */
    int        screen_cols;     /* terminal columns                          */

    int        total_lines;     /* cached line count                         */

    bool       dirty;           /* content modified since load?              */
    bool       running;         /* main-loop flag                            */
};

/* ──────────────────────────────────────────────────────────────────────
 *  Internal helpers — line indexing
 *
 *  The gap buffer stores flat text.  We walk it to find line boundaries
 *  on each render pass (cheap for the file sizes in a VFS demo).
 * ────────────────────────────────────────────────────────────────────── */

/**
 * @brief Count the number of lines in a string.
 *        An empty string has 1 line.
 */
static int count_lines(const char *text, size_t len) {
    int lines = 1;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n') lines++;
    }
    return lines;
}

/**
 * @brief Given (row, col), compute the flat character offset into `text`.
 *        Clamps col if the row is shorter than requested.
 *        Returns the clamped column through *actual_col.
 */
static size_t row_col_to_offset(const char *text, size_t len,
                                int row, int col, int *actual_col) {
    int cur_row = 0;
    size_t i = 0;

    /* Advance to the requested row */
    while (i < len && cur_row < row) {
        if (text[i] == '\n') cur_row++;
        i++;
    }

    /* i now points to the start of `row` */
    size_t line_start = i;

    /* Find end of this line */
    size_t line_end = i;
    while (line_end < len && text[line_end] != '\n') {
        line_end++;
    }

    int line_len = (int)(line_end - line_start);
    if (col > line_len) col = line_len;
    if (actual_col) *actual_col = col;

    return line_start + (size_t)col;
}

/**
 * @brief Given a flat character offset, compute (row, col).
 */
static void offset_to_row_col(const char *text, size_t offset,
                              int *out_row, int *out_col)
                              __attribute__((unused));
static void offset_to_row_col(const char *text, size_t offset,
                              int *out_row, int *out_col) {
    int row = 0, col = 0;
    for (size_t i = 0; i < offset; i++) {
        if (text[i] == '\n') {
            row++;
            col = 0;
        } else {
            col++;
        }
    }
    *out_row = row;
    *out_col = col;
}

/**
 * @brief Get the length of the given line (0-indexed).
 */
static int get_line_length(const char *text, size_t len, int row) {
    int cur_row = 0;
    size_t i = 0;
    while (i < len && cur_row < row) {
        if (text[i] == '\n') cur_row++;
        i++;
    }
    size_t line_start = i;
    while (i < len && text[i] != '\n') i++;
    return (int)(i - line_start);
}

/* ──────────────────────────────────────────────────────────────────────
 *  Synchronise gap-buffer cursor position with editor (row, col)
 * ────────────────────────────────────────────────────────────────────── */

static void editor_sync_cursor(Editor *ed) {
    size_t text_len;
    char *text = gb_get_text(ed->buffer, &text_len);
    if (!text) return;

    int actual_col;
    size_t offset = row_col_to_offset(text, text_len,
                                       ed->cursor_row, ed->cursor_col,
                                       &actual_col);
    ed->cursor_col = actual_col;
    gb_move_cursor(ed->buffer, offset);

    ed->total_lines = count_lines(text, text_len);
    free(text);
}

/* ──────────────────────────────────────────────────────────────────────
 *  Scrolling
 * ────────────────────────────────────────────────────────────────────── */

static void editor_scroll(Editor *ed) {
    /* Vertical scroll */
    if (ed->cursor_row < ed->row_offset) {
        ed->row_offset = ed->cursor_row;
    }
    if (ed->cursor_row >= ed->row_offset + ed->screen_rows) {
        ed->row_offset = ed->cursor_row - ed->screen_rows + 1;
    }

    /* Horizontal scroll */
    if (ed->cursor_col < ed->col_offset) {
        ed->col_offset = ed->cursor_col;
    }
    if (ed->cursor_col >= ed->col_offset + ed->screen_cols) {
        ed->col_offset = ed->cursor_col - ed->screen_cols + 1;
    }
}

/* ──────────────────────────────────────────────────────────────────────
 *  Rendering
 * ────────────────────────────────────────────────────────────────────── */

static void editor_render(Editor *ed) {
    size_t text_len;
    char *text = gb_get_text(ed->buffer, &text_len);
    if (!text) return;

    /* Refresh terminal dimensions each frame (handles resize) */
    int term_rows, term_cols;
    term_get_window_size(&term_rows, &term_cols);
    ed->screen_rows = term_rows - STATUS_BAR_ROWS;
    ed->screen_cols = term_cols;
    if (ed->screen_rows < 1) ed->screen_rows = 1;

    editor_scroll(ed);

    AppendBuffer ab;
    ab_init(&ab);

    /* Hide cursor + move to top-left */
    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);

    /* ── Draw text rows ─────────────────────────────────────────────── */

    /* Walk the text to find line starts */
    /* We pre-compute an array of line-start offsets for fast access */
    int total_lines = count_lines(text, text_len);
    ed->total_lines = total_lines;

    /* Collect line-start offsets (we only need from row_offset onward) */
    int cur_line = 0;
    size_t pos = 0;

    /* Skip to row_offset */
    while (pos < text_len && cur_line < ed->row_offset) {
        if (text[pos] == '\n') cur_line++;
        pos++;
    }

    for (int y = 0; y < ed->screen_rows; y++) {
        int file_row = ed->row_offset + y;

        if (file_row >= total_lines) {
            /* Past end of file — draw tilde */
            ab_append(&ab, "\x1b[34m~\x1b[0m", 12);  /* blue tilde */
        } else {
            /* Find this line in the text */
            size_t line_start = pos;
            size_t line_end = pos;
            while (line_end < text_len && text[line_end] != '\n') {
                line_end++;
            }
            int line_len = (int)(line_end - line_start);

            /* Apply horizontal scroll */
            int render_start = ed->col_offset;
            if (render_start < line_len) {
                int render_len = line_len - render_start;
                if (render_len > ed->screen_cols) render_len = ed->screen_cols;
                ab_append(&ab, text + line_start + render_start, (size_t)render_len);
            }

            /* Advance pos past this line (and the newline) */
            pos = line_end;
            if (pos < text_len && text[pos] == '\n') pos++;
        }

        /* Clear rest of line + newline */
        ab_append(&ab, "\x1b[K\r\n", 5);
    }

    /* ── Draw status bar ────────────────────────────────────────────── */
    ab_append(&ab, "\x1b[7m", 4);  /* Inverted colors */

    char status_left[256];
    char status_right[128];
    int left_len = snprintf(status_left, sizeof(status_left),
                            " %.60s%s",
                            ed->filename ? ed->filename : "[No Name]",
                            ed->dirty ? " [+]" : "");
    int right_len = snprintf(status_right, sizeof(status_right),
                             "Ln %d/%d  Col %d ",
                             ed->cursor_row + 1, total_lines,
                             ed->cursor_col + 1);

    if (left_len > ed->screen_cols) left_len = ed->screen_cols;
    ab_append(&ab, status_left, (size_t)left_len);

    /* Pad between left and right */
    int padding = ed->screen_cols - left_len - right_len;
    for (int i = 0; i < padding; i++) {
        ab_append(&ab, " ", 1);
    }
    if (padding + left_len + right_len <= ed->screen_cols) {
        ab_append(&ab, status_right, (size_t)right_len);
    }

    ab_append(&ab, "\x1b[0m\r\n", 6);  /* Reset attributes + newline */

    /* ── Draw message bar ───────────────────────────────────────────── */
    ab_append(&ab, "\x1b[K", 3);  /* Clear line */
    const char *help = " ESC = Save & Quit | Arrow Keys = Navigate | Type to insert";
    int help_len = (int)strlen(help);
    if (help_len > ed->screen_cols) help_len = ed->screen_cols;
    ab_append(&ab, help, (size_t)help_len);

    /* ── Position cursor ────────────────────────────────────────────── */
    char cursor_buf[32];
    int cursor_len = snprintf(cursor_buf, sizeof(cursor_buf), "\x1b[%d;%dH",
                              (ed->cursor_row - ed->row_offset) + 1,
                              (ed->cursor_col - ed->col_offset) + 1);
    ab_append(&ab, cursor_buf, (size_t)cursor_len);

    /* Show cursor */
    ab_append(&ab, "\x1b[?25h", 6);

    /* ── Flush to stdout ────────────────────────────────────────────── */
    fwrite(ab.buf, 1, ab.len, stdout);
    fflush(stdout);

    ab_free(&ab);
    free(text);
}

/* ──────────────────────────────────────────────────────────────────────
 *  Keystroke Processing
 * ────────────────────────────────────────────────────────────────────── */

static void editor_process_key(Editor *ed, int key) {
    switch (key) {

    /* ── EXIT: Save & Quit ──────────────────────────────────────────── */
    case KEY_ESCAPE:
        ed->running = false;
        break;

    /* ── NAVIGATION ─────────────────────────────────────────────────── */
    case KEY_ARROW_LEFT:
        if (ed->cursor_col > 0) {
            ed->cursor_col--;
        } else if (ed->cursor_row > 0) {
            /* Wrap to end of previous line */
            ed->cursor_row--;
            size_t tlen;
            char *t = gb_get_text(ed->buffer, &tlen);
            if (t) {
                ed->cursor_col = get_line_length(t, tlen, ed->cursor_row);
                free(t);
            }
        }
        editor_sync_cursor(ed);
        break;

    case KEY_ARROW_RIGHT: {
        size_t tlen;
        char *t = gb_get_text(ed->buffer, &tlen);
        if (t) {
            int line_len = get_line_length(t, tlen, ed->cursor_row);
            if (ed->cursor_col < line_len) {
                ed->cursor_col++;
            } else if (ed->cursor_row < ed->total_lines - 1) {
                /* Wrap to start of next line */
                ed->cursor_row++;
                ed->cursor_col = 0;
            }
            free(t);
        }
        editor_sync_cursor(ed);
        break;
    }

    case KEY_ARROW_UP:
        if (ed->cursor_row > 0) {
            ed->cursor_row--;
        }
        editor_sync_cursor(ed);   /* sync clamps col if shorter line */
        break;

    case KEY_ARROW_DOWN:
        if (ed->cursor_row < ed->total_lines - 1) {
            ed->cursor_row++;
        }
        editor_sync_cursor(ed);
        break;

    case KEY_HOME:
        ed->cursor_col = 0;
        editor_sync_cursor(ed);
        break;

    case KEY_END: {
        size_t tlen;
        char *t = gb_get_text(ed->buffer, &tlen);
        if (t) {
            ed->cursor_col = get_line_length(t, tlen, ed->cursor_row);
            free(t);
        }
        editor_sync_cursor(ed);
        break;
    }

    case KEY_PAGE_UP: {
        int jump = ed->screen_rows;
        ed->cursor_row -= jump;
        if (ed->cursor_row < 0) ed->cursor_row = 0;
        editor_sync_cursor(ed);
        break;
    }

    case KEY_PAGE_DOWN: {
        int jump = ed->screen_rows;
        ed->cursor_row += jump;
        if (ed->cursor_row >= ed->total_lines) {
            ed->cursor_row = ed->total_lines - 1;
        }
        if (ed->cursor_row < 0) ed->cursor_row = 0;
        editor_sync_cursor(ed);
        break;
    }

    /* ── DELETION ───────────────────────────────────────────────────── */
    case KEY_BACKSPACE: {
        if (ed->cursor_row == 0 && ed->cursor_col == 0) break;
        editor_sync_cursor(ed);
        gb_delete_before(ed->buffer, 1);
        ed->dirty = true;

        /* Recompute cursor position from gap buffer state */
        size_t tlen;
        char *t = gb_get_text(ed->buffer, &tlen);
        if (t) {
            /* The gap_start IS the cursor position after delete_before */
            /* We need to figure out the new offset. After gb_delete_before,
               the cursor is one position back, so we compute from the
               row/col we had before. */
            if (ed->cursor_col > 0) {
                ed->cursor_col--;
            } else {
                /* We were at col 0, so we joined with previous line */
                ed->cursor_row--;
                ed->cursor_col = get_line_length(t, tlen, ed->cursor_row);
            }
            ed->total_lines = count_lines(t, tlen);
            free(t);
        }
        editor_sync_cursor(ed);
        break;
    }

    case KEY_DELETE: {
        editor_sync_cursor(ed);
        size_t sz = gb_size(ed->buffer);
        /* Compute current offset */
        size_t tlen;
        char *t = gb_get_text(ed->buffer, &tlen);
        if (t) {
            int actual;
            size_t off = row_col_to_offset(t, tlen,
                                            ed->cursor_row, ed->cursor_col,
                                            &actual);
            free(t);
            if (off < sz) {
                gb_delete_after(ed->buffer, 1);
                ed->dirty = true;
                /* Re-fetch to update total_lines */
                t = gb_get_text(ed->buffer, &tlen);
                if (t) {
                    ed->total_lines = count_lines(t, tlen);
                    free(t);
                }
            }
        }
        break;
    }

    /* ── INSERTION ──────────────────────────────────────────────────── */
    case '\r':   /* Enter — insert newline */
    case '\n': {
        editor_sync_cursor(ed);
        gb_insert(ed->buffer, "\n", 1);
        ed->cursor_row++;
        ed->cursor_col = 0;
        ed->dirty = true;
        editor_sync_cursor(ed);
        break;
    }

    case '\t': { /* Tab — insert spaces */
        editor_sync_cursor(ed);
        int spaces = EDITOR_TAB_STOP - (ed->cursor_col % EDITOR_TAB_STOP);
        for (int i = 0; i < spaces; i++) {
            gb_insert(ed->buffer, " ", 1);
        }
        ed->cursor_col += spaces;
        ed->dirty = true;
        editor_sync_cursor(ed);
        break;
    }

    default: {
        /* Printable character insert */
        if (key >= 32 && key <= 126) {
            editor_sync_cursor(ed);
            char ch = (char)key;
            gb_insert(ed->buffer, &ch, 1);
            ed->cursor_col++;
            ed->dirty = true;
            editor_sync_cursor(ed);
        }
        break;
    }

    } /* end switch */
}

/* ──────────────────────────────────────────────────────────────────────
 *  Public API
 * ────────────────────────────────────────────────────────────────────── */

Editor *editor_create(const char *filename, const char *initial_content) {
    Editor *ed = malloc(sizeof(Editor));
    if (!ed) return NULL;

    memset(ed, 0, sizeof(Editor));

    /* Create the gap buffer */
    ed->buffer = gb_create(EDITOR_INITIAL_CAP);
    if (!ed->buffer) {
        free(ed);
        return NULL;
    }

    /* Load initial content into the buffer */
    if (initial_content && *initial_content) {
        size_t len = strlen(initial_content);
        if (!gb_insert(ed->buffer, initial_content, len)) {
            gb_destroy(ed->buffer);
            free(ed);
            return NULL;
        }
        /* Move the cursor back to the beginning */
        gb_move_cursor(ed->buffer, 0);
    }

    /* Store filename */
    if (filename) {
        ed->filename = malloc(strlen(filename) + 1);
        if (ed->filename) {
            strcpy(ed->filename, filename);
        }
    } else {
        ed->filename = NULL;
    }

    /* Initial state */
    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->row_offset = 0;
    ed->col_offset = 0;
    ed->dirty      = false;
    ed->running    = true;

    /* Count lines */
    size_t tlen;
    char *t = gb_get_text(ed->buffer, &tlen);
    if (t) {
        ed->total_lines = count_lines(t, tlen);
        free(t);
    } else {
        ed->total_lines = 1;
    }

    return ed;
}

void editor_run(Editor *ed) {
    if (!ed) return;

    term_enable_raw_mode();

    /* Get initial screen size */
    int term_rows, term_cols;
    term_get_window_size(&term_rows, &term_cols);
    ed->screen_rows = term_rows - STATUS_BAR_ROWS;
    ed->screen_cols = term_cols;
    if (ed->screen_rows < 1) ed->screen_rows = 1;

    ed->running = true;

    while (ed->running) {
        editor_render(ed);
        int key = term_read_key();
        editor_process_key(ed, key);
    }

    term_disable_raw_mode();

    /* Clear screen on exit so the CLI comes back clean */
    term_clear_screen();
}

char *editor_get_content(const Editor *ed, size_t *out_len) {
    if (!ed) return NULL;
    return gb_get_text(ed->buffer, out_len);
}

void editor_destroy(Editor *ed) {
    if (!ed) return;
    if (ed->buffer)   gb_destroy(ed->buffer);
    if (ed->filename)  free(ed->filename);
    free(ed);
}
