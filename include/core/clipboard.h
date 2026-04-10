#ifndef VFS_CLIPBOARD_H
#define VFS_CLIPBOARD_H

#include <stdbool.h>
#include "../vfs/vfs.h"
#include "../data_structures/nary_tree.h"

// Enum to track how the node got into the clipboard
typedef enum ClipboardOp {
    CLIPBOARD_EMPTY = 0,
    CLIPBOARD_CUT = 1,
    CLIPBOARD_COPY = 2
} ClipboardOp;

typedef struct VfsClipboard {
    NaryTreeNode *node;     // Pointer to the tree node held in the clipboard
    ClipboardOp current_op; // State of the clipboard
} VfsClipboard;

/**
 * @brief Create a new Clipboard instance.
 */
VfsClipboard *vfs_clipboard_create(void);

/**
 * @brief Destroy the clipboard. If a 'Cut' node is still held and unpasted, it must be freed.
 */
void vfs_clipboard_destroy(VfsClipboard *cb);

/**
 * @brief O(1) Cut Operation: Detaches the specified file/dir from the VFS current directory 
 * and stores its pointer in the clipboard.
 */
bool vfs_clipboard_cut(VfsClipboard *cb, Vfs *vfs, const char *name);

/**
 * @brief O(N) Copy Operation: Creates a deep recursive clone of the specified file/dir 
 * from the VFS and stores the newly allocated clone in the clipboard.
 */
bool vfs_clipboard_copy(VfsClipboard *cb, Vfs *vfs, const char *name);

/**
 * @brief Paste Operation: Appends the clipboard node to the VFS current directory.
 * - If it was a CUT: append the exact node, then clear the clipboard.
 * - If it was a COPY: append a fresh deep clone of the clipboard node (so we can paste again).
 */
bool vfs_clipboard_paste(VfsClipboard *cb, Vfs *vfs);

/**
 * @brief Clear the clipboard safely without pasting.
 */
void vfs_clipboard_clear(VfsClipboard *cb);

#endif // VFS_CLIPBOARD_H