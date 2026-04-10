#include "../../include/core/persistence.h"
#include "../vfs/vfs_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// gcc -Wall -g -I./include tests/test_persistence.c src/core/persistence.c src/core/clipboard.c src/vfs/vfs_ops.c src/vfs/vfs_nav.c src/vfs/vfs_pwd.c src/vfs/vfs_internal.c src/vfs/vfs_session.c src/data_structures/nary_tree.c src/data_structures/hash_map.c src/data_structures/gap_buffer.c src/data_structures/stack.c -o test_persistence.exe

// .\test_persistence.exe

const char *persistence_error_string(PersistenceError err) {
    switch(err) {
        case PERSIST_OK: return "OK";
        case PERSIST_ERR_FILE_NOT_FOUND: return "File Not Found or Access Denied";
        case PERSIST_ERR_CORRUPT_MAGIC: return "Corrupt Magic Header";
        case PERSIST_ERR_VERSION_MISMATCH: return "Version Mismatch";
        case PERSIST_ERR_TRUNCATED: return "Truncated File";
        case PERSIST_ERR_ALLOCATION: return "Memory Allocation Failed";
        case PERSIST_ERR_WRITE_FAILED: return "Disk Write Failed";
        case PERSIST_ERR_VFS_STATE: return "Invalid VFS State or Tree";
        default: return "Unknown Error";
    }
}

static PersistenceError serialize_node(FILE *fp, const NaryTreeNode *node) {
    if (!node) return PERSIST_OK;

    VfsEntry *entry = (VfsEntry *)ntree_node_get_data(node);
    if (!entry) return PERSIST_ERR_VFS_STATE;

    uint16_t name_len = (uint16_t)strlen(entry->name);
    if (fwrite(&name_len, sizeof(uint16_t), 1, fp) != 1) return PERSIST_ERR_WRITE_FAILED;
    if (name_len > 0) {
        if (fwrite(entry->name, 1, name_len, fp) != name_len) return PERSIST_ERR_WRITE_FAILED;
    }

    uint8_t type = (uint8_t)entry->type;
    if (fwrite(&type, sizeof(uint8_t), 1, fp) != 1) return PERSIST_ERR_WRITE_FAILED;

    if (type == VFS_NODE_FILE) {
        size_t text_len = 0;
        char *text = gb_get_text(entry->file_buf, &text_len);
        
        uint64_t content_len = (uint64_t)text_len;
        if (fwrite(&content_len, sizeof(uint64_t), 1, fp) != 1) {
            if (text) free(text);
            return PERSIST_ERR_WRITE_FAILED;
        }
        
        if (content_len > 0) {
            if (fwrite(text, 1, text_len, fp) != text_len) {
                if (text) free(text);
                return PERSIST_ERR_WRITE_FAILED;
            }
        }
        if (text) free(text);
    } else {
        uint32_t num_children = 0;
        NaryTreeNode *child = ntree_node_get_first_child(node);
        while (child) {
            num_children++;
            child = ntree_node_get_next_sibling(child);
        }

        if (fwrite(&num_children, sizeof(uint32_t), 1, fp) != 1) return PERSIST_ERR_WRITE_FAILED;

        child = ntree_node_get_first_child(node);
        while (child) {
            PersistenceError e = serialize_node(fp, child);
            if (e != PERSIST_OK) return e;
            child = ntree_node_get_next_sibling(child);
        }
    }

    return PERSIST_OK;
}

