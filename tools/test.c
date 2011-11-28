#include "../src/kowhai.h"
#include "../src/kowhai_protocol.h"
#include "../src/kowhai_protocol_server.h"
#include "xpsocket.h"
#include "beep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//
// treenode symbols
//

#include "symbols.h"

//
// settings tree descriptor
//

#define FLUX_CAP_COUNT 2
#define COEFF_COUNT    6

struct kowhai_node_t settings_descriptor[] =
{
    { BRANCH_START,     SYM_SETTINGS,       1,                0 },

    { BRANCH_START,     SYM_FLUXCAPACITOR,  FLUX_CAP_COUNT,   0 },
    { UINT32_T,         SYM_FREQUENCY,      1,                0 },
    { UINT32_T,         SYM_GAIN,           1,                0 },
    { FLOAT_T,          SYM_COEFFICIENT,    COEFF_COUNT,      0 },
    { BRANCH_END,       SYM_FLUXCAPACITOR,  0,                0 },

    { BRANCH_START,     SYM_OVEN,           1,                0 },
    { INT16_T,          SYM_TEMP,           1,                0 },
    { UINT16_T,         SYM_TIMEOUT,        1,                0 },
    { BRANCH_END,       SYM_OVEN,           0,                0 },

    { BRANCH_END,       SYM_SETTINGS,       1,                0 },
};

//
// shadow tree descriptor
//

struct kowhai_node_t shadow_descriptor[] =
{
    { BRANCH_START,     SYM_SHADOW,         1,                0 },
    { UINT8_T,          SYM_RUNNING,        1,                0 },
    { UINT8_T,          SYM_STATUS,         1,                0 },
    { BRANCH_END,       SYM_SHADOW,         0,                0 },
};

//
// action tree descriptor
//

#define ACTION_START 1
#define ACTION_STOP  2
#define ACTION_BEEP  3

struct kowhai_node_t action_descriptor[] =
{
    { BRANCH_START,     SYM_ACTIONS,        1,                 0 },
    { BRANCH_START,     SYM_START,          1,                 ACTION_START },
    { UINT32_T,         SYM_DELAY,          1,                 0 },
    { BRANCH_END,       SYM_START,          0,                 0 },
    { BRANCH_START,     SYM_STOP,           1,                 ACTION_STOP },
    { BRANCH_END,       SYM_STOP,           0,                 0 },
    { BRANCH_START,     SYM_BEEP,           1,                 ACTION_BEEP},
    { INT32_T,          SYM_FREQUENCY,      1,                 0 },
    { INT32_T,          SYM_DURATION,       1,                 0 },
    { BRANCH_END,       SYM_BEEP,           0,                 0 },
    { BRANCH_END,       SYM_ACTIONS,        0,                 0 },
};

//
// scope tree descriptor
//

#define NUM_PIXELS 512

struct kowhai_node_t scope_descriptor[] =
{
    { BRANCH_START,     SYM_SCOPE,          1,                 0 },
    { UINT16_T,         SYM_PIXELS,         NUM_PIXELS,        0 },
    { BRANCH_END,       SYM_SCOPE,          0,                 0 },
};

//
// settings tree structs
//

#pragma pack(1)

struct flux_capacitor_t
{
    uint32_t frequency;
    uint32_t gain;
    float coefficient[COEFF_COUNT];
};

struct oven_t
{
    int16_t temp;
    uint16_t timeout;
};

struct settings_data_t
{
    struct flux_capacitor_t flux_capacitor[FLUX_CAP_COUNT];
    struct oven_t oven;
};

//
// shadow tree stucts
//

struct shadow_data_t
{
    uint8_t running;
    uint8_t status;
};

//
// action tree stucts
//

struct start_t
{
    uint32_t delay;
};

struct beep_t
{
    int32_t freq;
    int32_t duration;
};

struct action_data_t
{
    struct start_t start;
    struct beep_t beep;
};

//
// scope tree stucts
//

struct scope_data_t
{
    uint16_t pixels[NUM_PIXELS];
};

#pragma pack()

//
// test commands
//

#define TEST_BASIC           0
#define TEST_PROTOCOL_SERVER 1
#define TEST_PROTOCOL_CLIENT 2

