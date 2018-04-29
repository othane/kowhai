#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "kowhai_serialize.h"
#include "../3rdparty/jsmn/jsmn.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>

#define NAME "name"
#define TYPE "type"
#define SYMBOL "symbol"
#define COUNT "count"
#define TAG "tag"
#define VALUE "value"
#define ARRAY "array"
#define CHILDREN "children"
#define PATH "path"

#define MIN(a,b) ((a<b)?a:b)

union any_type_t
{
    char c;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    uint8_t ui8;
    uint16_t ui16;
    uint32_t ui32;
    float f;
};

int write_string(char* buffer, size_t buffer_size, const char* format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    // we want to abort in the case of a truncated result
    if (result > (long long)buffer_size)
        return -1000;
    return result;
}

int add_header(char** dest, size_t* dest_size, int* current_offset, struct kowhai_node_t* node, void* get_name_param, kowhai_get_symbol_name_t get_name)
{
    int chars = write_string(*dest, *dest_size,
            "{\""NAME"\": \"%s\", \""TYPE"\": %d, \""SYMBOL"\": %d, \""COUNT"\": %d, \""TAG"\": %d",
            get_name(get_name_param, node->symbol), node->type, node->symbol, node->count, node->tag);
    if (chars >= 0)
    {
        *dest += chars;
        *dest_size -= chars;
        *current_offset += chars;
    }
    return chars;
}

int add_string(char** dest, size_t* dest_size, int* current_offset, char* string)
{
    int chars = write_string(*dest, *dest_size, string);
    if (chars >= 0)
    {
        *dest += chars;
        *dest_size -= chars;
        *current_offset += chars;
    }
    return chars;
}

int add_indent(char** dest, size_t* dest_size, int* current_offset, int depth)
{
    int i, chars = 0;
    for (i = 0; i < depth; i++)
    {
        int c = add_string(dest, dest_size, current_offset, "\t");
        if (c < 0)
            return c;
        chars += c;
    }
    return chars;
}

int add_value(char** dest, size_t* dest_size, int* current_offset, uint16_t node_type, void* data)
{
    int chars;
    union any_type_t val;

    // on some systems vsnprintf requires the src buffer to be aligned
    // properly, since data is possibly packed to make the tree tidier
    // the memcpy below gets our data into an aligned var
    memcpy(&val, data, kowhai_get_node_type_size(node_type));

    switch (node_type)
    {
        case KOW_CHAR:
            if (val.c < 32 || val.c >= 127)
                chars = write_string(*dest, *dest_size, "%d", val.c);
            else
                chars = write_string(*dest, *dest_size, "%c", val.c);
            break;
        case KOW_INT8:
            chars = write_string(*dest, *dest_size, "%d", val.i8);
            break;
        case KOW_INT16:
            chars = write_string(*dest, *dest_size, "%d", val.i16);
            break;
        case KOW_INT32:
            chars = write_string(*dest, *dest_size, "%d", val.i32);
            break;
        case KOW_UINT8:
            chars = write_string(*dest, *dest_size, "%d", val.ui8);
            break;
        case KOW_UINT16:
            chars = write_string(*dest, *dest_size, "%d", val.ui16);
            break;
        case KOW_UINT32:
            chars = write_string(*dest, *dest_size, "%d", val.ui32);
            break;
        case KOW_FLOAT:
            chars = write_string(*dest, *dest_size, "%.17g", val.f);
            break;
        default:
            return -1;
    }
    if (chars >= 0)
    {
        *dest += chars;
        *dest_size -= chars;
        *current_offset += chars;
    }
    return chars;
}

static int str_printable(char *str, int len)
{
    while (len--)
    {
        if (str[len] == 0)
            continue; // we allow/ignore nulls in strings
        if (str[len] < 32 || str[len] >= 127)
            return 0;
    }

    return 1;
}

