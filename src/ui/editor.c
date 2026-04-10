#include "../../include/ui/editor.h"
#include "../../include/ui/terminal.h"
#include "../../include/data_structures/gap_buffer.h"
#include "../../include/data_structures/stack.h"
#include "../../include/data_structures/select_buffer.h"

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
 *  Undo / Redo — action records pushed onto Stacks
 *
 *  Every insert or delete is recorded as an EditorAction so it can
 *  be reversed (undo) or replayed (redo).  Two stacks manage the
 *  history as specified in the project PPT (Slide 6).
 * ────────────────────────────────────────────────────────────────────── */

typedef enum {
    ACTION_INSERT,          /* characters were inserted at `offset`          */
    ACTION_DELETE           /* characters were deleted  at `offset`          */
} ActionType;

typedef struct {
    ActionType type;
    size_t     offset;      /* position in the flat text where it happened   */
    char      *text;        /* the characters that were inserted or deleted  */
    size_t     text_len;    /* length of `text`                              */
    int        cursor_row;  /* cursor row  before the action                 */
    int        cursor_col;  /* cursor col  before the action                 */
} EditorAction;

/**
 * @brief Create an action record on the heap.
 */
static EditorAction *action_create(ActionType type, size_t offset,
                                    const char *text, size_t text_len,
                                    int cursor_row, int cursor_col) {
    EditorAction *a = malloc(sizeof(EditorAction));
    if (!a) return NULL;
    a->type      = type;
    a->offset    = offset;
    a->text      = malloc(text_len + 1);
    if (a->text) {
        memcpy(a->text, text, text_len);
        a->text[text_len] = '\0';
    }
    a->text_len  = text_len;
    a->cursor_row = cursor_row;
    a->cursor_col = cursor_col;
    return a;
}

/**
 * @brief Free an action record.  Used as the StackItemDeallocator.
 */
