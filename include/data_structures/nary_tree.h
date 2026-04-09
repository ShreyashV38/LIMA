#ifndef NARY_TREE_H
#define NARY_TREE_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief N-ary Tree ADT, generic tree where each node can have multiple children.
 * Useful for hierarchal structures like Virtual File Systems.
 */

typedef struct NaryTreeNode NaryTreeNode;
typedef struct NaryTree NaryTree;

/**
 * @brief Callback function used when destroying or traversing the tree to handle the node data.
 */
typedef void (*NaryTreeDataDeallocator)(void *data);

/**
 * @brief Create a new N-ary tree.
 * @return Pointer to the new tree, or NULL on failure.
 */
NaryTree *ntree_create(void);

/**
 * @brief Destroy the tree and all its nodes.
 * @param tree Pointer to the tree.
 * @param deallocator Optional callback to free the internal data of each node.
 */
void ntree_destroy(NaryTree *tree, NaryTreeDataDeallocator deallocator);

/**
 * @brief Create a new node.
 * @param data Data to associate with this node.
 * @return Pointer to the new node, or NULL on failure.
 */
NaryTreeNode *ntree_node_create(void *data);

/**
 * @brief Set the root of the tree.
 * @param tree Pointer to the tree.
 * @param root The node to set as root.
 */
void ntree_set_root(NaryTree *tree, NaryTreeNode *root);

/**
 * @brief Get the root node of the tree.
 * @param tree Pointer to the tree.
 * @return The root node, or NULL if empty.
 */
NaryTreeNode *ntree_get_root(const NaryTree *tree);

/**
 * @brief Append a child to a given parent node.
 * @param parent The parent node.
 * @param child The child node to append.
 */
void ntree_append_child(NaryTreeNode *parent, NaryTreeNode *child);

/**
 * @brief Remove a child from a parent node.
 * @param parent The parent node.
 * @param child The child node to remove. (Node is returned but not destroyed)
 * @return Pointer to the removed child node, or NULL if not found.
 */
NaryTreeNode *ntree_remove_child(NaryTreeNode *parent, NaryTreeNode *child);

/**
 * @brief Get the node's data.
 * @param node Pointer to the node.
 * @return The node's data.
 */
void *ntree_node_get_data(const NaryTreeNode *node);

/**
 * @brief Set the node's data.
 * @param node Pointer to the node.
 * @param data The new data for the node.
 */
void ntree_node_set_data(NaryTreeNode *node, void *data);

/**
 * @brief Get the first child of the node.
 * @param node Pointer to the node.
 * @return Pointer to the first child, or NULL.
 */
NaryTreeNode *ntree_node_get_first_child(const NaryTreeNode *node);

/**
 * @brief Get the next sibling of the node.
 * @param node Pointer to the node.
 * @return Pointer to the next sibling, or NULL.
 */
NaryTreeNode *ntree_node_get_next_sibling(const NaryTreeNode *node);

/**
 * @brief Get the parent of the node.
 * @param node Pointer to the node.
 * @return Pointer to the parent node, or NULL.
 */
NaryTreeNode *ntree_node_get_parent(const NaryTreeNode *node);

#endif // NARY_TREE_H