int serialize_tree(struct kowhai_node_t** desc, void** data, char* target_buffer, size_t target_size, int level, void* get_name_param, kowhai_get_symbol_name_t get_name, int in_union, int* largest_data_field)
{
    int target_offset = 0;
    struct kowhai_node_t* node;
    int i, chars;
    char* node_end_str;

    while (1)
    {
        node = *desc;

        if (node->type == KOW_BRANCH_END)
            return target_offset;

        // indent to current level using tabs
        chars = add_indent(&target_buffer, &target_size, &target_offset, level);
        if (chars < 0)
            return chars;

        //
        // write node
        //

        switch (node->type)
        {
            case KOW_BRANCH_START:
            case KOW_BRANCH_U_START:
            {
                int node_is_union = node->type == KOW_BRANCH_U_START;
                int largest_child_data_field = 0;
                void* initial_data = *data;
                // write header
                chars = add_header(&target_buffer, &target_size, &target_offset, node, get_name_param, get_name);
                if (chars < 0)
                    return chars;
                if (node->count > 1)
                {
                    struct kowhai_node_t* initial_node = *desc;
                    // write array identifier
                    chars = add_string(&target_buffer, &target_size, &target_offset, ", \""ARRAY"\": [\n");
                    if (chars < 0)
                    return chars;
                    for (i = 0; i < node->count; i++)
                    {
                        // set descriptor to initial node at the branch array
                        *desc = initial_node;
                        (*desc) += 1;
                        // indent to current level using tab
                        chars = add_indent(&target_buffer, &target_size, &target_offset, level + 1);
                        if (chars < 0)
                            return chars;
                        // write branch children start
                        chars = add_string(&target_buffer, &target_size, &target_offset, "[\n");
                        if (chars < 0)
                            return chars;
                        // write branch children
                        chars = serialize_tree(desc, data, target_buffer, target_size, level + 1, get_name_param, get_name, node_is_union, &largest_child_data_field);
                        if (chars < 0)
                            return chars;
                        target_offset += chars;
                        target_buffer += chars;
                        target_size -= chars;
                        // increment data pointer if node is a union
                        if (node_is_union)
                        {
                            *data = (char*)initial_data + largest_child_data_field;
                            initial_data = *data;
                        }
                        // indent to current level using tab
                        chars = add_indent(&target_buffer, &target_size, &target_offset, level + 1);
                        if (chars < 0)
                            return chars;
                        // write branch children end
                        chars = add_string(&target_buffer, &target_size, &target_offset, "]");
                        if (chars < 0)
                            return chars;
                        if (i < node->count - 1)
                        {
                            chars = add_string(&target_buffer, &target_size, &target_offset, ",\n");
                            if (chars < 0)
                                return chars;
                        }
                        else
                        {
                            chars = add_string(&target_buffer, &target_size, &target_offset, "\n");
                            if (chars < 0)
                                return chars;
                        }
                    }
                }
                else
                {
                    // write children identifier
                    chars = add_string(&target_buffer, &target_size, &target_offset, ", \""CHILDREN"\": [\n");
                    if (chars < 0)
                        return chars;
                    // write branch children
                    (*desc) += 1;
                    chars = serialize_tree(desc, data, target_buffer, target_size, level + 1, get_name_param, get_name, node_is_union, &largest_child_data_field);
                    if (chars < 0)
                        return chars;
                    target_offset += chars;
                    target_buffer += chars;
                    target_size -= chars;
                    // increment data pointer if node is a union
                    if (node_is_union)
                        *data = (char*)initial_data + largest_child_data_field;
                }
                // indent to current level using tab
                chars = add_indent(&target_buffer, &target_size, &target_offset, level);
                if (chars < 0)
                    return chars;
                // write node end
                if (level == 0 || (*desc)[1].type == KOW_BRANCH_END)
                    node_end_str = "]}\n";
                else
                    node_end_str = "]},\n";
                chars = add_string(&target_buffer, &target_size, &target_offset, node_end_str);
                if (chars < 0)
                    return chars;
                break;
            }
            default:
            {
                int value_size = kowhai_get_node_type_size(node->type);
                // write header
                chars = add_header(&target_buffer, &target_size, &target_offset, node, get_name_param, get_name);
                if (chars < 0)
                    return chars;
                // write value identifier
                chars = add_string(&target_buffer, &target_size, &target_offset, ", \""VALUE"\": ");
                if (chars < 0)
                    return chars;
                // write value/s
                if (node->type == KOW_CHAR && node->count > 1 && str_printable((char *)*data, node->count))
                {
                    // special string case
                    chars = write_string(target_buffer, target_size, "\"%.*s\"", node->count, (char*)*data);
                    if (chars >= 0)
                    {
                        target_buffer += chars;
                        target_size -= chars;
                        target_offset += chars;
                    }
                    else
                        return chars;
                    // increment data pointer
                    if (!in_union)
                        *data = (char*)*data + value_size * node->count;
                    else if (value_size * node->count > *largest_data_field)
                        *largest_data_field = value_size * node->count;
                }
                else if (node->count > 1)
                {
                    // write start bracket
                    chars = add_string(&target_buffer, &target_size, &target_offset, "[");
                    if (chars < 0)
                        return chars;
                    for (i = 0; i < node->count; i++)
                    {
                        // write leaf node array item value
                        chars = add_value(&target_buffer, &target_size, &target_offset, node->type, (char*)*data + i * value_size);
                        if (chars < 0)
                            return chars;
                        // write comma if there is another array item
                        if (i < node->count - 1)
                        {
                            chars = add_string(&target_buffer, &target_size, &target_offset, ", ");
                            if (chars < 0)
                                return chars;
                        }
                    }
                    // increment data pointer
                    if (!in_union)
                        *data = (char*)*data + value_size * node->count;
                    else if (value_size * node->count > *largest_data_field)
                        *largest_data_field = value_size * node->count;
                    // write end bracket
                    chars = add_string(&target_buffer, &target_size, &target_offset, "]");
                    if (chars < 0)
                        return chars;
                }
                else
                {
                    // write leaf node value
                    chars = add_value(&target_buffer, &target_size, &target_offset, node->type, *data);
                    if (chars < 0)
                        return chars;
                    // increment data pointer
                    if (!in_union)
                        *data = (char*)*data + value_size;
                    else if (value_size > *largest_data_field)
                        *largest_data_field = value_size;
                }
                // write node end
                if (level == 0 || node[1].type == KOW_BRANCH_END)
                    node_end_str = " }\n";
                else
                    node_end_str = " },\n";
                chars = add_string(&target_buffer, &target_size, &target_offset, node_end_str);
                if (chars < 0)
                    return chars;
                break;
            }
        }

        if (level == 0)
            return target_offset;

        (*desc) += 1;
    }
}