static void action_free(void *ptr) {
    EditorAction *a = (EditorAction *)ptr;
    if (a) {
        free(a->text);
        free(a);
    }
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

    /* Undo / Redo stacks (PPT Slide 6) */
    Stack     *undo_stack;      /* stack of EditorAction* for undo           */
    Stack     *redo_stack;      /* stack of EditorAction* for redo           */

    /* Clipboard / Selection */
    SelectBuffer *clipboard;    /* select buffer for cut/copy/paste          */
    int        sel_anchor_row;  /* selection anchor row (-1 = no selection)  */
    int        sel_anchor_col;  /* selection anchor col                      */

    char      *status_msg;      /* transient message for the message bar     */
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

/**
 * @brief Get the current flat offset of the cursor.
 */
static size_t editor_get_cursor_offset(Editor *ed) {
    size_t text_len;
    char *text = gb_get_text(ed->buffer, &text_len);
    if (!text) return 0;
    int actual;
    size_t off = row_col_to_offset(text, text_len,
                                    ed->cursor_row, ed->cursor_col, &actual);
    free(text);
    return off;
}

/* ──────────────────────────────────────────────────────────────────────
 *  Undo / Redo helpers
 * ────────────────────────────────────────────────────────────────────── */

/**
 * @brief Clear the redo stack (called after any new edit).
 */
static void editor_clear_redo(Editor *ed) {
    while (!stack_is_empty(ed->redo_stack)) {
        EditorAction *a = (EditorAction *)stack_pop(ed->redo_stack);
        action_free(a);
    }
}

/**
 * @brief Push an insert action onto the undo stack.
 *        Records that `text_len` chars were inserted at `offset`.
 */
static void editor_record_insert(Editor *ed, size_t offset,
                                  const char *text, size_t text_len) {
    EditorAction *a = action_create(ACTION_INSERT, offset, text, text_len,
                                     ed->cursor_row, ed->cursor_col);
    if (a) {
        stack_push(ed->undo_stack, a);
        editor_clear_redo(ed);
    }
}

/**
 * @brief Push a delete action onto the undo stack.
 *        Records that `text_len` chars were deleted from `offset`.
 */
static void editor_record_delete(Editor *ed, size_t offset,
                                  const char *text, size_t text_len) {
    EditorAction *a = action_create(ACTION_DELETE, offset, text, text_len,
                                     ed->cursor_row, ed->cursor_col);
    if (a) {
        stack_push(ed->undo_stack, a);
        editor_clear_redo(ed);
    }
}

/**
 * @brief Set the transient status message.
 */
static void editor_set_status_msg(Editor *ed, const char *msg) {
    free(ed->status_msg);
    ed->status_msg = NULL;
    if (msg) {
        ed->status_msg = malloc(strlen(msg) + 1);
        if (ed->status_msg) strcpy(ed->status_msg, msg);
    }
}

/**
 * @brief Undo the last action.
 */
static void editor_undo(Editor *ed) {
    if (stack_is_empty(ed->undo_stack)) {
        editor_set_status_msg(ed, "Nothing to undo");
        return;
    }

    EditorAction *a = (EditorAction *)stack_pop(ed->undo_stack);

    if (a->type == ACTION_INSERT) {
        /* Undo an insert = delete the inserted text */
        gb_move_cursor(ed->buffer, a->offset);
        /* The text was inserted at offset, so characters are at
           offset .. offset + text_len.  We need to move cursor to
           offset + text_len and delete backwards. */
        gb_move_cursor(ed->buffer, a->offset + a->text_len);
        gb_delete_before(ed->buffer, a->text_len);
    } else {
        /* Undo a delete = re-insert the deleted text */
        gb_move_cursor(ed->buffer, a->offset);
        gb_insert(ed->buffer, a->text, a->text_len);
        /* After insert, cursor is at offset + text_len, move back */
        gb_move_cursor(ed->buffer, a->offset + a->text_len);
    }

    /* Restore cursor position to what it was before the action */
    ed->cursor_row = a->cursor_row;
    ed->cursor_col = a->cursor_col;
    ed->dirty = true;

    /* Recalc total lines */
    size_t tlen;
    char *t = gb_get_text(ed->buffer, &tlen);
    if (t) {
        ed->total_lines = count_lines(t, tlen);
        free(t);
    }
    editor_sync_cursor(ed);

    /* Push to redo stack */
    stack_push(ed->redo_stack, a);

    editor_set_status_msg(ed, "Undo");
}

/**
 * @brief Redo the last undone action.
 */
static void editor_redo(Editor *ed) {
    if (stack_is_empty(ed->redo_stack)) {
        editor_set_status_msg(ed, "Nothing to redo");
        return;
    }

    EditorAction *a = (EditorAction *)stack_pop(ed->redo_stack);

    if (a->type == ACTION_INSERT) {
        /* Redo an insert = insert the text again */
        gb_move_cursor(ed->buffer, a->offset);
        gb_insert(ed->buffer, a->text, a->text_len);
    } else {
        /* Redo a delete = delete the text again */
        gb_move_cursor(ed->buffer, a->offset + a->text_len);
        gb_delete_before(ed->buffer, a->text_len);
    }

    ed->dirty = true;

    /* Recalc total lines and place cursor after the action */
    size_t tlen;
    char *t = gb_get_text(ed->buffer, &tlen);
    if (t) {
        ed->total_lines = count_lines(t, tlen);
        if (a->type == ACTION_INSERT) {
            /* After re-inserting, cursor goes to end of inserted text */
            offset_to_row_col(t, a->offset + a->text_len,
                              &ed->cursor_row, &ed->cursor_col);
        } else {
            /* After re-deleting, cursor goes to deletion point */
            offset_to_row_col(t, a->offset,
                              &ed->cursor_row, &ed->cursor_col);
        }
        free(t);
    }
    editor_sync_cursor(ed);

    /* Push back to undo stack (without clearing redo) */
    stack_push(ed->undo_stack, a);

    editor_set_status_msg(ed, "Redo");
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
    int total_lines = count_lines(text, text_len);
    ed->total_lines = total_lines;

    /* Skip to row_offset */
    int cur_line = 0;
    size_t pos = 0;
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
    const char *help_default =
        " ESC = Save & Quit | Ctrl+Z = Undo | Ctrl+Y = Redo";
    const char *msg = (ed->status_msg && ed->status_msg[0])
                          ? ed->status_msg : help_default;
    int msg_len = (int)strlen(msg);
    if (msg_len > ed->screen_cols) msg_len = ed->screen_cols;
    ab_append(&ab, msg, (size_t)msg_len);

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

void editor_process_key(Editor *ed, int key) {

    /* Clear transient status message on any key */
    editor_set_status_msg(ed, NULL);

    switch (key) {

    /* ── EXIT: Save & Quit ──────────────────────────────────────────── */
    case KEY_ESCAPE:
        ed->running = false;
        break;

    /* ── UNDO / REDO ────────────────────────────────────────────────── */
    case KEY_CTRL_Z:
        editor_undo(ed);
        break;

    case KEY_CTRL_Y:
        editor_redo(ed);
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

        /* Capture the character about to be deleted (for undo) */
        size_t offset = editor_get_cursor_offset(ed);
        if (offset == 0) break;

        size_t tlen;
        char *t = gb_get_text(ed->buffer, &tlen);
        if (!t) break;
        char deleted_char = t[offset - 1];
        free(t);

        /* Record undo BEFORE the deletion */
        int save_row = ed->cursor_row;
        int save_col = ed->cursor_col;
        editor_record_delete(ed, offset - 1, &deleted_char, 1);

        /* Perform the deletion */
        gb_delete_before(ed->buffer, 1);
        ed->dirty = true;

        /* Recompute cursor position */
        t = gb_get_text(ed->buffer, &tlen);
        if (t) {
            if (save_col > 0) {
                ed->cursor_col = save_col - 1;
            } else {
                /* Joined with previous line */
                ed->cursor_row = save_row - 1;
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
        size_t offset = editor_get_cursor_offset(ed);

        if (offset >= sz) break;

        /* Capture character about to be deleted */
        size_t tlen;
        char *t = gb_get_text(ed->buffer, &tlen);
        if (!t) break;
        char deleted_char = t[offset];
        free(t);

        /* Record undo */
        editor_record_delete(ed, offset, &deleted_char, 1);

        /* Perform deletion */
        gb_delete_after(ed->buffer, 1);
        ed->dirty = true;

        /* Update total lines */
        t = gb_get_text(ed->buffer, &tlen);
        if (t) {
            ed->total_lines = count_lines(t, tlen);
            free(t);
        }
        break;
    }

    /* ── INSERTION ──────────────────────────────────────────────────── */
    case '\r':   /* Enter — insert newline */
    case '\n': {
        editor_sync_cursor(ed);
        size_t offset = editor_get_cursor_offset(ed);

        gb_insert(ed->buffer, "\n", 1);
        editor_record_insert(ed, offset, "\n", 1);

        ed->cursor_row++;
        ed->cursor_col = 0;
        ed->dirty = true;
        editor_sync_cursor(ed);
        break;
    }

    case '\t': { /* Tab — insert spaces */
        editor_sync_cursor(ed);
        int spaces = EDITOR_TAB_STOP - (ed->cursor_col % EDITOR_TAB_STOP);
        size_t offset = editor_get_cursor_offset(ed);

        /* Build the space string */
        char space_buf[8];
        memset(space_buf, ' ', (size_t)spaces);

        for (int i = 0; i < spaces; i++) {
            gb_insert(ed->buffer, " ", 1);
        }
        editor_record_insert(ed, offset, space_buf, (size_t)spaces);

        ed->cursor_col += spaces;
        ed->dirty = true;
        editor_sync_cursor(ed);
        break;
    }

    default: {
        /* Printable character insert */
        if (key >= 32 && key <= 126) {
            editor_sync_cursor(ed);
            size_t offset = editor_get_cursor_offset(ed);

            char ch = (char)key;
            gb_insert(ed->buffer, &ch, 1);
            editor_record_insert(ed, offset, &ch, 1);

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

    /* Create undo / redo stacks */
    ed->undo_stack = stack_create();
    ed->redo_stack = stack_create();
    if (!ed->undo_stack || !ed->redo_stack) {
        gb_destroy(ed->buffer);
        if (ed->undo_stack) stack_destroy(ed->undo_stack, NULL);
        if (ed->redo_stack) stack_destroy(ed->redo_stack, NULL);
        free(ed);
        return NULL;
    }

    /* Load initial content into the buffer */
    if (initial_content && *initial_content) {
        size_t len = strlen(initial_content);
        if (!gb_insert(ed->buffer, initial_content, len)) {
            gb_destroy(ed->buffer);
            stack_destroy(ed->undo_stack, NULL);
            stack_destroy(ed->redo_stack, NULL);
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
    ed->status_msg = NULL;

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
    if (ed->buffer)     gb_destroy(ed->buffer);
    if (ed->undo_stack) stack_destroy(ed->undo_stack, action_free);
    if (ed->redo_stack) stack_destroy(ed->redo_stack, action_free);
    if (ed->filename)   free(ed->filename);
    if (ed->status_msg) free(ed->status_msg);
    free(ed);
}
