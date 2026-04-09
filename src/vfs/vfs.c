#include "../../include/vfs/vfs.h"

#include <stdlib.h>
#include <string.h>

typedef struct VfsEntry {
    char *name;
    VfsNodeType type;
    GapBuffer *file_buf; // only for files
} VfsEntry;

struct Vfs {
    NaryTree *tree;
    NaryTreeNode *cwd;
};

static char *vfs_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static void vfs_entry_free(void *p) {
    if (!p) return;
    VfsEntry *e = (VfsEntry *)p;
    free(e->name);
    if (e->file_buf) gb_destroy(e->file_buf);
    free(e);
}

static VfsEntry *vfs_entry_create(const char *name, VfsNodeType type) {
    VfsEntry *e = (VfsEntry *)malloc(sizeof(VfsEntry));
    if (!e) return NULL;
    e->name = vfs_strdup(name ? name : "");
    if (!e->name) {
        free(e);
        return NULL;
    }
    e->type = type;
    e->file_buf = NULL;
    if (type == VFS_NODE_FILE) {
        e->file_buf = gb_create(128);
        if (!e->file_buf) {
            free(e->name);
            free(e);
            return NULL;
        }
    }
    return e;
}

static NaryTreeNode *vfs_find_child_by_name(const NaryTreeNode *dir, const char *name) {
    if (!dir || !name) return NULL;
    for (NaryTreeNode *c = ntree_node_get_first_child(dir); c; c = ntree_node_get_next_sibling(c)) {
        VfsEntry *e = (VfsEntry *)ntree_node_get_data(c);
        if (e && e->name && strcmp(e->name, name) == 0) return c;
    }
    return NULL;
}

static bool vfs_name_is_valid(const char *name) {
    if (!name) return false;
    if (*name == '\0') return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;
    if (strchr(name, '/')) return false;
    return true;
}

Vfs *vfs_create(void) {
    Vfs *vfs = (Vfs *)malloc(sizeof(Vfs));
    if (!vfs) return NULL;

    vfs->tree = ntree_create();
    vfs->cwd = NULL;
    if (!vfs->tree) {
        free(vfs);
        return NULL;
    }

    VfsEntry *root_e = vfs_entry_create("/", VFS_NODE_DIR);
    if (!root_e) {
        ntree_destroy(vfs->tree, NULL);
        free(vfs);
        return NULL;
    }

    NaryTreeNode *root = ntree_node_create(root_e);
    if (!root) {
        vfs_entry_free(root_e);
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
    ntree_destroy(vfs->tree, vfs_entry_free);
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

static size_t vfs_depth_to_root(const NaryTreeNode *node) {
    size_t d = 0;
    for (const NaryTreeNode *cur = node; cur; cur = ntree_node_get_parent(cur)) d++;
    return d;
}

char *vfs_pwd(const Vfs *vfs) {
    if (!vfs || !vfs->cwd) return NULL;

    // collect nodes from cwd up to root
    size_t depth = vfs_depth_to_root(vfs->cwd);
    const NaryTreeNode **stack = (const NaryTreeNode **)malloc(sizeof(NaryTreeNode *) * depth);
    if (!stack) return NULL;

    size_t i = 0;
    for (const NaryTreeNode *cur = vfs->cwd; cur; cur = ntree_node_get_parent(cur)) {
        stack[i++] = cur;
    }

    // compute length (canonical: "/" or "/a/b")
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

    size_t len = 1; // leading "/"
    if (seg_count > 0) {
        len += seg_chars;
        if (seg_count > 1) len += (seg_count - 1); // internal slashes between segments
    }

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

static bool vfs_add_child_entry(Vfs *vfs, const char *name, VfsNodeType type) {
    if (!vfs || !vfs->cwd) return false;
    VfsEntry *cwd_e = (VfsEntry *)ntree_node_get_data(vfs->cwd);
    if (!cwd_e || cwd_e->type != VFS_NODE_DIR) return false;
    if (!vfs_name_is_valid(name)) return false;
    if (vfs_find_child_by_name(vfs->cwd, name)) return false;

    VfsEntry *e = vfs_entry_create(name, type);
    if (!e) return false;
    NaryTreeNode *n = ntree_node_create(e);
    if (!n) {
        vfs_entry_free(e);
        return false;
    }
    ntree_append_child(vfs->cwd, n);
    return true;
}

bool vfs_mkdir(Vfs *vfs, const char *name) {
    return vfs_add_child_entry(vfs, name, VFS_NODE_DIR);
}

bool vfs_touch(Vfs *vfs, const char *name) {
    return vfs_add_child_entry(vfs, name, VFS_NODE_FILE);
}

static NaryTreeNode *vfs_resolve_from(NaryTreeNode *start, const char *path) {
    if (!start || !path) return NULL;
    NaryTreeNode *cur = start;

    const char *p = path;
    while (*p == '/') p++;

    while (*p) {
        // token [p, end)
        const char *end = p;
        while (*end && *end != '/') end++;
        size_t tlen = (size_t)(end - p);

        if (tlen == 0) {
            // skip repeated slashes
        } else if (tlen == 1 && p[0] == '.') {
            // no-op
        } else if (tlen == 2 && p[0] == '.' && p[1] == '.') {
            NaryTreeNode *parent = ntree_node_get_parent(cur);
            if (parent) cur = parent;
        } else {
            char *name = (char *)malloc(tlen + 1);
            if (!name) return NULL;
            memcpy(name, p, tlen);
            name[tlen] = '\0';

            NaryTreeNode *child = vfs_find_child_by_name(cur, name);
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
    if (path[0] == '/') {
        start = ntree_get_root(vfs->tree);
    }

    NaryTreeNode *resolved = vfs_resolve_from(start, path);
    if (!resolved) return false;
    vfs->cwd = resolved;
    return true;
}

char **vfs_ls(const Vfs *vfs, size_t *out_count) {
    if (out_count) *out_count = 0;
    if (!vfs || !vfs->cwd) return NULL;

    // count
    size_t n = 0;
    for (NaryTreeNode *c = ntree_node_get_first_child(vfs->cwd); c; c = ntree_node_get_next_sibling(c)) n++;
    if (out_count) *out_count = n;
    if (n == 0) return NULL;

    char **arr = (char **)calloc(n, sizeof(char *));
    if (!arr) return NULL;

    size_t i = 0;
    for (NaryTreeNode *c = ntree_node_get_first_child(vfs->cwd); c; c = ntree_node_get_next_sibling(c)) {
        const char *name = vfs_node_name(c);
        arr[i++] = vfs_strdup(name ? name : "");
    }

    return arr;
}

GapBuffer *vfs_open_file(Vfs *vfs, const char *name) {
    if (!vfs || !vfs->cwd || !name) return NULL;
    NaryTreeNode *child = vfs_find_child_by_name(vfs->cwd, name);
    if (!child) return NULL;
    VfsEntry *e = (VfsEntry *)ntree_node_get_data(child);
    if (!e || e->type != VFS_NODE_FILE) return NULL;
    return e->file_buf;
}

