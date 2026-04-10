/*
 * main.c - CLI entry point for LIMA VFS
 * Team: Anosh, Anish, Shreyash, Samuel, Zilu
 *
 * Provides a POSIX-like interactive shell that simulates a single-root
 * file hierarchy. All user commands are routed through a dispatch table
 * to the underlying VFS, clipboard, persistence, and editor modules.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/vfs/vfs.h"
#include "../include/vfs/vfs_session.h"
#include "../include/core/clipboard.h"
#include "../include/core/persistence.h"
#include "../include/ui/editor.h"
#include "../include/data_structures/gap_buffer.h"
#include "../include/data_structures/nary_tree.h"

/* ========================================================================
 * Maximum input line length for the REPL.
 * ======================================================================== */
#define INPUT_MAX 1024

/* ========================================================================
 * Global state — the Session owns the VFS; the clipboard is separate.
 * ======================================================================== */
static VfsSession   *g_session   = NULL;
static Vfs          *g_vfs       = NULL;  /* alias into session's VFS */
static VfsClipboard *g_clipboard = NULL;

/* ========================================================================
 * Command handlers
 * Each returns 0 on success, 1 on failure (matches POSIX conventions).
 * ======================================================================== */

/* ---- pwd --------------------------------------------------------------- */
static int cmd_pwd(void) {
    char *path = vfs_pwd(g_vfs);
    if (!path) {
        fprintf(stderr, "pwd: cannot determine current directory\n");
        return 1;
    }
    printf("%s\n", path);
    free(path);
    return 0;
}

/* ---- ls ---------------------------------------------------------------- */
static int cmd_ls(void) {
    if (!g_vfs) return 1;

    NaryTreeNode *cwd = vfs_cwd(g_vfs);
    if (!cwd) return 1;

    NaryTreeNode *child = ntree_node_get_first_child(cwd);
    if (!child) {
        printf("  (empty directory)\n");
        return 0;
    }

    while (child) {
        VfsNodeType type = vfs_node_type(child);
        const char *name = vfs_node_name(child);

        if (type == VFS_NODE_DIR) {
            printf("  [DIR]  %s\n", name ? name : "?");
        } else {
            printf("  [FILE] %s\n", name ? name : "?");
        }

        child = ntree_node_get_next_sibling(child);
    }

    return 0;
}

/* ---- cd ---------------------------------------------------------------- */
static int cmd_cd(const char *path) {
    if (!path || !*path) {
        /* bare "cd" with no argument goes to root, like `cd /` */
        path = "/";
    }
    /* Use session if it tracks our VFS, otherwise fall back to raw VFS */
    bool ok;
    if (g_session && vfs_session_vfs(g_session) == g_vfs) {
        ok = vfs_session_cd(g_session, path);
    } else {
        ok = vfs_cd(g_vfs, path);
    }
    if (!ok) {
        fprintf(stderr, "cd: no such directory: %s\n", path);
        return 1;
    }
    return 0;
}

/* ---- mkdir ------------------------------------------------------------- */
static int cmd_mkdir(const char *name) {
    if (!name || !*name) {
        fprintf(stderr, "mkdir: missing operand\n");
        return 1;
    }
    bool ok;
    if (g_session && vfs_session_vfs(g_session) == g_vfs) {
        ok = vfs_session_mkdir(g_session, name);
    } else {
        ok = vfs_mkdir(g_vfs, name);
    }
    if (!ok) {
        fprintf(stderr, "mkdir: cannot create directory '%s': already exists or invalid name\n", name);
        return 1;
    }
    return 0;
}

/* ---- touch ------------------------------------------------------------- */
static int cmd_touch(const char *name) {
    if (!name || !*name) {
        fprintf(stderr, "touch: missing operand\n");
        return 1;
    }
    bool ok;
    if (g_session && vfs_session_vfs(g_session) == g_vfs) {
        ok = vfs_session_touch(g_session, name);
    } else {
        ok = vfs_touch(g_vfs, name);
    }
    if (!ok) {
        fprintf(stderr, "touch: cannot create file '%s': already exists or invalid name\n", name);
        return 1;
    }
    return 0;
}

