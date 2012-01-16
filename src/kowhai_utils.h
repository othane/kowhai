#ifndef _KOWHAI_UTILS_H_
#define _KOWHAI_UTILS_H_

#include "kowhai.h" 

/**
 * @brief called when a difference is found between two tree's
 * @param left_node, points to the left node where the difference was found, or NULL if right node is unique
 * @param left_data, point to the start of the difference in the left node, or NULL if right node is unique
 * @param left_offset, the offset into left_data (if not NULL) where the difference starts
 * @param right_node, points to the right node where the difference was found, or NULL if left node is unique
 * @param right_data, point to the start of the difference in the right node, or NULL if left node is unique
 * @param right_offset, the offset into right_data (if not NULL) where the difference starts
 * @param index, the array index of this node where the difference starts
 * @param depth number of parent nodes from the root node that this difference was found
 */
typedef int (*kowhai_on_diff_t)(const struct kowhai_node_t *left_node, void *left_data, int left_offset, const struct kowhai_node_t *right_node, void *right_data, int right_offset, int index, int depth);

/**
 * @brief diff left and right tree
 * If a node is found in the left tree that is not in the right tree (ie symbol path and types/array size match) or visa versa, call on_diff
 * If a node is found in both left and right tree, but the values of the node items do not match call on_diff
 * @param left, diff this tree against right
 * @param right, diff this tree against left
 * @param on_diff, call this when a unique node or common nodes that have different values are found
 */
int kowhai_diff(struct kowhai_tree_t *left, struct kowhai_tree_t *right, kowhai_on_diff_t on_diff);

/**
 * @brief merge nodes that are common to src and dst from src into dst leaving unique nodes unchanged
 * @brief dst, destination tree (this is updated from the source tree)
 * @brief src, source tree
 * @return KOW_STATUS_OK on success otherwise other KOW_STATUS code
 */
int kowhai_merge(struct kowhai_tree_t *dst, struct kowhai_tree_t *src);

#endif