int kowhai_serialize_tree(struct kowhai_tree_t tree, char* target_buffer, int* target_size, void* get_name_param, kowhai_get_symbol_name_t get_name)
{
    int largest_data_field;
    int chars = serialize_tree(&tree.desc, &tree.data, target_buffer, *target_size, 0, get_name_param, get_name, tree.desc->type == KOW_BRANCH_U_START, &largest_data_field);
    if (chars < 0)
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
    *target_size = chars;
    return KOW_STATUS_OK;
}

static int path_to_str(union kowhai_symbol_t *path, int path_len, char *dst, int dst_len, const char *separator, int hide_last_index, struct kowhai_node_t *root, void* get_name_param, kowhai_get_symbol_name_t get_name)
{
    int i, r, e;
    int count = 0;

    for (i = 0; i < path_len; i++)
    {
        struct kowhai_node_t *tnode;
    
        // if a root is given we can use this to hide ugly [0] on each node since we can look-up the
        // node count for each path item using kowhai_get_node(), then we only add the index if it
        // is not 0 
        if (root)
            if ((e = kowhai_get_node(root, i+1, path, 0, &tnode)) != KOW_STATUS_OK)
                return -1;
        
        // display the index at the end of this node if we can know the count of this node (needs root so we can find this node from
        // the path), and if the count > 1 ie is an array, and if this is the last item and hide_last_index == true
        if (root == NULL || tnode->count == 1 || ((i == (path_len - 1)) && hide_last_index))
            r = snprintf(dst, dst_len, "%s", get_name(get_name_param, path[i].parts.name));
        else
            r = snprintf(dst, dst_len, "%s[%d]", get_name(get_name_param, path[i].parts.name), path[i].parts.array_index);
        if (r < 0)
            return r; // real error
        if (r > dst_len)
            return count + r; // dst not long enough
        dst += r;
        count += r;
        dst_len -= r;
        
        // put separator unless this is the last item
        if (i != path_len - 1)
        {
            r = snprintf(dst, dst_len, "%s", separator);
            if (r < 0)
                return r; // real error
            if (r > dst_len)
                return count + r; // dst not long enough
            dst += r;
            count += r;
            dst_len -= r;
        }
    }

    return count;
}

static const char *strnchr(const char *str, size_t len, char c)
{
    int l;
    for (l = 0; l < len; l++)
    {
        if (str[l] == c) // found
            return &str[l];
        if (str[l] == 0) // null terminated
            return NULL;
    }
    return NULL;
}

int kowhai_str_to_path(const char *path_str, int path_strlen, union kowhai_symbol_t *path, int *path_len, void *get_name_param, kowhai_get_symbol_t get_name)
{
    int ipath = 0;
    const char *init = path_str;
    const char *sym_start = init, *sym_end;
    const char *istart = init, *iend;
    int r, index;

    while (sym_start < init + path_strlen)
    {
        // find next symbol
        int n = path_strlen - (sym_start - init);
        sym_end = strnchr(sym_start, n, '.');
        if (sym_end == NULL)
            sym_end = init + path_strlen; // so the rest of the string

        // check for index
        index = 0;
        istart = strnchr(sym_start, sym_end - sym_start, '[');
        iend = strnchr(sym_start, sym_end - sym_start, ']');
        if (istart != NULL)
        {
            if (iend == NULL)
                return KOW_STATUS_INVALID_SYMBOL_PATH; // malformed path, array index start must have a end
            index = atoi(istart + 1);
        }
        else
            istart = sym_end; // if no indexing this starts (and ends) at the last char

        r = get_name(get_name_param, sym_start, istart - sym_start);
        if (r < 0)
            return KOW_STATUS_NOT_FOUND;
        path[ipath++].symbol = KOWHAI_SYMBOL(r, index);
        if (ipath >= *path_len)
            return KOW_STATUS_PATH_TOO_SMALL; // path buffer not big enough

        sym_start = sym_end + 1;
    }

    // return number of symbols in the path
    *path_len = ipath;
    return KOW_STATUS_OK;
}

// internal helper function to avoid return type conflicts with the kowhai method
static int str_to_path(jsmn_parser* parser, jsmntok_t* tok, union kowhai_symbol_t *path, int path_len, void *get_name_param, kowhai_get_symbol_t get_name)
{
    const char *path_str = &parser->js[tok->start];
    int path_strlen = tok->end - tok->start;
    int e;

    e = kowhai_str_to_path(path_str, path_strlen, path, &path_len, get_name_param, get_name);
    switch (e)
    {
        case KOW_STATUS_INVALID_SYMBOL_PATH:
            return -2;
        case KOW_STATUS_NOT_FOUND:
            return -4;
        case KOW_STATUS_PATH_TOO_SMALL:
            return -3;
        case KOW_STATUS_OK:
            return path_len;
        default:
            return KOW_STATUS_UNKNOWN_ERROR;
    }
}

