#ifndef _KOWHAI_PROTOCOL_SERVER_H_
#define _KOWHAI_PROTOCOL_SERVER_H_

#include "kowhai_protocol.h" 

#include <stddef.h>

typedef void (*kowhai_send_packet_t)(void* packet, int packet_size);

struct kowhai_protocol_server_t
{
    size_t max_packet_size;
    void* packet_buffer;
    kowhai_send_packet_t send_packet;
    int tree_count;
    struct kowhai_node_t** tree_descriptors;
    size_t* tree_descriptor_sizes;
    void** tree_data_buffers;
};

void kowhai_protocol_handle_packet(struct kowhai_protocol_server_t* server, char* packet, size_t packet_size);

#endif
