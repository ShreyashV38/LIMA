/*
 * main.c - CLI entry point for LIMA VFS
 * Team: Anosh, Anish, Shreyash, Samuel, Zilu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/vfs/vfs.h"
#include "../include/ui/editor.h"
#include "../include/data_structures/gap_buffer.h"

/* * Global VFS instance that the CLI commands will operate on.
 * This will be initialized in the main() function when we write it.
 */
Vfs *g_vfs = NULL;

/* Command handlers */
extern int cmd_ls(void);
extern int cmd_cd(const char *path);
extern int cmd_back(void);
extern int cmd_front(void);
extern int cmd_mkdir(const char *name);
extern int cmd_touch(const char *name);
extern int cmd_copy(const char *path);
extern int cmd_cut(const char *path);
extern int cmd_paste(void);
extern int cmd_delete(const char *path);

/**
 * @brief Edit a file using the text editor
 * @param filepath Path to the file to edit (in current directory)
 * @return 0 on success, 1 on failure
 */
int cmd_edit(const char *filepath) {
    if (!g_vfs || !filepath) return 1;

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