static int val_to_str(struct kowhai_node_t *node, void *data, char *dst, int dst_len)
{
    int r, i, count = 0;
    int node_type_size = kowhai_get_node_type_size(node->type);
    union any_type_t val;

    // handle special string case
    if (node->type == KOW_CHAR)
    {
        r = snprintf(dst, dst_len, "%.*s", node->count, (char *)data);

        if (r < 0)
            return r; // real error
        if (r > dst_len)
            return count + r; // dst not long enough
        dst += r;
        count += r;
        dst_len -= r;

        return count;
    }

    // all other cases
    for (i = 0; i < node->count; i++)
    {
        // some systems require that the src buffer to be aligned
        // properly, since data is possibly packed to make the tree 
        // tidier the memcpy below gets our data into an aligned var
        memcpy(&val, data, node_type_size);
        data = (char *)data + node_type_size;

        switch (node->type)
        {
            case KOW_INT8:
                r = snprintf(dst, dst_len, "%"PRIi8, val.i8);
                break;
            case KOW_INT16:
                r = snprintf(dst, dst_len, "%"PRIi16, val.i16);
                break;
            case KOW_INT32:
                r = snprintf(dst, dst_len, "%"PRIi32, val.i32);
                break;
            case KOW_UINT8:
                r = snprintf(dst, dst_len, "%"PRIu8, val.ui8);
                break;
            case KOW_UINT16:
                r = snprintf(dst, dst_len, "%"PRIu16, val.ui16);
                break;
            case KOW_UINT32:
                r = snprintf(dst, dst_len, "%"PRIu32, val.ui32);
                break;
            case KOW_FLOAT:
                r = snprintf(dst, dst_len, "%.17g", val.f);
                break;
            default:
                return -1;
        }
        if (r < 0)
            return r; // real error
        if (r > dst_len)
            return count + r; // dst not long enough
        dst += r;
        count += r;
        dst_len -= r;

        // put the next separator (ie ,) if this not the last item
        if (i != node->count - 1)
        {
            r = snprintf(dst, dst_len, ", ");
            if (r < 0)
                return r; // real error
            if (r > dst_len)
                return count + r; // dst not long enough
            dst += r;
            count += r;
            dst_len -= r;
        }
    }
    return count;
}

static int print_node_type(struct kowhai_node_t *root, char *dst, int dst_len, struct kowhai_node_t *node, void **src_data, 
        union kowhai_symbol_t *path, int ipath, void* get_name_param, kowhai_get_symbol_name_t get_name)
{
    int r, count = 0;

    // print the path
    r = snprintf(dst, dst_len, "\t{\""PATH"\": \"");
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    r = path_to_str(path, ipath + 1, dst, dst_len, ".", 1, root, get_name_param, get_name);
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    r = snprintf(dst, dst_len, "\", ");
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    // print type
    r = snprintf(dst, dst_len, "\""TYPE"\": %d, ", node->type);
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    // print count
    r = snprintf(dst, dst_len, "\""COUNT"\": %d, ", node->count);
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    // print tag
    r = snprintf(dst, dst_len, "\""TAG"\": %d, ", node->tag);
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    // print the value(s)
    if (node->count == 1)
        r = snprintf(dst, dst_len, "\""VALUE"\": ");
    else if (node->type == KOW_CHAR)
        r = snprintf(dst, dst_len, "\""VALUE"\": \"");
    else
        r = snprintf(dst, dst_len, "\""VALUE"\": [");
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    r = val_to_str(node, *src_data, dst, dst_len);
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    if (node->count == 1)
        r = snprintf(dst, dst_len, "},\n");
    else if (node->type == KOW_CHAR)
        r = snprintf(dst, dst_len, "\"},\n");
    else
        r = snprintf(dst, dst_len, "]},\n");
    if (r < 0)
        return -1;
    if (r > dst_len)
        return count + r;
    dst += r;
    count += r;
    dst_len -= r;

    return count;
}

#define STARTEND(x) \
{ \
    r = snprintf(dst, dst_len, x"\n"); \
    if (r < 0) \
        return -1; \
    if (r > dst_len) \
        return count + r; \
    dst += r; \
    count += r; \
    dst_len -= r; \
}


