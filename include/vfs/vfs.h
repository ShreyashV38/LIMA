#ifndef VFS_H
#define VFS_H

#include <stdbool.h>
#include <stddef.h>

#include "../data_structures/gap_buffer.h"
#include "../data_structures/nary_tree.h"

typedef struct Vfs Vfs;

typedef enum VfsNodeType {
    VFS_NODE_DIR = 0,
    VFS_NODE_FILE = 1
} VfsNodeType;

/**
 * Create an in-memory virtual filesystem with a single root directory ("/").
 */
Vfs *vfs_create(void);

/**
 * Destroy the VFS and all contained nodes (including file buffers).
 */
void vfs_destroy(Vfs *vfs);

/**
 * Return the current working directory node.
 */
NaryTreeNode *vfs_cwd(const Vfs *vfs);

/**
 * Return a newly allocated absolute path string for the current working directory.
 * Caller must free.
 */
char *vfs_pwd(const Vfs *vfs);

/**
 * Create a directory under the current directory.
 */
bool vfs_mkdir(Vfs *vfs, const char *name);

/**
 * Create an empty file under the current directory (like touch).
 */
bool vfs_touch(Vfs *vfs, const char *name);

/**
 * Change current directory. Supports:
 * - "/" (root)
 * - ".." (parent)
 * - "." (no-op)
 * - "name" (child directory)
 * - simple slash paths like "a/b/.." and absolute "/a/b"
 */
bool vfs_cd(Vfs *vfs, const char *path);

/**
 * List children names in the current directory.
 * Returns a newly allocated array of newly allocated strings; caller frees each string and the array.
 */
char **vfs_ls(const Vfs *vfs, size_t *out_count);

/**
 * Open a file in the current directory and return its GapBuffer (owned by the VFS).
 * Returns NULL if not found or not a file.
 */
GapBuffer *vfs_open_file(Vfs *vfs, const char *name);

/**
 * Helpers for tests / callers: extract node type + name from a node.
 */
VfsNodeType vfs_node_type(const NaryTreeNode *node);
const char *vfs_node_name(const NaryTreeNode *node);

#endif // VFS_H

