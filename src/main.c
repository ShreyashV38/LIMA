/*
 * main.c - CLI entry point for LIMA VFS
 * Team: Anosh, Anish, Shreyash, Samuel, Zilu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core/vfs.h"
#include "../include/ui/editor.h"

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
 * @param filepath Path to the file to edit
 * @return 0 on success, 1 on failure
 */
int cmd_edit(const char *filepath) {
    /* Resolve the file path */
    VFSNode *file_node = vfs_resolve_path(filepath);
    if (!file_node) {
        fprintf(stderr, "edit: cannot access '%s': No such file\n", filepath);
        return 1;
    }
    
    /* Check if it's a file (not directory) */
    if (vfs_node_is_directory(file_node)) {
        fprintf(stderr, "edit: '%s' is a directory\n", filepath);
        return 1;
    }
    
    /* Get current file content */
    size_t content_len;
    char *initial_content = vfs_file_get_content(file_node, &content_len);
    if (!initial_content) {
        initial_content = strdup("");
    }
    
    /* Create editor with file content */
    const char *filename = vfs_node_get_name(file_node);
    Editor *ed = editor_create(filename, initial_content);
    if (!ed) {
        fprintf(stderr, "edit: failed to create editor\n");
        free(initial_content);
        return 1;
    }
    
    /* Run the editor (blocks until Escape) */
    editor_run(ed);
    
    /* Get edited content and save back */
    size_t new_len;
    char *new_content = editor_get_content(ed, &new_len);
    if (new_content) {
        vfs_file_set_content(file_node, new_content, new_len);
        free(new_content);
    }
    
    /* Cleanup */
    editor_destroy(ed);
    free(initial_content);
    
    return 0;
}
