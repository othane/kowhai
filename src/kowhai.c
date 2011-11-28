#include "kowhai.h"

#include <string.h>

#ifdef KOWHAI_DBG
#include <stdio.h>
#define KOWHAI_ERR "KOWHAI ERROR:"
#define KOWHAI_INFO "KOWHAI INFO: "
#endif


/**
 * @brief return the size for a given node type
 * @param type a node type to find the size of
 * @return the size in bytes
 */
static int kowhai_get_node_type_size(enum kowhai_node_type type)
{
    switch (type)
    {
        // meta tags only (no real size in the buffer)
        case BRANCH_START:
        case BRANCH_END:
            return 0;

        // normal types to describe a buffer
        case INT8_T:
        case UINT8_T:
            return 1;
        case INT16_T:
        case UINT16_T:
            return 2;
        case INT32_T:
        case UINT32_T:
        case FLOAT_T:
            return 4;
        default:
#ifdef KOWHAI_DBG
            printf(KOWHAI_ERR" unknown item type: %d\n", type);
#endif
            return -1;
    }
}

int kowhai_get_node_size(const struct kowhai_node_t *node, int *node_count, int *size)
{
    int _size = 0;                                // size of the node 
    uint16_t i = 0;                                // items in the node 
    uint16_t _node_count = 0xFFFF;

    // if node_count is non null then limit the search items
    if (node_count != NULL)
        _node_count = *node_count;

    // if this is not a branch then just return the size of the node item (otherwise we need to drill baby)
    if (node->type != BRANCH_START)
    {
        _size = kowhai_get_node_type_size(node[i].type) * node[i].count;
        goto done;
    }
    
    // this is a branch so look through all the elements in this branch and accumulate the sizes
    for (i = 1; i < _node_count; i++)
    {
        switch ((enum kowhai_node_type)node[i].type)
        {
            // navigate the hierarchy info
            case BRANCH_START:
            {
                int new_branch_size = 0;
                int new_branch_items = _node_count - i;         // remaining items
                int ret;
                ret = kowhai_get_node_size(&node[i], &new_branch_items, &new_branch_size);
                if (ret != STATUS_OK)
                    return ret;                                 // propagate this down the stack
                _size += new_branch_size;                       // accumulate the branches size
                i += new_branch_items - 1;                      // skip the already processed branch items
                break;
            }
            case BRANCH_END:
                // accumulate the whole array
                _size *= node[0].count;
                goto done;
            
            // accumulate the size of all the other node_count
            default:
                _size += kowhai_get_node_type_size(node[i].type) * node[i].count;
                break;
        }
    }

    // did we get too many items to check
    if (i >= _node_count)
        return STATUS_INVALID_SYMBOL_PATH;

done:
    if (node_count != NULL)
        *node_count = i + 1;
    if (size != NULL)
        *size = _size;
    return STATUS_OK;
    
}

/** 
 * @brief find a item in the tree given its path
 * @param node to start searching from for the given item
 * @param node_count if not null the number of nodes to search, on return if not null will contain the actual number of nodes offset
 * @param num_symbols number of items in the symbols path (@todo should we just terminate the path instead)
 * @param symbols the path of the item to find
 * @param offset set to number of bytes from the current branch to the item
 * @param target_node if return is successful this is the node that matches the symbol path
 */
static int _kowhai_get_node(const struct kowhai_node_t *node, int *node_count, int num_symbols, const struct kowhai_symbol_t *symbols, uint16_t *offset, struct kowhai_node_t **target_node)
{
    int i = 0;
    uint16_t _offset = 0;
    uint16_t _node_count = 0xFFFF;
    int found_item_size;
    int remaining_nodes;
    int ret, r;
    int items;
    uint16_t branch_offset = 0;
    int branch_node_count = 0;

    if (node_count != NULL)
        // if node_count is non null then limit the search items
        _node_count = *node_count;

    // look through all the items in the node list
    while (i < _node_count)
    {
        int skip_size;
        int skip_nodes;

        switch ((enum kowhai_node_type)node[i].type)
        {
            case BRANCH_END:
                // if we got a branch end then we didn't find it on this path
                return STATUS_INVALID_OFFSET;
            case BRANCH_START:
            default:
                // if the path symbols match and the item array is large enough to contain our index this could be it ...
                if ((symbols->symbol == node[i].symbol) && (node[i].count > symbols->index))
                {
                    if (num_symbols == 1)
                    {
                        // the path fully match in values and length so this is the item we are looking for
                        ret = STATUS_OK;
                        goto done;
                    }
                    
                    if ((enum kowhai_node_type)node[i].type == BRANCH_START)
                    {
                        // this is not the item but it maybe in this branch so drill baby drill
                        branch_offset = 0;                          // offset into this branch for the item
                        branch_node_count = _node_count - i;        // max nodes to search from here
                        ret = _kowhai_get_node(&node[i+1], &branch_node_count, num_symbols - 1, &symbols[1], &branch_offset, NULL);
                        if (ret == STATUS_INVALID_SYMBOL_PATH)
                            // branch ended without finding our item so goto next item
                            break;
                        // found or another error occurred so propagate it out
                        branch_node_count += 1;         // since we passed in the next node here we should add 1 to result (note dont add it to i yet as we want to still point at the branch start to get the count etc, it will be added later, see below)
                        _offset += branch_offset;       // accumulate the bytes offset into the branch where the result was found
                        goto done;
                    }
                }
                break;
        }
        
        // this item is not a match so skip it (find out how many bytes and nodes to skip)
        skip_nodes = _node_count - i; // max nodes left
        ret = kowhai_get_node_size(&node[i], &skip_nodes, &skip_size);
        if (ret != STATUS_OK)
            // propagate the error
            return ret;
        _offset += skip_size;
        i += skip_nodes;
    }

done:

    // check to see if we went off the end of the descriptor list
    if (i >= _node_count)
        return STATUS_INVALID_SYMBOL_PATH;
    
    // we are pointing at the root of this setting but now we need to move to the correct array index
    // all we need is (the size of this node / its count) * index bytes offset

    // Index into the array element
    remaining_nodes = _node_count - i; // max nodes left
    r = kowhai_get_node_size(&node[i], &remaining_nodes, &found_item_size);
    if (r != STATUS_OK)
        return r;
    if (node[i].type == BRANCH_END)
        items = 1;
    else
        items = node[i].count;
    _offset += (found_item_size / items) * symbols->index;

    // update return values
    i += branch_node_count;    // if we came from a branch update the node count now we have the brnach start info (see above)
    if (node_count != NULL)
        *node_count = i;
    if (offset != NULL)
        *offset = _offset;
    if (target_node != NULL)
        *target_node = (struct kowhai_node_t *)&node[i];
    return ret;    // success
}