//
// test tree ids
//

#define TREE_ID_SETTINGS 0
#define TREE_ID_SHADOW   1
#define TREE_ID_ACTIONS  2
#define TREE_ID_SCOPE    3

//
// main
//

struct settings_data_t settings;
struct shadow_data_t shadow;
struct action_data_t action = { 100, 500, 100 };
struct scope_data_t scope;

#define MAX_PACKET_SIZE 0x40

void node_written(void* param, struct kowhai_node_t* node)
{
    if (node->tag == ACTION_BEEP)
        beep(action.beep.freq, action.beep.duration);
}

void server_buffer_send(void* param, void* buffer, size_t buffer_size)
{
    xpsocket_handle conn = (xpsocket_handle)param;
    xpsocket_send(conn, buffer, buffer_size);
}

void server_buffer_received(xpsocket_handle conn, void* param, char* buffer, int buffer_size)
{
    int i;
    struct kowhai_protocol_server_t* server = (struct kowhai_protocol_server_t*)param;
    server->send_packet_param = conn;

    // randomize the scope buffer for funzies
    for (i = 0; i < NUM_PIXELS; i++)
        scope.pixels[i] = rand();

    kowhai_server_process_packet(server, buffer, buffer_size);
}

int main(int argc, char* argv[])
{
    int test_command = TEST_BASIC;

    struct kowhai_symbol_t symbols1[] = {{SYM_SETTINGS}, {SYM_OVEN}, {SYM_TEMP}};
    struct kowhai_symbol_t symbols2[] = {{SYM_SETTINGS}, {SYM_OVEN}, {SYM_TIMEOUT}};
    struct kowhai_symbol_t symbols3[] = {{SYM_SETTINGS}, {SYM_FLUXCAPACITOR}};
    struct kowhai_symbol_t symbols4[] = {{431}, {12343}};
    struct kowhai_symbol_t symbols5[] = {{SYM_SHADOW}, {SYM_STATUS}};
    struct kowhai_symbol_t symbols6[] = {{SYM_SETTINGS}, {SYM_FLUXCAPACITOR}, {SYM_GAIN}};
    struct kowhai_symbol_t symbols7[] = {{SYM_SETTINGS}, {SYM_FLUXCAPACITOR}, {SYM_COEFFICIENT}};
    struct kowhai_symbol_t symbols8[] = {{SYM_SETTINGS}, {SYM_FLUXCAPACITOR, 1}, {SYM_GAIN}};
    struct kowhai_symbol_t symbols9[] = {{SYM_SETTINGS}, {SYM_FLUXCAPACITOR, 1}, {SYM_COEFFICIENT, 3}};
    struct kowhai_symbol_t symbols10[] = {{SYM_SETTINGS}, {SYM_FLUXCAPACITOR}, {SYM_COEFFICIENT, 3}};
    struct kowhai_symbol_t symbols11[] = {{SYM_SETTINGS}, {SYM_OVEN}};
    struct kowhai_symbol_t symbols12[] = {{SYM_SETTINGS}, {SYM_FLUXCAPACITOR, 1}};

    uint16_t offset;
    int size;
    struct kowhai_node_t* node;

    uint8_t status;
    uint16_t temp;
    uint16_t timeout;
    uint32_t gain;
    float coeff;
    struct flux_capacitor_t flux_capacitor;

#ifdef KOWHAI_DBG
    printf("kowhai debugging enabled!\n");
#endif

    // determine test command
    if (argc > 1)
    {
        if (strcmp("server", argv[1]) == 0)
            test_command = TEST_PROTOCOL_SERVER;
        else if (strcmp("client", argv[1]) == 0)
            test_command = TEST_PROTOCOL_CLIENT;
    }

    // test tree parsing
    printf("test kowhai_get_node...\t\t\t");
    assert(kowhai_get_node(settings_descriptor, NULL, 3, symbols1, &offset, &node) == STATUS_OK);
    assert(offset == 64);
    assert(node == &settings_descriptor[7]);
    assert(kowhai_get_node(settings_descriptor, NULL, 3, symbols2, &offset, &node) == STATUS_OK);
    assert(offset == 66);
    assert(node == &settings_descriptor[8]);
    assert(kowhai_get_node(settings_descriptor, NULL, 2, symbols3, &offset, &node) == STATUS_OK);
    assert(offset == 0);
    assert(node == &settings_descriptor[1]);
    assert(kowhai_get_node(settings_descriptor, NULL, 2, symbols4, &offset, &node) == STATUS_INVALID_SYMBOL_PATH);
    assert(kowhai_get_node(settings_descriptor, NULL, 3, symbols6, &offset, &node) == STATUS_OK);
    assert(offset == 4);
    assert(node == &settings_descriptor[3]);
    assert(kowhai_get_node(settings_descriptor, NULL, 3, symbols8, &offset, &node) == STATUS_OK);
    assert(offset == sizeof(struct flux_capacitor_t) + 4);
    assert(node == &settings_descriptor[3]);
    assert(kowhai_get_node(settings_descriptor, NULL, 3, symbols9, &offset, &node) == STATUS_OK);
    assert(offset == sizeof(struct flux_capacitor_t) + 8 + 3 * 4);
    assert(node == &settings_descriptor[4]);
    assert(kowhai_get_node(settings_descriptor, NULL, 3, symbols10, &offset, &node) == STATUS_OK);
    assert(offset == 8 + 3 * 4);
    assert(node == &settings_descriptor[4]);
    assert(kowhai_get_node(settings_descriptor, NULL, 2, symbols12, &offset, &node) == STATUS_OK);
    assert(offset == sizeof(struct flux_capacitor_t));
    assert(node == &settings_descriptor[1]);
    printf(" passed!\n");

    // test get node size
    printf("test kowhai_get_node_size...\t\t");
    assert(kowhai_get_node_size(settings_descriptor, NULL, &size) == STATUS_OK);
    assert(size == sizeof(struct settings_data_t));
    printf(" passed!\n");

    // test read/write settings
    printf("test kowhai_read/kowhai_write...\t");
    status = 1;
    shadow.status = 0;
    assert(kowhai_write(shadow_descriptor, &shadow, 2, symbols5, 0, &status, 1) == STATUS_OK);
    assert(shadow.status == 1);
    status = 0;
    assert(kowhai_read(shadow_descriptor, &shadow, 2, symbols5, 0, &status, 1) == STATUS_OK);
    assert(status == 1);
    timeout = 999;
    settings.oven.timeout = 0;
    assert(kowhai_write(settings_descriptor, &settings, 3, symbols2, 0, &timeout, sizeof(timeout)) == STATUS_OK);
    assert(settings.oven.timeout == 999);
    timeout = 0;
    assert(kowhai_read(settings_descriptor, &settings, 3, symbols2, 0, &timeout, sizeof(timeout)) == STATUS_OK);
    assert(timeout == 999);
    flux_capacitor.frequency = 100; flux_capacitor.gain = 200;
    settings.flux_capacitor[0].frequency = 0; settings.flux_capacitor[0].gain = 0;
    assert(kowhai_write(settings_descriptor, &settings, 2, symbols3, 0, &flux_capacitor, sizeof(flux_capacitor)) == STATUS_OK);
    assert(settings.flux_capacitor[0].frequency == 100 && settings.flux_capacitor[0].gain == 200);
    flux_capacitor.frequency = 0; flux_capacitor.gain = 0;
    assert(kowhai_read(settings_descriptor, &settings, 2, symbols3, 0, &flux_capacitor, sizeof(flux_capacitor)) == STATUS_OK);
    assert(flux_capacitor.frequency == 100 && flux_capacitor.gain == 200);
    coeff = 999.9f;
    settings.flux_capacitor[1].coefficient[3] = 0;
    assert(kowhai_write(settings_descriptor, &settings, 3, symbols9, 0, &coeff, sizeof(coeff)) == STATUS_OK);
    assert(settings.flux_capacitor[1].coefficient[3] == 999.9f);
    coeff = 0;
    assert(kowhai_read(settings_descriptor, &settings, 3, symbols9, 0, &coeff, sizeof(coeff)) == STATUS_OK);
    assert(coeff == 999.9f);
    printf(" passed!\n");

    // test set/get settings
    printf("test kowhai_get_xxx/kowhai_set_xxx...\t");
    shadow.status = 0;
    assert(kowhai_set_char(shadow_descriptor, &shadow, 2, symbols5, 255) == STATUS_OK);
    assert(shadow.status == 255);
    assert(kowhai_get_char(shadow_descriptor, &shadow, 2, symbols5, &status) == STATUS_OK);
    assert(status == 255);
    settings.oven.temp = 0;
    assert(kowhai_set_int16(settings_descriptor, &settings, 3, symbols1, 999) == STATUS_OK);
    assert(settings.oven.temp == 999);
    assert(kowhai_get_int16(settings_descriptor, &settings, 3, symbols1, &temp) == STATUS_OK);
    assert(temp == 999);
    settings.oven.timeout = 0;
    assert(kowhai_set_int16(settings_descriptor, &settings, 3, symbols2, 999) == STATUS_OK);
    assert(settings.oven.timeout == 999);
    assert(kowhai_get_int16(settings_descriptor, &settings, 3, symbols2, &timeout) == STATUS_OK);
    assert(timeout == 999);
    settings.flux_capacitor[0].gain = 0;
    assert(kowhai_set_int32(settings_descriptor, &settings, 3, symbols6, 999) == STATUS_OK);
    assert(settings.flux_capacitor[0].gain == 999);
    assert(kowhai_get_int32(settings_descriptor, &settings, 3, symbols6, &gain) == STATUS_OK);
    assert(gain == 999);
    settings.flux_capacitor[0].coefficient[0] = 0;
    assert(kowhai_set_float(settings_descriptor, &settings, 3, symbols7, 999.9f) == STATUS_OK);
    assert(settings.flux_capacitor[0].coefficient[0] ==  999.9f);
    assert(kowhai_get_float(settings_descriptor, &settings, 3, symbols7, &coeff) == STATUS_OK);
    assert(coeff == 999.9f);
    printf(" passed!\n");

    // test server protocol
    if (test_command == TEST_PROTOCOL_SERVER)
    {
        char packet_buffer[MAX_PACKET_SIZE];
        struct kowhai_node_t* tree_descriptors[] = {settings_descriptor, shadow_descriptor, action_descriptor, scope_descriptor};
        size_t tree_descriptor_sizes[] = {sizeof(settings_descriptor), sizeof(shadow_descriptor), sizeof(action_descriptor), sizeof(scope_descriptor)};
        void* tree_data_buffers[] = {&settings, &shadow, &action, &scope};
        struct kowhai_protocol_server_t server = {MAX_PACKET_SIZE, packet_buffer, node_written, NULL, server_buffer_send, NULL, 4, tree_descriptors, tree_descriptor_sizes, tree_data_buffers};
        printf("test server protocol...\n");
        xpsocket_init();
        xpsocket_serve(server_buffer_received, &server, MAX_PACKET_SIZE);
        xpsocket_cleanup();
    }

    // test client protocol
    if (test_command == TEST_PROTOCOL_CLIENT)
    {
        xpsocket_handle conn;
        printf("test client protocol...\n");
        xpsocket_init();
        conn = xpsocket_init_client();
        if (conn != NULL)
        {
            char buffer[MAX_PACKET_SIZE];
            int bytes_required;
            int received_size;
            struct kowhai_protocol_t prot;
            int overhead;
            char value;
            struct oven_t oven = {0x0102, 321};
            struct flux_capacitor_t flux_cap[2] = {{100, 200, {1, 2, 3, 4, 5, 6}}, {110, 210, {11, 12, 13, 14, 15, 16}}};
            // write oven.temp
            temp = 25;
            POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, CMD_WRITE_DATA, 3, symbols1, INT16_T, 0, sizeof(uint16_t), &temp);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_WRITE_DATA_ACK);
            assert(prot.payload.spec.data.symbols.count == 3);
            assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(struct kowhai_symbol_t) * 3) == 0);
            assert(prot.payload.spec.data.memory.type == INT16_T);
            assert(prot.payload.spec.data.memory.offset == 0);
            assert(prot.payload.spec.data.memory.size == sizeof(uint16_t));
            assert(*((uint16_t*)prot.payload.buffer) == temp);
            // write low byte of oven.temp
            value = 255;
            POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, CMD_WRITE_DATA, 3, symbols1, INT16_T, 1, 1, &value);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_WRITE_DATA_ACK);
            assert(prot.payload.spec.data.symbols.count == 3);
            assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(struct kowhai_symbol_t) * 3) == 0);
            assert(prot.payload.spec.data.memory.type == INT16_T);
            assert(prot.payload.spec.data.memory.offset == 1);
            assert(prot.payload.spec.data.memory.size == 1);
            assert(*((char*)prot.payload.buffer) == value);
            // double check oven.temp
            POPULATE_PROTOCOL_READ(prot, TREE_ID_SETTINGS, CMD_READ_DATA, 3, symbols1);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(*((uint16_t*)prot.payload.buffer) >> 8 == (uint8_t)value);
            // write oven
            POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, CMD_WRITE_DATA, 2, symbols11, 0, 0, sizeof(oven), &oven);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_WRITE_DATA_ACK);
            assert(prot.payload.spec.data.symbols.count == 2);
            assert(memcmp(prot.payload.spec.data.symbols.array_, symbols11, sizeof(struct kowhai_symbol_t) * 2) == 0);
            assert(prot.payload.spec.data.memory.type == 0);
            assert(prot.payload.spec.data.memory.offset == 0);
            assert(prot.payload.spec.data.memory.size == sizeof(oven));
            assert(memcmp(prot.payload.buffer, &oven, sizeof(oven)) == 0);
            // write flux capacitor array
            POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, CMD_WRITE_DATA, 2, symbols3, 0, 0, sizeof(struct flux_capacitor_t) * 2, flux_cap);
            kowhai_protocol_get_overhead(&prot, &overhead);
            prot.payload.spec.data.memory.size = MAX_PACKET_SIZE - overhead;
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_WRITE_DATA_ACK);
            assert(prot.payload.spec.data.symbols.count == 2);
            assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(struct kowhai_symbol_t) * 2) == 0);
            assert(prot.payload.spec.data.memory.type == 0);
            assert(prot.payload.spec.data.memory.offset == 0);
            assert(prot.payload.spec.data.memory.size == MAX_PACKET_SIZE - overhead);
            assert(memcmp(prot.payload.buffer, flux_cap, MAX_PACKET_SIZE - overhead) == 0);
            offset = prot.payload.spec.data.memory.size;
            size = sizeof(struct flux_capacitor_t) * 2 - offset;
            POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, CMD_WRITE_DATA, 2, symbols3, 0, offset, size, (char*)flux_cap + offset);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.command == CMD_WRITE_DATA_ACK);
            assert(prot.payload.spec.data.memory.offset == offset);
            assert(prot.payload.spec.data.memory.size == size);
            assert(memcmp(prot.payload.buffer, (char*)flux_cap + offset, size) == 0);
            // write flux capacitor[1]
            POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, CMD_WRITE_DATA, 2, symbols12, 0, 0, sizeof(struct flux_capacitor_t), &flux_cap[1]);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_WRITE_DATA_ACK);
            assert(prot.payload.spec.data.symbols.count == 2);
            assert(memcmp(prot.payload.spec.data.symbols.array_, symbols12, sizeof(struct kowhai_symbol_t) * 2) == 0);
            assert(prot.payload.spec.data.memory.type == 0);
            assert(prot.payload.spec.data.memory.offset == 0);
            assert(prot.payload.spec.data.memory.size == sizeof(struct flux_capacitor_t));
            assert(memcmp(prot.payload.buffer, &flux_cap[1], sizeof(struct flux_capacitor_t)) == 0);
            // read oven.temp
            POPULATE_PROTOCOL_READ(prot, TREE_ID_SETTINGS, CMD_READ_DATA, 3, symbols1);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_READ_DATA_ACK_END);
            assert(prot.payload.spec.data.symbols.count == 3);
            assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(struct kowhai_symbol_t) * 3) == 0);
            assert(prot.payload.spec.data.memory.type == INT16_T);
            assert(prot.payload.spec.data.memory.offset == 0);
            assert(prot.payload.spec.data.memory.size == sizeof(int16_t));
            assert(*((int16_t*)prot.payload.buffer) == 0x0102);
            // read flux capacitor array
            POPULATE_PROTOCOL_READ(prot, TREE_ID_SETTINGS, CMD_READ_DATA, 2, symbols3);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_READ_DATA_ACK);
            assert(prot.payload.spec.data.symbols.count == 2);
            assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(struct kowhai_symbol_t) * 2) == 0);
            assert(prot.payload.spec.data.memory.type == 0);
            assert(prot.payload.spec.data.memory.offset == 0);
            kowhai_protocol_get_overhead(&prot, &overhead);
            assert(overhead == 17);
            assert(prot.payload.spec.data.memory.size == MAX_PACKET_SIZE - overhead);
            assert(memcmp(prot.payload.buffer, flux_cap, prot.payload.spec.data.memory.size) == 0);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_READ_DATA_ACK_END);
            assert(prot.payload.spec.data.symbols.count == 2);
            assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(struct kowhai_symbol_t) * 2) == 0);
            assert(prot.payload.spec.data.memory.type == 0);
            assert(prot.payload.spec.data.memory.offset == MAX_PACKET_SIZE - overhead);
            assert(prot.payload.spec.data.memory.size == sizeof(struct flux_capacitor_t) * 2 - prot.payload.spec.data.memory.offset);
            assert(memcmp(prot.payload.buffer, (char*)flux_cap + prot.payload.spec.data.memory.offset, prot.payload.spec.data.memory.size) == 0);
            // read settings tree descriptor
            POPULATE_PROTOCOL_CMD(prot, TREE_ID_SETTINGS, CMD_READ_DESCRIPTOR);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_READ_DESCRIPTOR_ACK);
            assert(prot.payload.spec.descriptor.node_count == sizeof(settings_descriptor) / sizeof(settings_descriptor[0]));
            assert(prot.payload.spec.data.memory.offset == 0);
            kowhai_protocol_get_overhead(&prot, &overhead);
            assert(overhead == 8);
            assert(prot.payload.spec.descriptor.size == MAX_PACKET_SIZE - overhead);
            assert(memcmp(prot.payload.buffer, settings_descriptor, prot.payload.spec.data.memory.size) == 0);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_READ_DESCRIPTOR_ACK_END);
            assert(prot.payload.spec.descriptor.node_count == sizeof(settings_descriptor) / sizeof(settings_descriptor[0]));
            assert(prot.payload.spec.descriptor.offset == MAX_PACKET_SIZE - overhead);
            assert(prot.payload.spec.descriptor.size == sizeof(settings_descriptor) - prot.payload.spec.descriptor.offset);
            assert(memcmp(prot.payload.buffer, (char*)settings_descriptor + prot.payload.spec.data.memory.offset, prot.payload.spec.data.memory.size) == 0);
            // test invalid tree id
            POPULATE_PROTOCOL_CMD(prot, 255, CMD_READ_DESCRIPTOR);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == 255);
            assert(prot.header.command == CMD_ERROR_INVALID_TREE_ID);
            // test invalid command
            POPULATE_PROTOCOL_CMD(prot, TREE_ID_SETTINGS, CMD_READ_DESCRIPTOR);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            buffer[1] = 255;
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_ERROR_INVALID_COMMAND);
            // test invalid symbol path
            POPULATE_PROTOCOL_READ(prot, TREE_ID_SETTINGS, CMD_READ_DATA, 2, symbols4);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_ERROR_INVALID_SYMBOL_PATH);
            POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, CMD_WRITE_DATA, 2, symbols4, INT16_T, 0, sizeof(uint16_t), &temp);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.tree_id == TREE_ID_SETTINGS);
            assert(prot.header.command == CMD_ERROR_INVALID_SYMBOL_PATH);

            xpsocket_free_client(conn);
        }
        xpsocket_cleanup();
        printf("\t\t\t\t\t passed!\n");
    }

    return 0;
}