/* ---- rm (delete) ------------------------------------------------------- */
static int cmd_delete(const char *path) {
    if (!g_vfs || !path) {
        fprintf(stderr, "rm: missing operand\n");
        return 1;
    }

    /*
     * vfs_rm() handles the O(1) detachment from the parent directory's
     * N-ary tree node, and then internally calls our O(N)
     * vfs__destroy_subtree() to wipe it from memory.
     */
    if (vfs_rm(g_vfs, path)) {
        printf("Deleted '%s' successfully.\n", path);
        return 0;
    } else {
        fprintf(stderr, "rm: cannot remove '%s': No such file or directory\n", path);
        return 1;
    }
}

/* ---- edit (transitions into the plain-text editor) --------------------- */
static int cmd_edit(const char *filepath) {
    if (!g_vfs || !filepath) {
        fprintf(stderr, "edit: missing filename\n");
        return 1;
    }

    char *initial_content = NULL;
    size_t content_len = 0;

    /* 1. Try to open the file using the actual VFS API */
    GapBuffer *file_gb = vfs_open_file(g_vfs, filepath);

    if (file_gb) {
        /* File exists, extract its current text */
        initial_content = gb_get_text(file_gb, &content_len);
    } else {
        /* File doesn't exist. Let's try to create it. */
        if (!vfs_touch(g_vfs, filepath)) {
            fprintf(stderr, "edit: cannot open or create '%s'. It might be a directory.\n", filepath);
            return 1;
        }
        /* Open the newly created file */
        file_gb = vfs_open_file(g_vfs, filepath);
        if (!file_gb) return 1;

        initial_content = strdup(""); /* Start empty */
    }

    /* 2. Create and run the editor */
    Editor *ed = editor_create(filepath, initial_content);
    if (!ed) {
        fprintf(stderr, "edit: failed to create editor\n");
        free(initial_content);
        return 1;
    }

    editor_run(ed); /* Blocks until user presses Escape */

    /* 3. Get the saved content and write it back to the VFS */
    size_t new_len;
    char *new_content = editor_get_content(ed, &new_len);
    if (new_content) {
        /* The easiest way to overwrite in this VFS is to remove and recreate */
        vfs_rm(g_vfs, filepath);
        vfs_touch(g_vfs, filepath);

        GapBuffer *new_gb = vfs_open_file(g_vfs, filepath);
        if (new_gb) {
            gb_insert(new_gb, new_content, new_len);
        }
        free(new_content);
    }

    /* 4. Cleanup */
    editor_destroy(ed);
    free(initial_content);

    return 0;
}

/* ---- copy -------------------------------------------------------------- */
static int cmd_copy(const char *name) {
    if (!name || !*name) {
        fprintf(stderr, "copy: missing operand\n");
        return 1;
    }
    if (!vfs_clipboard_copy(g_clipboard, g_vfs, name)) {
        fprintf(stderr, "copy: cannot copy '%s': not found\n", name);
        return 1;
    }
    printf("Copied '%s' to clipboard.\n", name);
    return 0;
}

/* ---- cut --------------------------------------------------------------- */
static int cmd_cut(const char *name) {
    if (!name || !*name) {
        fprintf(stderr, "cut: missing operand\n");
        return 1;
    }
    if (!vfs_clipboard_cut(g_clipboard, g_vfs, name)) {
        fprintf(stderr, "cut: cannot cut '%s': not found\n", name);
        return 1;
    }
    printf("Cut '%s' to clipboard.\n", name);
    return 0;
}

/* ---- paste ------------------------------------------------------------- */
static int cmd_paste(void) {
    if (!vfs_clipboard_paste(g_clipboard, g_vfs)) {
        fprintf(stderr, "paste: clipboard is empty or name collision in current directory\n");
        return 1;
    }
    printf("Pasted from clipboard.\n");
    return 0;
}

/* ---- back (navigation history) ----------------------------------------- */
static int cmd_back(void) {
    if (!vfs_session_back(g_session)) {
        fprintf(stderr, "back: no previous directory in history\n");
        return 1;
    }
    /* Show the user where they ended up */
    char *path = vfs_pwd(g_vfs);
    if (path) {
        printf("%s\n", path);
        free(path);
    }
    return 0;
}

/* ---- forward (navigation history) -------------------------------------- */
static int cmd_front(void) {
    if (!vfs_session_forward(g_session)) {
        fprintf(stderr, "forward: no next directory in history\n");
        return 1;
    }
    char *path = vfs_pwd(g_vfs);
    if (path) {
        printf("%s\n", path);
        free(path);
    }
    return 0;
}