PersistenceError persistence_save_vfs(const Vfs *vfs, const char *filepath) {
    if (!vfs || !filepath) return PERSIST_ERR_VFS_STATE;

    FILE *fp = fopen(filepath, "wb");
    if (!fp) return PERSIST_ERR_FILE_NOT_FOUND;
    
    if (fwrite(LIMA_MAGIC, 1, 4, fp) != 4) {
        fclose(fp);
        return PERSIST_ERR_WRITE_FAILED;
    }
    
    uint32_t version = LIMA_PERSIST_VERSION;
    if (fwrite(&version, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return PERSIST_ERR_WRITE_FAILED;
    }

    NaryTreeNode *root = ntree_get_root(vfs->tree);
    if (!root) {
        fclose(fp);
        return PERSIST_ERR_VFS_STATE;
    }

    PersistenceError err = serialize_node(fp, root);
    fclose(fp);
    return err;
}

static NaryTreeNode *deserialize_node(FILE *fp, PersistenceError *err) {
    uint16_t name_len;
    if (fread(&name_len, sizeof(uint16_t), 1, fp) != 1) {
        *err = PERSIST_ERR_TRUNCATED;
        return NULL;
    }
    
    char *name = (char *)malloc(name_len + 1);
    if (!name) {
        *err = PERSIST_ERR_ALLOCATION;
        return NULL;
    }
    
    if (name_len > 0) {
        if (fread(name, 1, name_len, fp) != name_len) {
            free(name);
            *err = PERSIST_ERR_TRUNCATED;
            return NULL;
        }
    }
    name[name_len] = '\0';

    uint8_t type;
    if (fread(&type, sizeof(uint8_t), 1, fp) != 1) {
        free(name);
        *err = PERSIST_ERR_TRUNCATED;
        return NULL;
    }

    if (type != VFS_NODE_DIR && type != VFS_NODE_FILE) {
        free(name);
        *err = PERSIST_ERR_VFS_STATE;
        return NULL;
    }

    VfsEntry *entry = vfs__entry_create(name, type);
    free(name);
    if (!entry) {
        *err = PERSIST_ERR_ALLOCATION;
        return NULL;
    }

    NaryTreeNode *node = ntree_node_create(entry);
    if (!node) {
        vfs__entry_free(entry);
        *err = PERSIST_ERR_ALLOCATION;
        return NULL;
    }

    if (type == VFS_NODE_FILE) {
        uint64_t text_len;
        if (fread(&text_len, sizeof(uint64_t), 1, fp) != 1) {
            *err = PERSIST_ERR_TRUNCATED;
            vfs__destroy_subtree(node);
            return NULL;
        }
        
        if (text_len > 0) {
            char *text = (char *)malloc(text_len);
            if (!text) {
                *err = PERSIST_ERR_ALLOCATION;
                vfs__destroy_subtree(node);
                return NULL;
            }
            if (fread(text, 1, text_len, fp) != text_len) {
                free(text);
                *err = PERSIST_ERR_TRUNCATED;
                vfs__destroy_subtree(node);
                return NULL;
            }
            gb_insert(entry->file_buf, text, text_len);
            free(text);
        }
    } else {
        uint32_t num_children;
        if (fread(&num_children, sizeof(uint32_t), 1, fp) != 1) {
            *err = PERSIST_ERR_TRUNCATED;
            vfs__destroy_subtree(node);
            return NULL;
        }
        for (uint32_t i = 0; i < num_children; i++) {
            NaryTreeNode *child = deserialize_node(fp, err);
            if (!child) {
                // error already set in child
                vfs__destroy_subtree(node);
                return NULL;
            }
            ntree_append_child(node, child);
        }
    }
    *err = PERSIST_OK;
    return node;
}

Vfs *persistence_load_vfs(const char *filepath, PersistenceError *out_error) {
    PersistenceError local_err = PERSIST_OK;
    if (!out_error) out_error = &local_err;

    if (!filepath) {
        *out_error = PERSIST_ERR_FILE_NOT_FOUND;
        return NULL;
    }

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        *out_error = PERSIST_ERR_FILE_NOT_FOUND;
        return NULL;
    }

    char magic[4];
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, LIMA_MAGIC, 4) != 0) {
        *out_error = PERSIST_ERR_CORRUPT_MAGIC;
        fclose(fp);
        return NULL;
    }

    uint32_t version;
    if (fread(&version, sizeof(uint32_t), 1, fp) != 1 || version != LIMA_PERSIST_VERSION) {
        *out_error = PERSIST_ERR_VERSION_MISMATCH;
        fclose(fp);
        return NULL;
    }

    NaryTreeNode *deserialized_root = deserialize_node(fp, out_error);
    fclose(fp);

    if (!deserialized_root) {
        // error already populated
        return NULL;
    }

    Vfs *vfs = vfs_create();
    if (!vfs) {
        vfs__destroy_subtree(deserialized_root);
        *out_error = PERSIST_ERR_ALLOCATION;
        return NULL;
    }

    if (vfs->tree) {
        ntree_destroy(vfs->tree, vfs__entry_free);
        vfs->tree = ntree_create();
        if (!vfs->tree) {
            vfs__destroy_subtree(deserialized_root);
            free(vfs); // OOM safety fallback
            *out_error = PERSIST_ERR_ALLOCATION;
            return NULL;
        }
    }
    
    ntree_set_root(vfs->tree, deserialized_root);
    vfs->cwd = deserialized_root;

    *out_error = PERSIST_OK;
    return vfs;
}