static int serialize_nodes(struct kowhai_node_t *root, char *dst, int dst_len, struct kowhai_node_t *src_node, void **src_data, 
                    union kowhai_symbol_t *path, int ipath, int path_len, void* get_name_param, kowhai_get_symbol_name_t get_name,
                    int is_union, int level)
{
    int r, i, n = 0;
    int count = 0;
    int node_size;
    int node_count;
    int max_union_size = 0;
    int rmax = 0;

    // print opening array brace only if we are level 0
    if (level == 0)
        STARTEND("[");

    while (1)
    {
        struct kowhai_node_t *node = &src_node[n];
        
        switch(node->type)
        {
            case KOW_BRANCH_U_START:
                if (kowhai_get_node_size(node, &node_size) != KOW_STATUS_OK)
                    return -1;

                // for each complex array type recurse into each array element
                for (i = 0; i < node->count; i++)
                {
                    // append this branch to the path
                    if (ipath >= path_len)
                        return -2;
                    path[ipath].symbol = KOWHAI_SYMBOL(node->symbol, i);
                
                    // recurse into branch
                    r = serialize_nodes(root, dst, dst_len, node + 1, src_data, path, ipath + 1, path_len, get_name_param, get_name, 1, level + 1);
                    if (r < 0)
                        return r;
                    if (r >= dst_len)
                        return count + r;
                }
                
                // union type over
                dst += r;
                count += r;
                dst_len -= r;
                *(char **)src_data += node_size;

                // move past this branch in the descriptor
                if (kowhai_get_node_count(node, &node_count) != KOW_STATUS_OK)
                    return -1;
                node_count -= 1;
                n += node_count;

                // ending criteria
                if (level == 0)
                {
                    STARTEND("]");
                    return count;
                }
                break;

            case KOW_BRANCH_START:
                if (kowhai_get_node_size(node, &node_size) != KOW_STATUS_OK)
                    return -1;

                // for each complex array type recurse into each array element
                if ((is_union && (node_size >= max_union_size)) || !is_union)
                {
                    for (i = 0; i < node->count; i++)
                    {
                        // append this branch to the path
                        if (ipath >= path_len)
                            return -2;
                        path[ipath].symbol = KOWHAI_SYMBOL(node->symbol, i);
                    
                        // recurse into branch
                        r = serialize_nodes(root, dst, dst_len, node + 1, src_data, path, ipath + 1, path_len, get_name_param, get_name, 0, level + 1);
                        if (r < 0)
                            return r;
                        if (r >= dst_len)
                            return count + r;
                        rmax = r;

                        // push pointer on if this is not a union
                        // if it is we do this at the end for the
                        // biggest field
                        if (!is_union)
                        {
                            dst += r;
                            count += r;
                            dst_len -= r;
                        }
                    }
                }
                
                // move past this branch in the descriptor
                if (kowhai_get_node_count(node, &node_count) != KOW_STATUS_OK)
                    return -1;
                node_count -= 1;
                n += node_count;

                // ending criteria
                if (level == 0)
                {
                    STARTEND("]");
                    return count;
                }
                break;

            case KOW_BRANCH_END:
                if (is_union)
                {
                    dst += rmax;
                    count += rmax;
                    dst_len -= rmax;
                }
                return count;

            default:

                // get this node size in bytes
                node_size = kowhai_get_node_type_size(node->type) * node->count;
                if (node_size < 0)
                    return -1;

                // handle unions
                if (is_union)
                {
                    if (node_size <= max_union_size)
                        break;
                    max_union_size = node_size;
                }

                // update the path scratch buffer for this item
                if (ipath >= path_len)
                    return -2;
                path[ipath].symbol = KOWHAI_SYMBOL(node->symbol, 0);

                // print this item
                r = print_node_type(root, dst, dst_len, node, src_data, path, ipath, get_name_param, get_name);
                if (r < 0)
                    return r;
                if (r > dst_len)
                    return count + r;
                rmax = r;
                if (!is_union)
                {
                    dst += r;
                    count += r;
                    dst_len -= r;

                    // move the data pointer
                    *(char **)src_data += node_size;
                }

                break;
        }
        n++;
    }
}

int kowhai_serialize_nodes(char *dst, int *dst_len, struct kowhai_tree_t *src_tree, union kowhai_symbol_t *path, int path_len, void* get_name_param, kowhai_get_symbol_name_t get_name)
{
    void *src_data = src_tree->data;
    int is_union = src_tree->desc->type == KOW_BRANCH_U_START;
    int chars = serialize_nodes(src_tree->desc, dst, *dst_len, src_tree->desc, &src_data, path, 0, path_len, get_name_param, get_name, is_union, 0);

    // handle errors
    switch (chars)
    {
        case -1:
            return KOW_STATUS_UNKNOWN_ERROR;
        case -2:
            return KOW_STATUS_PATH_TOO_SMALL;
        default:
            break;
    }
    
    if (chars >= *dst_len)
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;

    *dst_len = chars;
    return KOW_STATUS_OK;
}

static int copy_string_from_token(const char* js, jsmntok_t* tok, char* dest, int dest_size)
{
    int token_string_size = tok->end - tok->start;
    if (token_string_size < dest_size)
    {
        strncpy(dest, js + tok->start, token_string_size);
        dest[token_string_size] = 0;
        return 1;
    }
    return 0;
}

#define TEMP_SIZE 100

static int get_token_uint8(jsmn_parser* parser, jsmntok_t* tok, uint8_t* value)
{
    char temp[TEMP_SIZE];
    if (copy_string_from_token(parser->js, tok, temp, TEMP_SIZE))
    {
        *value = atoi(temp);
        return KOW_STATUS_OK;
    }
    else
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
}

static int get_token_uint16(jsmn_parser* parser, jsmntok_t* tok, uint16_t* value)
{
    char temp[TEMP_SIZE];
    if (copy_string_from_token(parser->js, tok, temp, TEMP_SIZE))
    {
        *value = atoi(temp);
        return KOW_STATUS_OK;
    }
    else
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
}

