#include "vfs_internal.h"

#include <stdlib.h>
#include <string.h>

static size_t vfs_depth_to_root(const NaryTreeNode *node) {
    size_t d = 0;
    for (const NaryTreeNode *cur = node; cur; cur = ntree_node_get_parent(cur)) d++;
    return d;
}

char *vfs_pwd(const Vfs *vfs) {
    if (!vfs || !vfs->cwd) return NULL;

    /*
      Construct a canonical absolute path.
      - root: "/"
      - child: "/a"
      - deeper: "/a/b"
    */
    size_t depth = vfs_depth_to_root(vfs->cwd);
    const NaryTreeNode **stack = (const NaryTreeNode **)malloc(sizeof(NaryTreeNode *) * depth);
    if (!stack) return NULL;

    size_t i = 0;
    for (const NaryTreeNode *cur = vfs->cwd; cur; cur = ntree_node_get_parent(cur)) {
        stack[i++] = cur;
    }

    size_t seg_count = 0;
    size_t seg_chars = 0;
    for (size_t j = 0; j < i; j++) {
        const NaryTreeNode *n = stack[i - 1 - j];
        const char *name = vfs_node_name(n);
        if (!name) continue;
        if (strcmp(name, "/") == 0) continue;
        seg_count++;
        seg_chars += strlen(name);
    }

    size_t len = 1; /* leading "/" */
    if (seg_count == 0) {
        free(stack);
        return vfs__strdup("/ (root)");
    }
    
    len += seg_chars;
    if (seg_count > 1) len += (seg_count - 1); /* internal slashes */

    char *out = (char *)malloc(len + 1);
    if (!out) {
        free(stack);
        return NULL;
    }

    size_t pos = 0;
    out[pos++] = '/';
    for (size_t j = 0; j < i; j++) {
        const NaryTreeNode *n = stack[i - 1 - j];
        const char *name = vfs_node_name(n);
        if (!name) continue;
        if (strcmp(name, "/") == 0) continue;
        if (pos > 1) out[pos++] = '/';
        size_t nlen = strlen(name);
        memcpy(out + pos, name, nlen);
        pos += nlen;
    }
    out[pos] = '\0';

    free(stack);
    return out;
}

