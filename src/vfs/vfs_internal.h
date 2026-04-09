#ifndef VFS_INTERNAL_H
#define VFS_INTERNAL_H

/*
  Internal implementation details for the in-memory VFS.

  Public callers should only include `include/vfs/vfs.h`.
  The functions/types here exist to keep the implementation split across files
  while still sharing core helpers (entry allocation, name validation, etc.).
*/

#include "../../include/vfs/vfs.h"

typedef struct VfsEntry {
    char *name;
    VfsNodeType type;
    GapBuffer *file_buf; /* only for files */
} VfsEntry;

struct Vfs {
    NaryTree *tree;
    NaryTreeNode *cwd;
};

char *vfs__strdup(const char *s);
bool vfs__name_is_valid(const char *name);
VfsEntry *vfs__entry_create(const char *name, VfsNodeType type);
void vfs__entry_free(void *p);
NaryTreeNode *vfs__find_child_by_name(const NaryTreeNode *dir, const char *name);

bool vfs__add_child_entry_at(Vfs *vfs, NaryTreeNode *dir_node, const char *name, VfsNodeType type);
void vfs__destroy_subtree(NaryTreeNode *node);

#endif /* VFS_INTERNAL_H */