static int get_token_uint32(jsmn_parser* parser, jsmntok_t* tok, uint32_t* value)
{
    char temp[TEMP_SIZE];
    if (copy_string_from_token(parser->js, tok, temp, TEMP_SIZE))
    {
        *value = atol(temp);
        return KOW_STATUS_OK;
    }
    else
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
}

static int get_token_float(jsmn_parser* parser, jsmntok_t* tok, float* value)
{
    char temp[TEMP_SIZE];
    if (copy_string_from_token(parser->js, tok, temp, TEMP_SIZE))
    {
        *value = (float)atof(temp);
        return KOW_STATUS_OK;
    }
    else
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
}

static int token_string_match(jsmn_parser* parser, jsmntok_t* tok, char* str)
{
    return tok->end - tok->start == strlen(str) &&
        strncmp(parser->js + tok->start, str, strlen(str)) == 0;
}

static int process_tree_token(jsmn_parser* parser, int token_index, struct kowhai_node_t* desc, int desc_size, int* desc_nodes_populated, void* data, int data_size, int* data_offset)
{
#define INC { i++; token_index++; continue; }
    int initial_token_index = token_index;
    struct kowhai_node_t* initial_desc = desc;
    int i;
    int res;
    *desc_nodes_populated = 0;
    *data_offset = 0;

    if (desc_size < 1)
        return -2;

    if (parser->tokens[token_index].type == JSMN_OBJECT)
    {
        int token_is_branch = 0;
        jsmntok_t* parent_tok = &parser->tokens[token_index];

        token_index = initial_token_index;
        desc = initial_desc;

        for (i = 0; i < parent_tok->size; i++)
        {
            jsmntok_t* tok;
            token_index++;
            tok = &parser->tokens[token_index];
            if (tok->type == JSMN_STRING)
            {
                if (token_string_match(parser, tok, TYPE))
                {
                    res = get_token_uint16(parser, tok + 1, &desc->type);
                    if (res != KOW_STATUS_OK)
                        return -1;
                    INC;
                }
                else if (token_string_match(parser, tok, SYMBOL))
                {
                    res = get_token_uint16(parser, tok + 1, &desc->symbol);
                    if (res != KOW_STATUS_OK)
                        return -1;
                    INC;
                }
                else if (token_string_match(parser, tok, COUNT))
                {
                    res = get_token_uint16(parser, tok + 1, &desc->count);
                    if (res != KOW_STATUS_OK)
                        return -1;
                    INC;
                }
                else if (token_string_match(parser, tok, TAG))
                {
                    res = get_token_uint16(parser, tok + 1, &desc->tag);
                    if (res != KOW_STATUS_OK)
                        return -1;
                    INC;
                }
                else if (token_string_match(parser, tok, VALUE))
                {
                    int k;
                    int size = kowhai_get_node_type_size(desc->type);
                    i++;

                    // skip the [ or " wrappers
                    if (desc->count > 1)
                    {
                        token_index++;
                        tok++;

                        // special string case
                        if (desc->type == KOW_CHAR && desc->count > 1 && parser->js[tok->start - 1] == '"')
                        {
                            memset(data, 0, desc->count);
                            strncpy(data, parser->js + tok->start, MIN(tok->end - tok->start, desc->count));
                            data = (char*)data + desc->count * size;
                            data_size -= desc->count * desc->count * size;
                            *data_offset += desc->count * size;
                            continue;
                        }
                    }

                    for (k = 0; k < desc->count; k++)
                    {
                        // on some systems casting unaligned memory (ie the data pointer) to 
                        // types like uint32's, float's etc results in nonsense values because
                        // the memory is assumed to be aligned ... hence to solve this we 
                        // memcpy from an aligned buffer (value) to an unaligned buffer (data so
                        // it works as raw byte moves, see below)
                        union any_type_t val;
                        if (size < 0)
                            return -1;
                        token_index++;
                        tok++;
                        switch (desc->type)
                        {
                            case KOW_UINT8:
                            case KOW_INT8:
                            case KOW_CHAR:
                            {
                                //TODO: could probably call get_token_uint32 instead and remove get_token_uint16 (needs testing)
                                res = get_token_uint8(parser, tok, &val.ui8);
                                break;
                            }
                            case KOW_UINT16:
                            case KOW_INT16:
                            {
                                //TODO: could probably call get_token_uint32 instead and remove get_token_uint16 (needs testing)
                                res = get_token_uint16(parser, tok, &val.ui16);
                                break;
                            }
                            case KOW_UINT32:
                            case KOW_INT32:
                            {
                                res = get_token_uint32(parser, tok, &val.ui32);
                                break;
                            }
                            case KOW_FLOAT:
                            {
                                res = get_token_float(parser, tok, &val.f);
                                break;
                            }
                        }
                        
                        if (res != KOW_STATUS_OK)
                            return -1;
                        if ((*data_offset) + size > data_size)
                            return -3;

                        memcpy(data, &val, size);
                        data = (char*)data + size;
                        data_size -= size;
                        *data_offset += size;
                    }
                    continue;
                }
                else if (token_string_match(parser, tok, CHILDREN) ||
                            token_string_match(parser, tok, ARRAY))
                {
                    int parent_array_index;
                    int parent_array_size = 1, start_nodes_populated = *desc_nodes_populated;
                    int k, nodes_populated, _data_offset;
                    int largest_child_offset = 0;
                    jsmntok_t* array_tok = tok + 1;
                    i++;
                    token_index++;

                    if (token_string_match(parser, tok, ARRAY))
                    {
                        parent_array_size = array_tok->size;
                        array_tok = array_tok + 1;
                        token_index++;
                    }
                    for (parent_array_index = 0; parent_array_index < parent_array_size; parent_array_index++)
                    {
                        desc = initial_desc;
                        *desc_nodes_populated = start_nodes_populated;

                        if (array_tok->size > 0)
                        {
                            token_index++;
                            for (k = 0; k < array_tok->size; k++)
                            {
                                res = process_tree_token(parser, token_index, desc + 1, desc_size - 1, &nodes_populated, data, data_size, &_data_offset);
                                if (res >= 0)
                                {
                                    if (largest_child_offset < _data_offset)
                                        largest_child_offset = _data_offset;
                                    if (initial_desc->type == KOW_BRANCH_U_START)
                                        _data_offset = 0;
                                    token_index += res;
                                    if (k < array_tok->size - 1)
                                        token_index++;
                                    desc += nodes_populated;
                                    desc_size -= nodes_populated;
                                    *desc_nodes_populated += nodes_populated;
                                    data = (char*)data + _data_offset;
                                    data_size -= _data_offset;
                                    *data_offset += _data_offset;
                                }
                                else
                                    return res;
                            }
                            if (initial_desc->type == KOW_BRANCH_U_START)
                            {
                                data = (char*)data + largest_child_offset;
                                data_size -= largest_child_offset;
                                *data_offset += largest_child_offset;
                            }
                        }
                        if (parent_array_index < parent_array_size - 1)
                            token_index++;
                    }
                }
            }
        }
        token_is_branch = initial_desc->type == KOW_BRANCH_START || initial_desc->type == KOW_BRANCH_U_START;
        if (token_is_branch)
        {
            desc++;
            desc_size--;

            if (desc_size < 1)
                return -2;

            desc->type = KOW_BRANCH_END;
            desc->symbol = initial_desc->symbol;
            desc->count = 0;
            desc->tag = 0;
            (*desc_nodes_populated)++;
        }
        (*desc_nodes_populated)++;
    }
    return token_index - initial_token_index;
}