/* ---- undo -------------------------------------------------------------- */
static int cmd_undo(void) {
    if (!vfs_session_undo(g_session)) {
        fprintf(stderr, "undo: nothing to undo\n");
        return 1;
    }
    printf("Undo successful.\n");
    return 0;
}

/* ---- redo -------------------------------------------------------------- */
static int cmd_redo(void) {
    if (!vfs_session_redo(g_session)) {
        fprintf(stderr, "redo: nothing to redo\n");
        return 1;
    }
    printf("Redo successful.\n");
    return 0;
}

/* ---- save (persistence) ------------------------------------------------ */
static int cmd_save(const char *host_path) {
    if (!host_path || !*host_path) {
        fprintf(stderr, "save: missing filename (host OS path)\n");
        return 1;
    }
    PersistenceError err = persistence_save_vfs(g_vfs, host_path);
    if (err != PERSIST_OK) {
        fprintf(stderr, "save: %s\n", persistence_error_string(err));
        return 1;
    }
    printf("VFS saved to '%s'.\n", host_path);
    return 0;
}

/* ---- load (persistence) ------------------------------------------------ */
static int cmd_load(const char *host_path) {
    if (!host_path || !*host_path) {
        fprintf(stderr, "load: missing filename (host OS path)\n");
        return 1;
    }

    PersistenceError err;
    Vfs *loaded = persistence_load_vfs(host_path, &err);
    if (!loaded) {
        fprintf(stderr, "load: %s\n", persistence_error_string(err));
        return 1;
    }

    /*
     * Replace current state with the loaded VFS.
     *
     * VfsSession internally owns its own VFS and doesn't expose a setter,
     * so after loading we point g_vfs at the loaded tree.  The session
     * keeps its own (now-unused) VFS; cd/mkdir/touch detect the mismatch
     * and fall back to raw VFS calls, so everything still works correctly.
     * Undo/redo history is naturally reset after a load.
     */
    vfs_clipboard_clear(g_clipboard);
    vfs_session_destroy(g_session);

    g_session = vfs_session_create();
    g_vfs = loaded;

    if (!g_session) {
        fprintf(stderr, "load: warning — session could not be recreated; undo/redo unavailable\n");
    }

    printf("VFS loaded from '%s'. Undo/redo history has been reset.\n", host_path);
    return 0;
}

/* ---- clear ------------------------------------------------------------- */
static int cmd_clear(void) {
    /* ANSI escape to clear screen and move cursor to top-left */
    printf("\033[2J\033[H");
    fflush(stdout);
    return 0;
}

/* ---- help -------------------------------------------------------------- */
static int cmd_help(void) {
    printf("\n");
    printf("  LIMA Virtual File System — Command Reference\n");
    printf("  =============================================\n\n");
    printf("  Navigation:\n");
    printf("    ls                  List contents of current directory\n");
    printf("    cd <path>           Change directory (supports /, .., relative & absolute)\n");
    printf("    pwd                 Print working directory\n");
    printf("    back                Go to previous directory (browser-style)\n");
    printf("    forward             Go to next directory (browser-style)\n\n");
    printf("  File Operations:\n");
    printf("    mkdir <name>        Create a new directory\n");
    printf("    touch <name>        Create a new empty file\n");
    printf("    rm <name>           Remove a file or directory\n");
    printf("    edit <name>         Open file in the built-in text editor\n\n");
    printf("  Clipboard:\n");
    printf("    copy <name>         Copy a file/directory to clipboard\n");
    printf("    cut <name>          Cut a file/directory to clipboard\n");
    printf("    paste               Paste from clipboard into current directory\n\n");
    printf("  History:\n");
    printf("    undo                Undo last tracked operation\n");
    printf("    redo                Redo last undone operation\n\n");
    printf("  Persistence:\n");
    printf("    save <file>         Save VFS to a file on the host OS\n");
    printf("    load <file>         Load VFS from a file on the host OS\n\n");
    printf("  Misc:\n");
    printf("    clear               Clear the screen\n");
    printf("    help                Show this help message\n");
    printf("    exit                Exit LIMA\n\n");
    return 0;
}

