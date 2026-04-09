#include "vfs_internal.h"

#include <stdlib.h>
#include <string.h>

static NaryTreeNode *vfs_resolve_from(NaryTreeNode *start, const char *path) {
    if (!start || !path) return NULL;
    NaryTreeNode *cur = start;

    /*
      Minimal path traversal:
      - treats multiple '/' as separators
      - supports "." and ".."
      - only resolves directories (cd into files is rejected)
    */
    const char *p = path;
    while (*p == '/') p++;

    while (*p) {
        const char *end = p;
        while (*end && *end != '/') end++;
        size_t tlen = (size_t)(end - p);

        if (tlen == 0) {
            /* repeated slashes */
        } else if (tlen == 1 && p[0] == '.') {
            /* no-op */
        } else if (tlen == 2 && p[0] == '.' && p[1] == '.') {
            NaryTreeNode *parent = ntree_node_get_parent(cur);
            if (parent) cur = parent;
        } else {
            char *name = (char *)malloc(tlen + 1);
            if (!name) return NULL;
            memcpy(name, p, tlen);
            name[tlen] = '\0';

            NaryTreeNode *child = vfs__find_child_by_name(cur, name);
            free(name);
            if (!child) return NULL;

            VfsEntry *ce = (VfsEntry *)ntree_node_get_data(child);
            if (!ce || ce->type != VFS_NODE_DIR) return NULL;
            cur = child;
        }

        p = end;
        while (*p == '/') p++;
    }

    return cur;
}

bool vfs_cd(Vfs *vfs, const char *path) {
    if (!vfs || !vfs->tree || !vfs->cwd || !path) return false;

    if (strcmp(path, "/") == 0) {
        vfs->cwd = ntree_get_root(vfs->tree);
        return vfs->cwd != NULL;
    }

    NaryTreeNode *start = vfs->cwd;
    if (path[0] == '/') start = ntree_get_root(vfs->tree);

    NaryTreeNode *resolved = vfs_resolve_from(start, path);
    if (!resolved) return false;
    vfs->cwd = resolved;
    return true;
}

