#ifndef _KOWHAI_PROTOCOL_H_
#define _KOWHAI_PROTOCOL_H_

#include "kowhai.h" 

//
// Protocol commands
//

// Write tree data
#define CMD_WRITE_DATA                   0x00
// Acknowledge write tree data command
#define CMD_WRITE_DATA_ACK               0x0F
// Read tree data
#define CMD_READ_DATA                    0x10
// Acknowledge read tree data command (and return the data)
#define CMD_READ_DATA_ACK                0x1F
// Acknowledge read tree data command (this is the final packet)
#define CMD_READ_DATA_ACK_END            0x1E
// Read the tree descriptor
#define CMD_READ_DESCRIPTOR              0x20
// Acknowledge read tree command (and return tree contents)
#define CMD_READ_DESCRIPTOR_ACK          0x2F
// Acknowledge read tree command (this is the final packet)
#define CMD_READ_DESCRIPTOR_ACK_END      0x2E
// Error codes
#define CMD_ERROR_INVALID_TREE_ID        0xF0
#define CMD_ERROR_INVALID_COMMAND        0xF1
#define CMD_ERROR_INVALID_SYMBOL_PATH    0xF2
#define CMD_ERROR_INVALID_PAYLOAD_OFFSET 0xF3
#define CMD_ERROR_INVALID_PAYLOAD_SIZE   0xF4
#define CMD_ERROR_UNKNOWN                0xFF

//
// Protocol structures
//

#pragma pack(1)

struct kowhai_protocol_header_t
{
    uint8_t tree_id;
    uint8_t command;
};

struct kowhai_protocol_symbol_spec_t
{
    uint8_t count;
    struct kowhai_symbol_t* array_;
};

struct kowhai_protocol_data_payload_memory_spec_t
{
    uint16_t type;
    uint16_t offset;
    uint16_t size;
};

struct kowhai_protocol_data_payload_spec_t
{
    struct kowhai_protocol_symbol_spec_t symbols;
    struct kowhai_protocol_data_payload_memory_spec_t memory;
};

struct kowhai_protocol_descriptor_payload_spec_t
{
    uint16_t node_count;
    uint16_t offset;
    uint16_t size;
};

union kowhai_protocol_payload_spec_t
{
    struct kowhai_protocol_data_payload_spec_t data;
    struct kowhai_protocol_descriptor_payload_spec_t descriptor;
};

struct kowhai_protocol_payload_t
{
    union kowhai_protocol_payload_spec_t spec;
    void* buffer;
};

struct kowhai_protocol_t
{
    struct kowhai_protocol_header_t header;
    struct kowhai_protocol_payload_t payload;
};

#pragma pack()

#define POPULATE_PROTOCOL_CMD(protocol, tree_id_, cmd)           \
    {                                                            \
        protocol.header.tree_id = tree_id_;                      \
        protocol.header.command = cmd;                           \
    }

#define POPULATE_PROTOCOL_READ(protocol, tree_id_, cmd, symbol_count_, symbols_) \
    {                                                            \
        POPULATE_PROTOCOL_CMD(protocol, tree_id_, cmd);          \
        protocol.payload.spec.data.symbols.count = symbol_count_;\
        protocol.payload.spec.data.symbols.array_ = symbols_;    \
    }

#define POPULATE_PROTOCOL_WRITE(protocol, tree_id_, cmd, symbol_count_, symbols_, data_type, data_offset, data_size, buffer_) \
    {                                                            \
        POPULATE_PROTOCOL_READ(protocol, tree_id_, cmd, symbol_count_, symbols_);\
        protocol.payload.spec.data.memory.type = data_type;      \
        protocol.payload.spec.data.memory.offset = data_offset;  \
        protocol.payload.spec.data.memory.size = data_size;      \
        protocol.payload.buffer = buffer_;                       \
    }

//
// Functions
//

/*
 * Parse a protocol packet and return the tree id
 */
int kowhai_protocol_get_tree_id(void* proto_packet, int packet_size, uint8_t* tree_id);

/*
 * Parse a protocol packet and return the:
 *   - protocol header and payload information
 *   - node memory offset in the tree data
 *   - referenced node
 */
int kowhai_protocol_parse(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol);

/*
 * Create a protocol packet
 * If function succeeds bytes_required has number of bytes written to proto_packet,
 * if function fails because proto_packet size is too small bytes_required has number of bytes needed
 */
int kowhai_protocol_create(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol, int* bytes_required);

/*
 * Return the protocol overhead (header, payload specification etc, ie the meta part of the protocol that describes the payload)
 * If function succeeds overhead has number of bytes of overhead
 */
int kowhai_protocol_get_overhead(struct kowhai_protocol_t* protocol, int* overhead);

#endif