/* ========================================================================
 * Utility: strip leading/trailing whitespace in-place.
 * Returns pointer into the same buffer (do NOT free the return value
 * separately from the original buffer).
 * ======================================================================== */
static char *str_trim(char *s) {
    if (!s) return s;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/* ========================================================================
 * Command dispatch
 *
 * Tokenizes the input line and routes to the appropriate handler.
 * Returns:
 *   0  — command executed (success or handled failure)
 *   -1 — user typed "exit"; caller should break the REPL
 * ======================================================================== */
static int dispatch(char *line) {
    char *trimmed = str_trim(line);
    if (*trimmed == '\0') return 0; /* blank line — no-op */

    /* Split into command and argument (everything after the first space) */
    char *cmd = trimmed;
    char *arg = NULL;

    char *space = strchr(trimmed, ' ');
    if (space) {
        *space = '\0';
        arg = str_trim(space + 1);
        if (*arg == '\0') arg = NULL;
    }

    /* ---- dispatch table ---- */
    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        return -1;

    } else if (strcmp(cmd, "help") == 0) {
        cmd_help();

    } else if (strcmp(cmd, "pwd") == 0) {
        cmd_pwd();

    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();

    } else if (strcmp(cmd, "cd") == 0) {
        cmd_cd(arg);

    } else if (strcmp(cmd, "mkdir") == 0) {
        cmd_mkdir(arg);

    } else if (strcmp(cmd, "touch") == 0) {
        cmd_touch(arg);

    } else if (strcmp(cmd, "rm") == 0) {
        cmd_delete(arg);

    } else if (strcmp(cmd, "edit") == 0) {
        cmd_edit(arg);

    } else if (strcmp(cmd, "copy") == 0) {
        cmd_copy(arg);

    } else if (strcmp(cmd, "cut") == 0) {
        cmd_cut(arg);

    } else if (strcmp(cmd, "paste") == 0) {
        cmd_paste();

    } else if (strcmp(cmd, "back") == 0) {
        cmd_back();

    } else if (strcmp(cmd, "forward") == 0) {
        cmd_front();

    } else if (strcmp(cmd, "undo") == 0) {
        cmd_undo();

    } else if (strcmp(cmd, "redo") == 0) {
        cmd_redo();

    } else if (strcmp(cmd, "save") == 0) {
        cmd_save(arg);

    } else if (strcmp(cmd, "load") == 0) {
        cmd_load(arg);

    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();

    } else {
        fprintf(stderr, "'%s': unknown command.  Type 'help' for a list of commands.\n", cmd);
    }

    return 0;
}

/* ========================================================================
 * main() — REPL entry point
 * ======================================================================== */
int main(void) {
    /* 1. Initialise core subsystems */
    g_session = vfs_session_create();
    if (!g_session) {
        fprintf(stderr, "fatal: could not create VFS session\n");
        return 1;
    }
    g_vfs = vfs_session_vfs(g_session);

    g_clipboard = vfs_clipboard_create();
    if (!g_clipboard) {
        fprintf(stderr, "fatal: could not create clipboard\n");
        vfs_session_destroy(g_session);
        return 1;
    }

    /* 2. Welcome banner */
    printf("\n");
    printf("  +------------------------------------------+\n");
    printf("  |   LIMA — Lightweight In-Memory Archive   |\n");
    printf("  |   Virtual File System Shell v1.0         |\n");
    printf("  |   Type 'help' for a list of commands.    |\n");
    printf("  +------------------------------------------+\n");
    printf("\n");

    /* 3. Read-Eval-Print Loop */
    char input[INPUT_MAX];

    for (;;) {
        /* Build the prompt:  "cwd$ " */
        char *cwd_path = vfs_pwd(g_vfs);
        printf("%s$ ", cwd_path ? cwd_path : "?");
        free(cwd_path);
        fflush(stdout);

        /* Read a line of input */
        if (!fgets(input, sizeof(input), stdin)) {
            /* EOF (Ctrl+D / Ctrl+Z) */
            printf("\n");
            break;
        }

        /* Dispatch */
        if (dispatch(input) == -1) {
            break;
        }
    }

    /* 4. Cleanup */
    printf("Goodbye.\n");
    vfs_clipboard_destroy(g_clipboard);
    vfs_session_destroy(g_session);

    return 0;
}