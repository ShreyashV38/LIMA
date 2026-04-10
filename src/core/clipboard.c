#include "../../include/core/clipboard.h"
#include "vfs_internal.h" // Needed for VfsEntry, vfs__find_child_by_name, etc.
#include <stdlib.h>
#include <string.h>

VfsClipboard *vfs_clipboard_create(void) {
    VfsClipboard *cb = (VfsClipboard *)malloc(sizeof(VfsClipboard));
    if (!cb) return NULL;
    cb->node = NULL;
    cb->current_op = CLIPBOARD_EMPTY;
    return cb;
}

void vfs_clipboard_clear(VfsClipboard *cb) {
    if (!cb || cb->current_op == CLIPBOARD_EMPTY) return;
    
    // If a user cuts a node but never pastes it, we must prevent a memory leak 
    // by destroying the detached subtree when the clipboard clears.
    if (cb->current_op == CLIPBOARD_CUT || cb->current_op == CLIPBOARD_COPY) {
        if (cb->node) {
            vfs__destroy_subtree(cb->node); // An internal function Anosh provided
        }
    }
    
    cb->node = NULL;
    cb->current_op = CLIPBOARD_EMPTY;
}

void vfs_clipboard_destroy(VfsClipboard *cb) {
    if (!cb) return;
    vfs_clipboard_clear(cb);
    free(cb);
}

// ----------------------------------------------------------------------------
// Recursive Deep Clone O(N)
// ----------------------------------------------------------------------------
static NaryTreeNode *vfs__deep_clone_subtree(NaryTreeNode *original_node) {
    if (!original_node) return NULL;

    // 1. Get the original data (VfsEntry)
    VfsEntry *orig_entry = (VfsEntry *)ntree_node_get_data(original_node);
    if (!orig_entry) return NULL;

    // 2. Allocate a NEW VfsEntry 
    // vfs__entry_create automatically allocates a fresh GapBuffer if it's a file.
    VfsEntry *new_entry = vfs__entry_create(orig_entry->name, orig_entry->type);
    if (!new_entry) return NULL;

    // 3. If it is a FILE, deep copy the GapBuffer contents
    if (orig_entry->type == VFS_NODE_FILE && orig_entry->file_buf && new_entry->file_buf) {
        size_t text_len = 0;
        char *text = gb_get_text(orig_entry->file_buf, &text_len);
        if (text) {
            // Insert the extracted text into the new file's gap buffer
            gb_insert(new_entry->file_buf, text, text_len);
            free(text); // gb_get_text allocates memory, so we must free it to avoid leaks!
        }
    }

    // 4. Create the NEW NaryTreeNode
    NaryTreeNode *new_node = ntree_node_create(new_entry);
    if (!new_node) {
        vfs__entry_free(new_entry); // Clean up if node creation fails
        return NULL;
    }

    // 5. Recursively traverse all children of original_node
    NaryTreeNode *child = ntree_node_get_first_child(original_node);
    while (child != NULL) {
        // Recursively clone the child
        NaryTreeNode *cloned_child = vfs__deep_clone_subtree(child);
        
        if (cloned_child) {
            // Attach the cloned child to our newly created parent node
            ntree_append_child(new_node, cloned_child);
        }
        
        // Move to the next sibling
        child = ntree_node_get_next_sibling(child);
    }

    return new_node; // Return the fully cloned branch
}

// ----------------------------------------------------------------------------
// Core Operations
// ----------------------------------------------------------------------------

bool vfs_clipboard_cut(VfsClipboard *cb, Vfs *vfs, const char *name) {
    if (!cb || !vfs || !vfs_cwd(vfs) || !name) return false;

    // Find the node in the current directory
    NaryTreeNode *target = vfs__find_child_by_name(vfs_cwd(vfs), name);
    if (!target) return false;

    // Clear existing clipboard contents
    vfs_clipboard_clear(cb);

    // O(1) CUT: Just remove the pointer from the parent, don't touch the data!
    cb->node = ntree_remove_child(vfs_cwd(vfs), target);
    if (cb->node) {
        cb->current_op = CLIPBOARD_CUT;
        return true;
    }
    return false;
}

bool vfs_clipboard_copy(VfsClipboard *cb, Vfs *vfs, const char *name) {
    if (!cb || !vfs || !vfs_cwd(vfs) || !name) return false;

    NaryTreeNode *target = vfs__find_child_by_name(vfs_cwd(vfs), name);
    if (!target) return false;

    vfs_clipboard_clear(cb);

    // O(N) COPY: Generate a completely new detached subtree in memory
    cb->node = vfs__deep_clone_subtree(target);
    if (cb->node) {
        cb->current_op = CLIPBOARD_COPY;
        return true;
    }
    return false;
}

bool vfs_clipboard_paste(VfsClipboard *cb, Vfs *vfs) {
    if (!cb || !vfs || !vfs_cwd(vfs) || cb->current_op == CLIPBOARD_EMPTY || !cb->node) {
        return false;
    }

    // Check if a file/dir with this name already exists in the destination
    VfsEntry *cb_entry = (VfsEntry *)ntree_node_get_data(cb->node);
    if (vfs__find_child_by_name(vfs_cwd(vfs), cb_entry->name)) {
        // Name collision handling (you could modify this to auto-rename e.g., "file_copy")
        return false; 
    }

    if (cb->current_op == CLIPBOARD_CUT) {
        // O(1) pointer swap: attach the exact node into the tree
        ntree_append_child(vfs_cwd(vfs), cb->node);
        
        // Clear clipboard so we don't accidentally double-paste a cut file
        cb->node = NULL;
        cb->current_op = CLIPBOARD_EMPTY; 
    } 
    else if (cb->current_op == CLIPBOARD_COPY) {
        // We must clone the clipboard node again so the clipboard retains its independent copy
        // allowing the user to paste multiple times in different directories.
        NaryTreeNode *paste_clone = vfs__deep_clone_subtree(cb->node);
        if (!paste_clone) return false;
        
        ntree_append_child(vfs_cwd(vfs), paste_clone);
    }

    return true;
}