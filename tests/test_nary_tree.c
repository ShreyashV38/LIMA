#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/data_structures/nary_tree.h"

void test_ntree_basic() {
    NaryTree *tree = ntree_create();
    assert(tree != NULL);
    assert(ntree_get_root(tree) == NULL);

    int root_val = 1;
    NaryTreeNode *root = ntree_node_create(&root_val);
    ntree_set_root(tree, root);
    assert(ntree_get_root(tree) == root);

    int child1_val = 11;
    NaryTreeNode *child1 = ntree_node_create(&child1_val);
    ntree_append_child(root, child1);
    assert(ntree_node_get_first_child(root) == child1);
    assert(ntree_node_get_parent(child1) == root);

    int child2_val = 12;
    NaryTreeNode *child2 = ntree_node_create(&child2_val);
    ntree_append_child(root, child2);
    assert(ntree_node_get_next_sibling(child1) == child2);

    // Removal
    assert(ntree_remove_child(root, child1) == child1);
    assert(ntree_node_get_first_child(root) == child2);
    assert(ntree_node_get_parent(child1) == NULL);
    
    // Clean up free node
    free(child1);

    ntree_destroy(tree, NULL);
    printf("test_ntree_basic passed!\n");
}

int main() {
    printf("Running n-ary tree tests...\n");
    test_ntree_basic();
    printf("All n-ary tree tests passed!\n");
    return 0;
}
