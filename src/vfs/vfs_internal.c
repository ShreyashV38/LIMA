#include "vfs_internal.h"

#include <stdlib.h>
#include <string.h>

char *vfs__strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

bool vfs__name_is_valid(const char *name) {
    if (!name) return false;
    if (*name == '\0') return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;
    if (strchr(name, '/')) return false;
    return true;
}

void vfs__entry_free(void *p) {
    if (!p) return;
    VfsEntry *e = (VfsEntry *)p;
    free(e->name);
    if (e->file_buf) gb_destroy(e->file_buf);
    free(e);
}

VfsEntry *vfs__entry_create(const char *name, VfsNodeType type) {
    VfsEntry *e = (VfsEntry *)malloc(sizeof(VfsEntry));
    if (!e) return NULL;

    e->name = vfs__strdup(name ? name : "");
    if (!e->name) {
        free(e);
        return NULL;
    }

    e->type = type;
    e->file_buf = NULL;

    if (type == VFS_NODE_FILE) {
        /*
          We keep file content in a gap buffer so editor operations stay fast.
          Capacity is a heuristic; it grows automatically.
        */
        e->file_buf = gb_create(128);
        if (!e->file_buf) {
            free(e->name);
            free(e);
            return NULL;
        }
    }

    return e;
}

NaryTreeNode *vfs__find_child_by_name(const NaryTreeNode *dir, const char *name) {
    if (!dir || !name) return NULL;
    for (NaryTreeNode *c = ntree_node_get_first_child(dir); c; c = ntree_node_get_next_sibling(c)) {
        VfsEntry *e = (VfsEntry *)ntree_node_get_data(c);
        if (e && e->name && strcmp(e->name, name) == 0) return c;
    }
    return NULL;
}

bool vfs__add_child_entry_at(Vfs *vfs, NaryTreeNode *dir_node, const char *name, VfsNodeType type) {
    (void)vfs;
    if (!dir_node) return false;
    VfsEntry *dir_e = (VfsEntry *)ntree_node_get_data(dir_node);
    if (!dir_e || dir_e->type != VFS_NODE_DIR) return false;
    if (!vfs__name_is_valid(name)) return false;
    if (vfs__find_child_by_name(dir_node, name)) return false;

    VfsEntry *e = vfs__entry_create(name, type);
    if (!e) return false;
    NaryTreeNode *n = ntree_node_create(e);
    if (!n) {
        vfs__entry_free(e);
        return false;
    }
    ntree_append_child(dir_node, n);
    return true;
}

void vfs__destroy_subtree(NaryTreeNode *node) {
    if (!node) return;

    // 1. Recursively delete all children first (Bottom-Up / Post-Order)
    NaryTreeNode *child = ntree_node_get_first_child(node);
    while (child != NULL) {
        NaryTreeNode *next_sibling = ntree_node_get_next_sibling(child);
        vfs__destroy_subtree(child); 
        child = next_sibling;
    }

    // 2. Free the internal payload data
    VfsEntry *entry = (VfsEntry *)ntree_node_get_data(node);
    if (entry) {
        vfs__entry_free(entry);
    }

    // 3. Free the N-ary Tree node shell itself
    free(node);
}