int kowhai_deserialize_tree(char* buffer, void* scratch, int scratch_size, struct kowhai_node_t* descriptor, int* descriptor_size, void* data, int* data_size)
{
    int desc_nodes_populated, data_offset, result;
    jsmn_parser parser;
    jsmntok_t* tokens = (jsmntok_t*)scratch;
    int token_count = scratch_size / sizeof(jsmntok_t);
    jsmnerr_t err;
    jsmn_init_parser(&parser, buffer, tokens, token_count);
    err = jsmn_parse(&parser);
    switch (err)
    {
        case JSMN_ERROR_INVAL:
        case JSMN_ERROR_PART:
            return KOW_STATUS_BUFFER_INVALID;
        case JSMN_ERROR_NOMEM:
            return KOW_STATUS_SCRATCH_TOO_SMALL;
        default:
            break;
    }

    result = process_tree_token(&parser, 0, descriptor, *descriptor_size, &desc_nodes_populated, data, *data_size, &data_offset);
    switch (result)
    {
        case -1:
            return KOW_STATUS_BUFFER_INVALID;
        case -2:
        case -3:
            return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
        default:
            break;
    }
    *descriptor_size = desc_nodes_populated;
    *data_size = data_offset;

    return KOW_STATUS_OK;
}

static int process_nodes_token(jsmn_parser *parser, int src_size, struct kowhai_tree_t *dst_tree, union kowhai_symbol_t *path, int path_len, int *node_count, void *get_name_param, kowhai_get_symbol_t get_name, void *not_found_param, kowhai_node_not_found_t not_found)
{
    int res;
    int t = 0;
    jsmntok_t *path_tok = NULL;
    uint16_t type = KOW_BRANCH_START;
    uint16_t count = 0;
    uint16_t tag = 0;

    while (t < parser->num_tokens)
    {
        // get next token, and check if we are done
        jsmntok_t *tok = &parser->tokens[t++];
        if (tok->end > src_size || tok->end < 0 || tok->start > src_size || tok->start < 0)
            break;
        
        // clear all stored info for the previous kowhai node object
        if (tok->type == JSMN_OBJECT)
        {
            path_tok = NULL;
            type = KOW_BRANCH_START; // invalid value
            count = 0;
            tag = 0;
        }

        // if this is a path token then store a pointer to the path
        if (token_string_match(parser, tok, PATH))
        {
            path_tok = tok + 1;
            t++;
            continue;
        }
        
        // if this is a type token the store the type
        if (token_string_match(parser, tok, TYPE))
        {
            res = get_token_uint16(parser, tok + 1, &type);
            if (res != KOW_STATUS_OK)
                return -1;
            t++;
            continue;
        }

        // if this is a count token then store the count
        if (token_string_match(parser, tok, COUNT))
        {
            res = get_token_uint16(parser, tok + 1, &count);
            if (res != KOW_STATUS_OK)
                return -1;
            t++;
            continue;
        }
        
        // if this is a tag token then store the tag
        if (token_string_match(parser, tok, TAG))
        {
            res = get_token_uint16(parser, tok + 1, &tag);
            if (res != KOW_STATUS_OK)
                return -1;
            t++;
            continue;
        }
        
        // if this is a value token then parse the values and write node to dst_tree
        if (token_string_match(parser, tok, VALUE))
        {
            int i;
            int N;
            int path_syms = 0;
            int size = kowhai_get_node_type_size(type);

            ///@todo check node types match

            // parse path string to kowhai_path
            path_syms = str_to_path(parser, path_tok, path, path_len, get_name_param, get_name);
            if (path_syms < 0)
                return path_syms;

            // skip the [ or " wrappers for array like objects
            if (count > 1)
            {
                t++;
                tok++;
                N = MIN(tok->size, count);
            }
            else
                N = 1;

            // special string case
            if (type == KOW_CHAR)
            {
                char null_buf[1] = {0,};
                int len = MIN(tok->end - tok->start + 1, count) * size;
                res = kowhai_write(dst_tree, path_syms, path, 0, (void *)(parser->js + tok->start), len);
                if (res == KOW_STATUS_INVALID_SYMBOL_PATH)
                {
                    if ((not_found != NULL) && (not_found(not_found_param, path, path_syms) == 0))
                        res = KOW_STATUS_OK;
                }
                if (res != KOW_STATUS_OK)
                    return -2;
                // null terminate all strings !
                res = kowhai_write(dst_tree, path_syms, path, len - size, (void *)null_buf, sizeof(null_buf));
                if (res == KOW_STATUS_INVALID_SYMBOL_PATH)
                {
                    if ((not_found != NULL) && (not_found(not_found_param, path, path_syms) == 0))
                        res = KOW_STATUS_OK;
                }
                if (res != KOW_STATUS_OK)
                    return -2;
                t++;
                continue;
            }

            // process other items 1 value at a time
            for (i = 0; i < N; i++)
            {
                union any_type_t val;

                t++;
                tok++;

                switch (type)
                {
                    case KOW_UINT8:
                    case KOW_INT8:
                    case KOW_CHAR:
                        res = get_token_uint8(parser, tok, &val.ui8);
                        break;
                    case KOW_UINT16:
                    case KOW_INT16:
                        res = get_token_uint16(parser, tok, &val.ui16);
                        break;
                    case KOW_UINT32:
                    case KOW_INT32:
                        res = get_token_uint32(parser, tok, &val.ui32);
                        break;
                    case KOW_FLOAT:
                        res = get_token_float(parser, tok, &val.f);
                        break;
                }
                if (res != KOW_STATUS_OK)
                    return -1;

                res = kowhai_write(dst_tree, path_syms, path, 0, &val, size);
                if (res == KOW_STATUS_INVALID_SYMBOL_PATH)
                {
                    if ((not_found != NULL) && (not_found(not_found_param, path, path_syms) == 0))
                        res = KOW_STATUS_OK;
                }
                if (res != KOW_STATUS_OK)
                    return -2;
                path[path_syms - 1].parts.array_index++;
            }
        }

    }

    return 0;
}

