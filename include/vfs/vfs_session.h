#ifndef VFS_SESSION_H
#define VFS_SESSION_H

#include <stdbool.h>

#include "vfs.h"

typedef struct VfsSession VfsSession;

VfsSession *vfs_session_create(void);
void vfs_session_destroy(VfsSession *s);

Vfs *vfs_session_vfs(VfsSession *s);

// FS operations tracked for undo/redo
bool vfs_session_mkdir(VfsSession *s, const char *name);
bool vfs_session_touch(VfsSession *s, const char *name);
bool vfs_session_cd(VfsSession *s, const char *path);

// Navigation history (like a file explorer)
bool vfs_session_back(VfsSession *s);
bool vfs_session_forward(VfsSession *s);

// Undo/redo last tracked operation
bool vfs_session_undo(VfsSession *s);
bool vfs_session_redo(VfsSession *s);

#endif // VFS_SESSION_H