int kowhai_get_node(const struct kowhai_node_t *node, int *node_count, int num_symbols, const struct kowhai_symbol_t *symbols, uint16_t *offset, struct kowhai_node_t **target_node)
{
    // safety wrapper to stop us walking off the end of the tree by updating node_count
    if (node_count == NULL && node->type == BRANCH_START)
    {
        int ret;
        int _node_count = 0xFFFF;
        ret = kowhai_get_node_size(node, &_node_count, NULL);
        if (ret != STATUS_OK)
            return ret;
        // do search with saftey node_count
        return _kowhai_get_node(node, &_node_count, num_symbols, symbols, offset, target_node);
    }
    
    // do unsafe search
    ///@todo we could stop unsafe searches, but why restrict cool ways to play with the tree
    return _kowhai_get_node(node, node_count, num_symbols, symbols, offset, target_node);
}

int kowhai_read(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, int read_offset, void* result, int read_size)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    int size;

    // find this node
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (read_offset < 0)
        return STATUS_INVALID_OFFSET;

    // check the read wont overrun the item
    status = kowhai_get_node_size(node, NULL, &size);
    if (status != STATUS_OK)
        return status;
    if (read_size + read_offset > size)
        return STATUS_NODE_DATA_TOO_SMALL;

    // do read
    memcpy(result, (char*)tree_data + offset + read_offset, read_size);
    return status;
}

int kowhai_write(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, int write_offset, void* value, int write_size)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    int size;
    
    // find this node
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (write_offset < 0)
        return STATUS_INVALID_OFFSET;
    
    // check the write wont overrun the item
    status = kowhai_get_node_size(node, NULL, &size);
    if (status != STATUS_OK)
        return status;
    if (write_size + write_offset > size)
        return STATUS_NODE_DATA_TOO_SMALL;
    
    // do write
    memcpy((char*)tree_data + offset + write_offset, value, write_size);
    return status;
}

int kowhai_get_char(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, char* result)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (node->type == INT8_T || node->type == UINT8_T)
    {
        *result = *((char*)((char*)tree_data + offset));
        return status;
    }
    return STATUS_INVALID_NODE_TYPE;
}

int kowhai_get_int16(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, int16_t* result)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (node->type == INT16_T || node->type == UINT16_T)
    {
        *result = *((int16_t*)((char*)tree_data + offset));
        return status;
    }
    return STATUS_INVALID_NODE_TYPE;
}

int kowhai_get_int32(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, int32_t* result)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (node->type == INT32_T || node->type == UINT32_T)
    {
        *result = *((uint32_t*)((char*)tree_data + offset));
        return status;
    }
    return STATUS_INVALID_NODE_TYPE;
}

int kowhai_get_float(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, float* result)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (node->type == FLOAT_T)
    {
        *result = *((float*)((char*)tree_data + offset));
        return status;
    }
    return STATUS_INVALID_NODE_TYPE;
}

int kowhai_set_char(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, char value)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (node->type == INT8_T || node->type == UINT8_T)
    {
        char* target_address = (char*)((char*)tree_data + offset);
        *target_address = value;
        return status;
    }
    return STATUS_INVALID_NODE_TYPE;
}

int kowhai_set_int16(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, int16_t value)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (node->type == INT16_T || node->type == UINT16_T)
    {
        int16_t* target_address = (int16_t*)((char*)tree_data + offset);
        *target_address = value;
        return status;
    }
    return STATUS_INVALID_NODE_TYPE;
}

int kowhai_set_int32(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, int32_t value)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (node->type == INT32_T || node->type == UINT32_T)
    {
        uint32_t* target_address = (uint32_t*)((char*)tree_data + offset);
        *target_address = value;
        return status;
    }
    return STATUS_INVALID_NODE_TYPE;
}

int kowhai_set_float(struct kowhai_node_t* tree_descriptor, void* tree_data, int num_symbols, struct kowhai_symbol_t* symbols, float value)
{
    struct kowhai_node_t* node;
    uint16_t offset;
    int status;
    status = kowhai_get_node(tree_descriptor, NULL, num_symbols, symbols, &offset, &node);
    if (status != STATUS_OK)
        return status;
    if (node->type == FLOAT_T)
    {
        float* target_address = (float*)((char*)tree_data + offset);
        *target_address = value;
        return status;
    }
    return STATUS_INVALID_NODE_TYPE;
}

