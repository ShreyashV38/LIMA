#include "../../include/data_structures/nary_tree.h"
#include <stdlib.h>

struct NaryTreeNode {
    void *data;
    NaryTreeNode *first_child;
    NaryTreeNode *next_sibling;
    NaryTreeNode *parent;
};

struct NaryTree {
    NaryTreeNode *root;
};

NaryTree *ntree_create(void) {
    NaryTree *tree = malloc(sizeof(NaryTree));
    if (tree) {
        tree->root = NULL;
    }
    return tree;
}

static void ntree_destroy_node_recursive(NaryTreeNode *node, NaryTreeDataDeallocator deallocator) {
    if (!node) return;
    
    ntree_destroy_node_recursive(node->first_child, deallocator);
    ntree_destroy_node_recursive(node->next_sibling, deallocator);
    
    if (deallocator && node->data) {
        deallocator(node->data);
    }
    free(node);
}

void ntree_destroy(NaryTree *tree, NaryTreeDataDeallocator deallocator) {
    if (!tree) return;
    ntree_destroy_node_recursive(tree->root, deallocator);
    free(tree);
}

NaryTreeNode *ntree_node_create(void *data) {
    NaryTreeNode *node = malloc(sizeof(NaryTreeNode));
    if (!node) return NULL;
    
    node->data = data;
    node->first_child = NULL;
    node->next_sibling = NULL;
    node->parent = NULL;
    return node;
}

void ntree_set_root(NaryTree *tree, NaryTreeNode *root) {
    if (tree) {
        tree->root = root;
    }
}

NaryTreeNode *ntree_get_root(const NaryTree *tree) {
    return tree ? tree->root : NULL;
}

void ntree_append_child(NaryTreeNode *parent, NaryTreeNode *child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    
    if (!parent->first_child) {
        parent->first_child = child;
        child->next_sibling = NULL;
    } else {
        NaryTreeNode *current = parent->first_child;
        while (current->next_sibling) {
            current = current->next_sibling;
        }
        current->next_sibling = child;
        child->next_sibling = NULL;
    }
}

NaryTreeNode *ntree_remove_child(NaryTreeNode *parent, NaryTreeNode *child) {
    if (!parent || !child || !parent->first_child) return NULL;
    
    if (parent->first_child == child) {
        parent->first_child = child->next_sibling;
        child->next_sibling = NULL;
        child->parent = NULL;
        return child;
    }
    
    NaryTreeNode *current = parent->first_child;
    while (current->next_sibling) {
        if (current->next_sibling == child) {
            current->next_sibling = child->next_sibling;
            child->next_sibling = NULL;
            child->parent = NULL;
            return child;
        }
        current = current->next_sibling;
    }
    
    return NULL;
}

void *ntree_node_get_data(const NaryTreeNode *node) {
    return node ? node->data : NULL;
}

void ntree_node_set_data(NaryTreeNode *node, void *data) {
    if (node) {
        node->data = data;
    }
}

NaryTreeNode *ntree_node_get_first_child(const NaryTreeNode *node) {
    return node ? node->first_child : NULL;
}

NaryTreeNode *ntree_node_get_next_sibling(const NaryTreeNode *node) {
    return node ? node->next_sibling : NULL;
}

NaryTreeNode *ntree_node_get_parent(const NaryTreeNode *node) {
    return node ? node->parent : NULL;
}