int kowhai_deserialize_nodes(char* src, int src_size, struct kowhai_tree_t *dst_tree, union kowhai_symbol_t *path, int path_len, void* scratch, int scratch_size, void *get_name_param, kowhai_get_symbol_t get_name, void *not_found_param, kowhai_node_not_found_t not_found)
{
    int desc_nodes_populated = 0, result;
    jsmn_parser parser;
    jsmntok_t* tokens = (jsmntok_t*)scratch;
    int token_count = scratch_size / sizeof(jsmntok_t);
    jsmnerr_t err;

    jsmn_init_parser(&parser, src, tokens, token_count);
    err = jsmn_parse(&parser);
    switch (err)
    {
        case JSMN_ERROR_INVAL:
        case JSMN_ERROR_PART:
            return KOW_STATUS_BUFFER_INVALID;
        case JSMN_ERROR_NOMEM:
            return KOW_STATUS_SCRATCH_TOO_SMALL;
        default:
            break;
    }

    result = process_nodes_token(&parser, src_size, dst_tree, path, path_len, &desc_nodes_populated, get_name_param, get_name, not_found_param, not_found);
    switch (result)
    {
        case -1:
            return KOW_STATUS_UNKNOWN_ERROR;
        case -2:
            return KOW_STATUS_INVALID_SYMBOL_PATH;
        case -3:
            return KOW_STATUS_PATH_TOO_SMALL;
        case -4:
            return KOW_STATUS_NOT_FOUND;
        default:
            return KOW_STATUS_OK;
    }

}
