#include "vfs_internal.h"

#include <stdlib.h>

Vfs *vfs_create(void) {
    Vfs *vfs = (Vfs *)malloc(sizeof(Vfs));
    if (!vfs) return NULL;

    vfs->tree = ntree_create();
    vfs->cwd = NULL;
    if (!vfs->tree) {
        free(vfs);
        return NULL;
    }

    VfsEntry *root_e = vfs__entry_create("/", VFS_NODE_DIR);
    if (!root_e) {
        ntree_destroy(vfs->tree, NULL);
        free(vfs);
        return NULL;
    }

    NaryTreeNode *root = ntree_node_create(root_e);
    if (!root) {
        vfs__entry_free(root_e);
        ntree_destroy(vfs->tree, NULL);
        free(vfs);
        return NULL;
    }

    ntree_set_root(vfs->tree, root);
    vfs->cwd = root;
    return vfs;
}

void vfs_destroy(Vfs *vfs) {
    if (!vfs) return;
    ntree_destroy(vfs->tree, vfs__entry_free);
    free(vfs);
}

NaryTreeNode *vfs_cwd(const Vfs *vfs) {
    return vfs ? vfs->cwd : NULL;
}

VfsNodeType vfs_node_type(const NaryTreeNode *node) {
    VfsEntry *e = (VfsEntry *)ntree_node_get_data(node);
    return e ? e->type : VFS_NODE_DIR;
}

const char *vfs_node_name(const NaryTreeNode *node) {
    VfsEntry *e = (VfsEntry *)ntree_node_get_data(node);
    return e ? e->name : NULL;
}

static bool vfs_add_child_entry(Vfs *vfs, const char *name, VfsNodeType type) {
    if (!vfs || !vfs->cwd) return false;
    return vfs__add_child_entry_at(vfs, vfs->cwd, name, type);
}

bool vfs_mkdir(Vfs *vfs, const char *name) {
    return vfs_add_child_entry(vfs, name, VFS_NODE_DIR);
}

bool vfs_touch(Vfs *vfs, const char *name) {
    return vfs_add_child_entry(vfs, name, VFS_NODE_FILE);
}

bool vfs_mkdir_at(Vfs *vfs, NaryTreeNode *dir_node, const char *name) {
    return vfs__add_child_entry_at(vfs, dir_node, name, VFS_NODE_DIR);
}

bool vfs_touch_at(Vfs *vfs, NaryTreeNode *dir_node, const char *name) {
    return vfs__add_child_entry_at(vfs, dir_node, name, VFS_NODE_FILE);
}

bool vfs_rm_at(Vfs *vfs, NaryTreeNode *dir_node, const char *name) {
    (void)vfs;
    if (!dir_node || !name) return false;
    VfsEntry *dir_e = (VfsEntry *)ntree_node_get_data(dir_node);
    if (!dir_e || dir_e->type != VFS_NODE_DIR) return false;

    NaryTreeNode *child = vfs__find_child_by_name(dir_node, name);
    if (!child) return false;

    NaryTreeNode *detached = ntree_remove_child(dir_node, child);
    if (!detached) return false;

    vfs__destroy_subtree(detached);
    return true;
}

bool vfs_rm(Vfs *vfs, const char *name) {
    if (!vfs || !vfs->cwd) return false;
    return vfs_rm_at(vfs, vfs->cwd, name);
}

bool vfs_chdir_node(Vfs *vfs, NaryTreeNode *dir_node) {
    if (!vfs || !dir_node) return false;
    VfsEntry *e = (VfsEntry *)ntree_node_get_data(dir_node);
    if (!e || e->type != VFS_NODE_DIR) return false;
    vfs->cwd = dir_node;
    return true;
}

char **vfs_ls(const Vfs *vfs, size_t *out_count) {
    if (out_count) *out_count = 0;
    if (!vfs || !vfs->cwd) return NULL;

    size_t n = 0;
    for (NaryTreeNode *c = ntree_node_get_first_child(vfs->cwd); c; c = ntree_node_get_next_sibling(c)) n++;
    if (out_count) *out_count = n;
    if (n == 0) return NULL;

    char **arr = (char **)calloc(n, sizeof(char *));
    if (!arr) return NULL;

    size_t i = 0;
    for (NaryTreeNode *c = ntree_node_get_first_child(vfs->cwd); c; c = ntree_node_get_next_sibling(c)) {
        const char *name = vfs_node_name(c);
        arr[i++] = vfs__strdup(name ? name : "");
    }

    return arr;
}

GapBuffer *vfs_open_file(Vfs *vfs, const char *name) {
    if (!vfs || !vfs->cwd || !name) return NULL;
    NaryTreeNode *child = vfs__find_child_by_name(vfs->cwd, name);
    if (!child) return NULL;
    VfsEntry *e = (VfsEntry *)ntree_node_get_data(child);
    if (!e || e->type != VFS_NODE_FILE) return NULL;
    return e->file_buf;
}

