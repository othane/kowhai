#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "../src/kowhai.h"
#include "../src/kowhai_utils.h"
#include "../src/kowhai_protocol.h"
#include "../src/kowhai_protocol_server.h"
#include "../src/kowhai_serialize.h"
#include "xpsocket.h"
#include "beep.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <float.h>
#include <math.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

//
// treenode symbols
//

#include "symbols.h"

//
// settings tree descriptor
//

#define FLUX_CAP_COUNT 2
#define COEFF_COUNT    6
#define UNION_COUNT    2
#define OWNER_MAX_LEN  12

struct kowhai_node_t settings_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_SETTINGS,       1,                0 },

    { KOW_BRANCH_START,     SYM_FLUXCAPACITOR,  FLUX_CAP_COUNT,   0 },
    { KOW_CHAR,             SYM_OWNER,          OWNER_MAX_LEN,    0 },
    { KOW_UINT32,           SYM_FREQUENCY,      1,                0 },
    { KOW_UINT32,           SYM_GAIN,           1,                0 },
    { KOW_FLOAT,            SYM_COEFFICIENT,    COEFF_COUNT,      0 },
    { KOW_BRANCH_END,       SYM_FLUXCAPACITOR,  0,                0 },

    { KOW_BRANCH_START,     SYM_OVEN,           1,                0 },
    { KOW_INT16,            SYM_TEMP,           1,                0 },
    { KOW_UINT16,           SYM_TIMEOUT,        1,                0 },
    { KOW_BRANCH_END,       SYM_OVEN,           0,                0 },

    { KOW_BRANCH_START,     SYM_UNIONCONTAINER, UNION_COUNT,      0 },
    { KOW_BRANCH_U_START,   SYM_UNION,          UNION_COUNT,      0 },
    { KOW_INT16,            SYM_TEMP,           1,                0 },
    { KOW_UINT16,           SYM_TIMEOUT,        1,                0 },
    { KOW_UINT8,            SYM_BEEP,           1,                0 },
    { KOW_CHAR,             SYM_OWNER,          OWNER_MAX_LEN,    0 },
    { KOW_BRANCH_START,     SYM_PARTS,          1,                0 },
    { KOW_UINT8,            SYM_PART1,          1,                0 },
    { KOW_UINT8,            SYM_PART2,          1,                0 },
    { KOW_BRANCH_END,       SYM_PARTS,          0,                0 },
    { KOW_BRANCH_END,       SYM_UNION,          0,                0 },
    { KOW_UINT32,           SYM_CHECK,          1,                0 },
    { KOW_BRANCH_END,       SYM_UNIONCONTAINER, 0,                0 },

    { KOW_UINT32,           SYM_CHECK,          1,                0 },

    { KOW_BRANCH_END,       SYM_SETTINGS,       0,                0 },
};

//
// shadow tree descriptor
//

struct kowhai_node_t shadow_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_SHADOW,         1,                0 },
    { KOW_UINT8,            SYM_RUNNING,        1,                0 },
    { KOW_UINT8,            SYM_STATUS,         1,                0 },
    { KOW_BRANCH_END,       SYM_SHADOW,         0,                0 },
};

//
// function tree descriptors
//

struct kowhai_node_t start_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_START,          1,                 0 },
    { KOW_UINT32,           SYM_DELAY,          1,                 0 },
    { KOW_BRANCH_END,       SYM_START,          0,                 0 },
};

struct kowhai_node_t status_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_STATUS,         1,                 0 },
    { KOW_UINT32,           SYM_STATUS,         1,                 0 },
    { KOW_UINT32,           SYM_TIME,           1,                 0 },
    { KOW_BRANCH_END,       SYM_STATUS,         0,                 0 },
};

#define BIG_COEFF_COUNT 100

struct kowhai_node_t big_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_BIG,            1,                 0 },
    { KOW_UINT32,           SYM_STATUS,         1,                 0 },
    { KOW_UINT32,           SYM_TIME,           1,                 0 },
    { KOW_UINT32,           SYM_COEFFICIENT,    BIG_COEFF_COUNT,   0 },
    { KOW_BRANCH_END,       SYM_BIG,            0,                 0 },
};

struct kowhai_node_t beep_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_BEEP,           1,                 0},
    { KOW_INT32,            SYM_FREQUENCY,      1,                 0 },
    { KOW_INT32,            SYM_DURATION,       1,                 0 },
    { KOW_BRANCH_END,       SYM_BEEP,           0,                 0 },
};

struct kowhai_node_t unsolicited_event_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_UNSOLICITEDEVENT,1,                 0},
    { KOW_INT32,            SYM_TIME,           1,                 0 },
    { KOW_BRANCH_END,       SYM_UNSOLICITEDEVENT,0,                 0 },
};

//
// scope tree descriptor
//

#define NUM_PIXELS 512

struct kowhai_node_t scope_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_SCOPE,          1,                 0 },
    { KOW_UINT16,           SYM_PIXELS,         NUM_PIXELS,        0 },
    { KOW_BRANCH_END,       SYM_SCOPE,          0,                 0 },
};

//
// settings tree structs
//

#pragma pack(1)

struct flux_capacitor_t
{
    char owner[OWNER_MAX_LEN];
    uint32_t frequency;
    uint32_t gain;
    float coefficient[COEFF_COUNT];
};

struct oven_t
{
    int16_t temp;
    uint16_t timeout;
};

union union_t
{
    int16_t temp;
    uint16_t timeout;
    uint8_t beep;
    char owner[OWNER_MAX_LEN];
};

struct union_container_t
{
    union union_t union_[UNION_COUNT];
    uint32_t check;
};

struct settings_data_t
{
    struct flux_capacitor_t flux_capacitor[FLUX_CAP_COUNT];
    struct oven_t oven;
    struct union_container_t union_container[UNION_COUNT];
    uint32_t check;
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
// function tree stucts
//

struct start_data_t
{
    uint32_t delay;
};

struct status_data_t
{
    uint32_t status;
    uint32_t time;
};

struct big_data_t
{
    uint32_t status;
    uint32_t time;
    uint32_t coeff[BIG_COEFF_COUNT];
};

struct beep_data_t
{
    int32_t freq;
    int32_t duration;
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
// test trees
//

struct settings_data_t settings;
struct settings_data_t settings2;
struct kowhai_tree_t settings_tree = {settings_descriptor, &settings};
struct shadow_data_t shadow;
struct kowhai_tree_t shadow_tree = {shadow_descriptor, &shadow};
struct start_data_t start;
struct kowhai_tree_t start_tree = {start_descriptor, &start};
struct status_data_t status;
struct kowhai_tree_t status_tree = {status_descriptor, &status};
struct big_data_t big;
struct kowhai_tree_t big_tree = {big_descriptor, &big};
struct beep_data_t beepd;
struct kowhai_tree_t beep_tree = {beep_descriptor, &beepd};
struct scope_data_t scope;
struct kowhai_tree_t scope_tree = {scope_descriptor, &scope};

//
// test server structures
//

struct kowhai_protocol_server_tree_item_t tree_list[] = {
    { KOW_TREE_ID(SYM_SETTINGS),                settings_descriptor,            sizeof(settings_descriptor),            &settings },
    { KOW_TREE_ID(SYM_SHADOW),                  shadow_descriptor,              sizeof(shadow_descriptor),              &shadow },
    { KOW_TREE_ID(SYM_SCOPE),                   scope_descriptor,               sizeof(scope_descriptor),               &scope },
    { KOW_TREE_ID_FUNCTION_ONLY(SYM_START),     start_descriptor,               sizeof(start_descriptor),               &start },
    { KOW_TREE_ID_FUNCTION_ONLY(SYM_STATUS),    status_descriptor,              sizeof(status_descriptor),              &status },
    { KOW_TREE_ID_FUNCTION_ONLY(SYM_BEEP),      beep_descriptor,                sizeof(beep_descriptor),                &beepd },
    { KOW_TREE_ID_FUNCTION_ONLY(SYM_BIG),       big_descriptor,                 sizeof(big_descriptor),                 &big },
    { KOW_TREE_ID(SYM_UNSOLICITEDEVENT),        unsolicited_event_descriptor,   sizeof(unsolicited_event_descriptor),   NULL },
};
struct kowhai_protocol_id_list_item_t tree_id_list[COUNT_OF(tree_list)];
struct kowhai_protocol_server_function_item_t function_list[] = {
    { KOW_FUNCTION_ID(SYM_START),               {SYM_START, KOW_UNDEFINED_SYMBOL} },
    { KOW_FUNCTION_ID(SYM_STOP),                {KOW_UNDEFINED_SYMBOL, KOW_UNDEFINED_SYMBOL} },
    { KOW_FUNCTION_ID(SYM_STATUS),              {KOW_UNDEFINED_SYMBOL, SYM_STATUS} },
    { KOW_FUNCTION_ID(SYM_BEEP),                {SYM_BEEP, KOW_UNDEFINED_SYMBOL} },
    { KOW_FUNCTION_ID(SYM_BIG),                 {SYM_BIG, SYM_BIG} },
    { KOW_FUNCTION_ID(SYM_FAIL),                {KOW_UNDEFINED_SYMBOL, KOW_UNDEFINED_SYMBOL}},
    { KOW_FUNCTION_ID(SYM_UNSOLICITEDMODE),     {KOW_UNDEFINED_SYMBOL, KOW_UNDEFINED_SYMBOL}},
};
struct kowhai_protocol_id_list_item_t function_id_list[COUNT_OF(function_list)];

//
// test protocol packet size
//

#define MAX_PACKET_SIZE 0x40

//
// test functions
//

union kowhai_symbol_t symbols1[] = {SYM_SETTINGS, SYM_OVEN, SYM_TEMP};
union kowhai_symbol_t symbols2[] = {SYM_SETTINGS, SYM_OVEN, SYM_TIMEOUT};
union kowhai_symbol_t symbols3[] = {SYM_SETTINGS, SYM_FLUXCAPACITOR};
union kowhai_symbol_t symbols4[] = {431, 12343};
union kowhai_symbol_t symbols5[] = {SYM_SHADOW, SYM_STATUS};
union kowhai_symbol_t symbols6[] = {SYM_SETTINGS, SYM_FLUXCAPACITOR, SYM_GAIN};
union kowhai_symbol_t symbols7[] = {SYM_SETTINGS, SYM_FLUXCAPACITOR, SYM_COEFFICIENT};
union kowhai_symbol_t symbols8[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 1), SYM_GAIN};
union kowhai_symbol_t symbols9[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 1), KOWHAI_SYMBOL(SYM_COEFFICIENT, 3)};
union kowhai_symbol_t symbols10[] = {SYM_SETTINGS, SYM_FLUXCAPACITOR, KOWHAI_SYMBOL(SYM_COEFFICIENT, 3)};
union kowhai_symbol_t symbols11[] = {SYM_SETTINGS, SYM_OVEN};
union kowhai_symbol_t symbols12[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 1)};
union kowhai_symbol_t symbols13[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 0), KOWHAI_SYMBOL(SYM_OWNER, 3)};
union kowhai_symbol_t symbols14[] = {SYM_SETTINGS, SYM_UNIONCONTAINER, SYM_UNION, SYM_TEMP};
union kowhai_symbol_t symbols15[] = {SYM_SETTINGS, SYM_UNIONCONTAINER, SYM_UNION, SYM_TIMEOUT};
union kowhai_symbol_t symbols16[] = {SYM_SETTINGS, SYM_UNIONCONTAINER, SYM_UNION, SYM_BEEP};
union kowhai_symbol_t symbols17[] = {SYM_SETTINGS, SYM_UNIONCONTAINER, SYM_UNION, SYM_OWNER};
union kowhai_symbol_t symbols18[] = {SYM_SETTINGS, SYM_UNIONCONTAINER, SYM_CHECK};
union kowhai_symbol_t symbols19[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_UNIONCONTAINER, 1), SYM_CHECK};
union kowhai_symbol_t symbols99[] = {SYM_SETTINGS, SYM_CHECK};

void core_tests()
{
    int offset;
    int size;
    int count;
    struct kowhai_node_t* node;

    uint8_t status, beep;
    uint16_t temp;
    uint16_t timeout;
    uint32_t gain, check;
    float coeff;
    char owner_initial;
    struct flux_capacitor_t flux_capacitor = {"empty", 1, 2, 10, 20, 30, 40, 50, 60};

    // test version
    assert(kowhai_version() == 6);

    // test tree parsing
    printf("test kowhai_get_node...\t\t\t");
    assert(kowhai_get_node(settings_tree.desc, 3, symbols1, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, oven.temp));
    assert(node == &settings_descriptor[8]);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols2, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, oven.timeout));
    assert(node == &settings_descriptor[9]);
    assert(kowhai_get_node(settings_tree.desc, 2, symbols3, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor));
    assert(node == &settings_descriptor[1]);
    assert(kowhai_get_node(settings_tree.desc, 2, symbols4, &offset, &node) == KOW_STATUS_INVALID_SYMBOL_PATH);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols6, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[0].gain));
    assert(node == &settings_descriptor[4]);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols8, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[1].gain));
    assert(node == &settings_descriptor[4]);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols9, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[1].coefficient[3]));
    assert(node == &settings_descriptor[5]);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols10, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[0].coefficient[3]));
    assert(node == &settings_descriptor[5]);
    assert(kowhai_get_node(settings_tree.desc, 2, symbols12, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[1]));
    assert(node == &settings_descriptor[1]);

    printf(" passed!\n");

    // test get node size
    printf("test kowhai_get_node_size & kowhai_get_node_count...\t\t");
    assert(kowhai_get_node_size(settings_tree.desc, &size) == KOW_STATUS_OK);
    assert(size == sizeof(struct settings_data_t));
    assert(kowhai_get_node_count(settings_tree.desc, &count) == KOW_STATUS_OK);
    assert(count == sizeof(settings_descriptor)/sizeof(struct kowhai_node_t));
    assert(kowhai_get_node_count(&settings_tree.desc[2], &count) == KOW_STATUS_OK);
    assert(count == 1);
    printf(" passed!\n");

    // test read/write settings
    printf("test kowhai_read/kowhai_write...\t");
    status = 1;
    shadow.status = 0;
    assert(kowhai_write(&shadow_tree, 2, symbols5, 0, &status, 1) == KOW_STATUS_OK);
    assert(shadow.status == 1);
    status = 0;
    assert(kowhai_read(&shadow_tree, 2, symbols5, 0, &status, 1) == KOW_STATUS_OK);
    assert(status == 1);
    timeout = 999;
    settings.oven.timeout = 0;
    assert(kowhai_write(&settings_tree, 3, symbols2, 0, &timeout, sizeof(timeout)) == KOW_STATUS_OK);
    assert(settings.oven.timeout == 999);
    timeout = 0;
    assert(kowhai_read(&settings_tree, 3, symbols2, 0, &timeout, sizeof(timeout)) == KOW_STATUS_OK);
    assert(timeout == 999);
    flux_capacitor.frequency = 100; flux_capacitor.gain = 200;
    settings.flux_capacitor[0].frequency = 0; settings.flux_capacitor[0].gain = 0;
    assert(kowhai_write(&settings_tree, 2, symbols3, 0, &flux_capacitor, sizeof(flux_capacitor)) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[0].frequency == 100 && settings.flux_capacitor[0].gain == 200);
    flux_capacitor.frequency = 0; flux_capacitor.gain = 0;
    assert(kowhai_read(&settings_tree, 2, symbols3, 0, &flux_capacitor, sizeof(flux_capacitor)) == KOW_STATUS_OK);
    assert(flux_capacitor.frequency == 100 && flux_capacitor.gain == 200);
    coeff = 999.9f;
    settings.flux_capacitor[1].coefficient[3] = 0;
    assert(kowhai_write(&settings_tree, 3, symbols9, 0, &coeff, sizeof(coeff)) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[1].coefficient[3] == 999.9f);
    coeff = 0;
    assert(kowhai_read(&settings_tree, 3, symbols9, 0, &coeff, sizeof(coeff)) == KOW_STATUS_OK);
    assert(coeff == 999.9f);
    printf(" passed!\n");

    // test set/get settings
    printf("test kowhai_get_xxx/kowhai_set_xxx...\t");
    shadow.status = 0;
    assert(kowhai_set_int8(&shadow_tree, 2, symbols5, 255) == KOW_STATUS_OK);
    assert(shadow.status == 255);
    assert(kowhai_get_int8(&shadow_tree, 2, symbols5, &status) == KOW_STATUS_OK);
    assert(status == 255);
    settings.oven.temp = 0;
    assert(kowhai_set_int16(&settings_tree, 3, symbols1, 999) == KOW_STATUS_OK);
    assert(settings.oven.temp == 999);
    assert(kowhai_get_int16(&settings_tree, 3, symbols1, &temp) == KOW_STATUS_OK);
    assert(temp == 999);
    settings.oven.timeout = 0;
    assert(kowhai_set_int16(&settings_tree, 3, symbols2, 999) == KOW_STATUS_OK);
    assert(settings.oven.timeout == 999);
    assert(kowhai_get_int16(&settings_tree, 3, symbols2, &timeout) == KOW_STATUS_OK);
    assert(timeout == 999);
    settings.flux_capacitor[0].gain = 0;
    assert(kowhai_set_int32(&settings_tree, 3, symbols6, 999) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[0].gain == 999);
    assert(kowhai_get_int32(&settings_tree, 3, symbols6, &gain) == KOW_STATUS_OK);
    assert(gain == 999);
    settings.flux_capacitor[0].coefficient[0] = 0;
    assert(kowhai_set_float(&settings_tree, 3, symbols7, 999.9f) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[0].coefficient[0] ==  999.9f);
    assert(kowhai_get_float(&settings_tree, 3, symbols7, &coeff) == KOW_STATUS_OK);
    assert(coeff == 999.9f);
    sprintf(settings.flux_capacitor[0].owner, "Dr brown");
    sprintf(settings.flux_capacitor[1].owner, "Marty McFly");
    assert(kowhai_set_char(&settings_tree, 3, symbols13, 'B') == KOW_STATUS_OK);
    assert(kowhai_get_char(&settings_tree, 3, symbols13, &owner_initial) == KOW_STATUS_OK);
    assert(owner_initial == 'B');
    assert(kowhai_set_int32(&settings_tree, COUNT_OF(symbols99), symbols99, 1234567890) == KOW_STATUS_OK);
    assert(settings.check == 1234567890);
    assert(kowhai_get_int32(&settings_tree, COUNT_OF(symbols99), symbols99, &check) == KOW_STATUS_OK);
    assert(check == 1234567890);

    // test get/set settings of a union branch
    assert(kowhai_set_int16(&settings_tree, COUNT_OF(symbols14), symbols14, 7337) == KOW_STATUS_OK);
    assert(settings.union_container[0].union_[0].temp == 7337);
    assert(kowhai_get_int16(&settings_tree, COUNT_OF(symbols15), symbols15, &timeout) == KOW_STATUS_OK);
    assert(timeout == 7337);
    assert(kowhai_get_int8(&settings_tree, COUNT_OF(symbols16), symbols16, &beep) == KOW_STATUS_OK);
    assert(beep == (7337 & 0xff));
    assert(kowhai_get_char(&settings_tree, COUNT_OF(symbols17), symbols17, &owner_initial) == KOW_STATUS_OK);
    assert((uint8_t)owner_initial == beep);
    assert(kowhai_set_int32(&settings_tree, COUNT_OF(symbols18), symbols18, 10) == KOW_STATUS_OK);
    assert(settings.union_container[0].check == 10);
    assert(kowhai_get_int32(&settings_tree, COUNT_OF(symbols18), symbols18, &check) == KOW_STATUS_OK);
    assert(check == 10);
    assert(kowhai_set_int32(&settings_tree, COUNT_OF(symbols19), symbols19, 20) == KOW_STATUS_OK);
    assert(settings.union_container[1].check == 20);
    assert(kowhai_get_int32(&settings_tree, COUNT_OF(symbols19), symbols19, &check) == KOW_STATUS_OK);
    assert(check == 20);
    printf(" passed!\n");
}

char* get_symbol_name(void* param, uint16_t symbol)
{
    return symbols[symbol];
}

int get_symbol_index(void *param, const char *symbol, int len)
{
    int i;
    for (i = 0; i < COUNT_OF(symbols); i++)
    {
        if (strncmp(symbols[i], symbol, len) == 0 && strlen(symbols[i]) == len)
            return i;
    }
    return -1;
}

int ignore_missing_nodes(void* param, union kowhai_symbol_t *path, int path_len)
{
    return 0;
}

void serialization_tests()
{
#define BUF_SIZE 0x3000
    int buf_size = BUF_SIZE;
    int desc_size = BUF_SIZE;
    int data_size = BUF_SIZE;
    char* js = (char*)malloc(BUF_SIZE);
    char* badjs = (char*)malloc(BUF_SIZE);
    char* scratch = (char*)malloc(BUF_SIZE);
    struct kowhai_node_t* desc = (struct kowhai_node_t*)malloc(BUF_SIZE);
    char* data = (char*)malloc(BUF_SIZE);
    union kowhai_symbol_t path[32];
    int n;

    printf("test kowhai_serialize/kowhai_deserialize...\n");

    // kowhai_serialize (tree)
    assert(js != NULL && scratch != NULL && desc != NULL && data != NULL);
    buf_size = 100;
    assert(kowhai_serialize_tree(settings_tree, js, &buf_size, NULL, get_symbol_name) == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    buf_size = BUF_SIZE;
    assert(kowhai_serialize_tree(settings_tree, js, &buf_size, NULL, get_symbol_name) == KOW_STATUS_OK);
    printf("---\n%s\n***\n", js);
    printf("js length: %d\n", (int)strlen(js));
    printf("---\n");
    
    // kowhai_deserialize (tree)
    buf_size = BUF_SIZE;
    desc_size = 10;
    data_size = 10;
    assert(kowhai_deserialize_tree(js, scratch, 100, desc, &desc_size, data, &data_size) == KOW_STATUS_SCRATCH_TOO_SMALL);
    assert(kowhai_deserialize_tree(js, scratch, buf_size, desc, &desc_size, data, &data_size) == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    desc_size = buf_size / sizeof(struct kowhai_node_t);
    assert(kowhai_deserialize_tree(js, scratch, buf_size, desc, &desc_size, data, &data_size) == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    data_size = buf_size;
    assert(kowhai_deserialize_tree(js, scratch, buf_size, desc, &desc_size, data, &data_size) == KOW_STATUS_OK);
    assert(desc_size == sizeof(settings_descriptor) / sizeof(struct kowhai_node_t));
    assert(memcmp(desc, settings_descriptor, sizeof(settings_descriptor)) == 0);
    assert(data_size == sizeof(settings));
    assert(memcmp(data, &settings, sizeof(settings)) == 0);

    // kowhai serialize (nodes)
    settings.flux_capacitor[0].coefficient[1] = FLT_MIN; // test really small values can be deserialised without loosing precision
    settings.flux_capacitor[0].coefficient[2] = 1.0f + FLT_EPSILON;
    settings.flux_capacitor[0].coefficient[3] = 0.33f;
    buf_size = 10; // test dst buffer too small
    assert(kowhai_serialize_nodes(js, &buf_size, &settings_tree, path, COUNT_OF(path), NULL, get_symbol_name) == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    buf_size = BUF_SIZE; // test path too small
    assert(kowhai_serialize_nodes(js, &buf_size, &settings_tree, path, 3, NULL, get_symbol_name) == KOW_STATUS_PATH_TOO_SMALL);
    assert(kowhai_serialize_nodes(js, &buf_size, &settings_tree, path, COUNT_OF(path), NULL, get_symbol_name) == KOW_STATUS_OK);
    printf("---\n%s\n***\n", js);
    printf("js length: %d\n", (int)strlen(js));
    printf("---\n");

    // kowhai deserialize (nodes)
    n = sprintf(badjs, "{\"path\": \"Settings.Oven.Gain\", \"type\": 114, \"count\": 1, \"tag\": 0, \"value\": 999}\n"); // test a path that does not exist in the dst_tree
    assert(kowhai_deserialize_nodes(badjs, n, &settings_tree, path, COUNT_OF(path), scratch, BUF_SIZE, NULL, get_symbol_index, NULL, NULL) == KOW_STATUS_INVALID_SYMBOL_PATH);
    n = sprintf(badjs, "{\"path\": \"Settings.Oven.Gain\", \"type\": 114, \"count\": 1, \"tag\": 0, \"value\": 999}\n"); // test a path that does not exist in the dst_tree, but ignore the error
    assert(kowhai_deserialize_nodes(badjs, n, &settings_tree, path, COUNT_OF(path), scratch, BUF_SIZE, NULL, get_symbol_index, NULL, ignore_missing_nodes) == KOW_STATUS_OK);
    n = sprintf(badjs, "{\"path\": \"Settings.Oven.Gain[5\", \"type\": 114, \"count\": 1, \"tag\": 0, \"value\": 999}\n"); // test a mangled path
    assert(kowhai_deserialize_nodes(badjs, n, &settings_tree, path, COUNT_OF(path), scratch, BUF_SIZE, NULL, get_symbol_index, NULL, NULL) == KOW_STATUS_INVALID_SYMBOL_PATH);
    n = sprintf(badjs, "{\"path\": \"Settings.Oven.Moo\", \"type\": 114, \"count\": 1, \"tag\": 0, \"value\": 999}\n"); // test a symbol that we dont know
    assert(kowhai_deserialize_nodes(badjs, n, &settings_tree, path, COUNT_OF(path), scratch, BUF_SIZE, NULL, get_symbol_index, NULL, NULL) == KOW_STATUS_NOT_FOUND);
    assert(kowhai_deserialize_nodes(js, buf_size, &settings_tree, path, 3, scratch, BUF_SIZE, NULL, get_symbol_index, NULL, NULL) == KOW_STATUS_PATH_TOO_SMALL);
    assert(kowhai_deserialize_nodes(js, buf_size, &settings_tree, path, COUNT_OF(path), scratch, 1, NULL, get_symbol_index, NULL, NULL) == KOW_STATUS_SCRATCH_TOO_SMALL);
    settings2 = settings;
    memset(&settings, 0, sizeof(settings));
    assert(kowhai_deserialize_nodes(js, buf_size, &settings_tree, path, COUNT_OF(path), scratch, BUF_SIZE, NULL, get_symbol_index, NULL, NULL) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[0].frequency == settings2.flux_capacitor[0].frequency);
    assert(settings.flux_capacitor[0].coefficient[1] == FLT_MIN); // test really small values can be de-serialised without loosing precision
    assert(settings.flux_capacitor[0].coefficient[2] != 1.0f);
    assert(settings.flux_capacitor[0].coefficient[2] == 1.0f + FLT_EPSILON);
    assert(settings.flux_capacitor[0].coefficient[3] * 3.0f == 3.0f * 0.33f);
    assert(settings.flux_capacitor[FLUX_CAP_COUNT - 1].coefficient[COEFF_COUNT - 1] == settings2.flux_capacitor[FLUX_CAP_COUNT - 1].coefficient[COEFF_COUNT - 1]);
    assert(settings.union_container[UNION_COUNT - 1].union_[UNION_COUNT - 1].beep == settings2.union_container[UNION_COUNT - 1].union_[UNION_COUNT - 1].beep);
    assert(memcmp(settings.union_container[UNION_COUNT - 1].union_[UNION_COUNT - 1].owner, settings2.union_container[UNION_COUNT - 1].union_[UNION_COUNT - 1].owner, OWNER_MAX_LEN) == 0);
    assert(settings.check == settings2.check);

    printf(" passed!\n");
}

#define DIFF_NONE           0
#define DIFF_LEFT_UNIQUE    1
#define DIFF_RIGHT_UNIQUE   2
#define DIFF_DATA           3
int diff_status;
void* diff_address;
int _on_diff(void* param, const struct kowhai_node_t *left_node, void *left_data, const struct kowhai_node_t *right_node, void *right_data, int index, int depth)
{
    if (left_node == NULL)
    {
        printf("    _on_diff: DIFF_RIGHT_UNIQUE\n");
        diff_status = DIFF_RIGHT_UNIQUE;
    }
    else if (right_node == NULL)
    {
        printf("    _on_diff: DIFF_LEFT_UNIQUE\n");
        diff_status = DIFF_LEFT_UNIQUE;
    }
    else
    {
        printf("    _on_diff: DIFF_DATA\n");
        diff_status = DIFF_DATA;
        diff_address = right_data;
    }
    return KOW_STATUS_OK;
}

void diff_tests()
{
    struct kowhai_node_t test_descriptor[] =
    {
        { KOW_BRANCH_START,     SYM_SETTINGS,       1,                0 },

        { KOW_BRANCH_U_START,   SYM_FLUXCAPACITOR,  FLUX_CAP_COUNT,   0 },
        { KOW_UINT32,           SYM_FREQUENCY,      1,                0 },
        { KOW_UINT32,           SYM_GAIN,           1,                0 },
        { KOW_FLOAT,            SYM_COEFFICIENT,    COEFF_COUNT,      0 },
        { KOW_BRANCH_END,       SYM_FLUXCAPACITOR,  0,                0 },

        { KOW_BRANCH_START,     SYM_OVEN,           1,                0 },
        { KOW_INT16,            SYM_TEMP,           1,                0 },
        { KOW_UINT16,           SYM_TIMEOUT,        1,                0 },
        { KOW_BRANCH_END,       SYM_OVEN,           0,                0 },

        { KOW_BRANCH_END,       SYM_SETTINGS,       1,                0 },
    };
    struct test_settings_t
    {
        union
        {
            uint32_t freq;
            uint32_t gain;
            float coeff[COEFF_COUNT];
        } fluxcap[FLUX_CAP_COUNT];
        struct
        {
            int16_t temp;
            uint16_t timeout;
        } oven;
    };
    
    struct test_settings_t settings1 = { 1, 2, 3, 4};
    struct test_settings_t settings2;

    struct kowhai_tree_t tree_left, tree_right;

    printf("kowhai_diff tests!\n");

    // init trees
    tree_left.desc = test_descriptor;
    tree_left.data = &settings1;
    tree_right.desc = test_descriptor;
    tree_right.data = &settings2;

    // tests
    memcpy(&settings2, &settings1, sizeof(struct test_settings_t));
    assert(memcmp(&settings2, &settings1, sizeof(struct test_settings_t)) == 0);
    diff_status = DIFF_NONE;
    kowhai_diff(&tree_left, &tree_right, NULL, _on_diff);
    assert(diff_status == DIFF_NONE);
    assert(memcmp(&settings2, &settings1, sizeof(struct test_settings_t)) == 0);
    settings1.oven.temp++;
    kowhai_diff(&tree_left, &tree_right, NULL, _on_diff);
    assert(diff_status == DIFF_DATA);
    assert(diff_address == &settings2.oven.temp);
    settings1.oven.temp--;
    diff_status = DIFF_NONE;
    settings1.fluxcap[1].coeff[3]++;
    kowhai_diff(&tree_left, &tree_right, NULL, _on_diff);
    assert(diff_status == DIFF_DATA);
    assert(diff_address == &settings2.fluxcap[1].coeff[3]);

    printf(" passed!\n");
}

void merge_tests(void)
{
    #pragma pack(1)

    struct kowhai_node_t old_settings_descriptor[] =
    {
        { KOW_BRANCH_START,     SYM_SETTINGS,       1,                0 },

        { KOW_BRANCH_START,     SYM_FLUXCAPACITOR,  FLUX_CAP_COUNT,   0 },
        { KOW_UINT32,           SYM_FREQUENCY,      1,                0 },
        { KOW_UINT32,           SYM_GAIN,           1,                0 },
        { KOW_FLOAT,            SYM_COEFFICIENT,    COEFF_COUNT,      0 },
        { KOW_BRANCH_END,       SYM_FLUXCAPACITOR,  0,                0 },

        { KOW_BRANCH_START,     SYM_OVEN,           1,                0 },
        { KOW_INT16,            SYM_TEMP,           1,                0 },
        { KOW_UINT16,           SYM_TIMEOUT,        1,                0 },
        { KOW_BRANCH_END,       SYM_OVEN,           0,                0 },

        { KOW_BRANCH_END,       SYM_SETTINGS,       1,                0 },
    };
    
    struct old_settings_data_t
    {
        struct flux_capacitor_t
        {
            uint32_t frequency;
            uint32_t gain;
            float coefficient[COEFF_COUNT];
        } flux_capacitor [FLUX_CAP_COUNT];

        struct oven_t
        {
            int16_t temp;
            uint16_t timeout;
        } oven;
    };

    struct old_settings_data_t old_settings =
    {
        // flux capacitor array
        {
            { 1000, 200, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f} },
            { 3000, 400, {0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f} }
        },
        // oven
        { 180, 30 }
    };

    struct kowhai_node_t new_settings_descriptor[] =
    {
        { KOW_BRANCH_START,     SYM_SETTINGS,       1,                0 },

        { KOW_BRANCH_START,     SYM_FLUXCAPACITOR,  FLUX_CAP_COUNT,   0 },
        { KOW_UINT32,           SYM_FREQUENCY,      1,                0 },
        { KOW_UINT8,            SYM_RUNNING,        1,                0 },
        { KOW_FLOAT,            SYM_COEFFICIENT,    COEFF_COUNT,      0 },
        { KOW_BRANCH_END,       SYM_FLUXCAPACITOR,  0,                0 },

        { KOW_BRANCH_START,     SYM_OVEN,           1,                0 },
        { KOW_INT16,            SYM_TEMP,           1,                0 },
        { KOW_INT32,            SYM_BEEP,           2,                0 },
        { KOW_UINT16,           SYM_TIMEOUT,        1,                0 },
        { KOW_BRANCH_END,       SYM_OVEN,           0,                0 },

        { KOW_BRANCH_END,       SYM_SETTINGS,       1,                0 },
    };

    struct new_settings_data_t
    {
        struct new_flux_capacitor_t
        {
            uint32_t frequency;
            uint8_t running;
            float coefficient[COEFF_COUNT];
        } flux_capacitor [FLUX_CAP_COUNT];

        struct new_oven_t
        {
            int16_t temp;
            uint32_t beep[2];
            uint16_t timeout;
        } oven;
    };

    struct new_settings_data_t new_settings =
    {
        // flux capacitor array
        {
            { 5000, 0, {-0.1f, -0.2f, -0.3f, -0.4f, -0.5f, -0.6f} },
            { 6000, 1, {-0.6f, -0.5f, -0.4f, -0.3f, -0.2f, -0.1f} }
        },
        // oven
        { 220, {10000, 5000}, 20 }
    };

    struct kowhai_tree_t old_settings_tree = {old_settings_descriptor, &old_settings};
    struct kowhai_tree_t new_settings_tree = {new_settings_descriptor, &new_settings};

    int cap;
    int coeff;

    #pragma pack()

    printf("test kowhai kowhai_merge...\t");

    // test the merge of old into new
    assert(kowhai_merge(&new_settings_tree, &old_settings_tree) == KOW_STATUS_OK);

    // test things that should not have been merged are untouched
    assert(new_settings.flux_capacitor[0].running == 0);
    assert(new_settings.flux_capacitor[1].running == 1);
    assert(new_settings.oven.beep[0] == 10000);
    assert(new_settings.oven.beep[1] == 5000);

    // check that things that should be merged are
    for (cap = 0; cap < FLUX_CAP_COUNT; cap++)
    {
        assert(new_settings.flux_capacitor[cap].frequency == old_settings.flux_capacitor[cap].frequency);
        for (coeff = 0; coeff < COEFF_COUNT; coeff++)
            assert(new_settings.flux_capacitor[cap].coefficient[coeff] == old_settings.flux_capacitor[cap].coefficient[coeff]);
    }
    assert(new_settings.oven.temp == old_settings.oven.temp);
    assert(new_settings.oven.timeout == old_settings.oven.timeout);
    
    printf(" passed!\n");
}

void create_symbol_path_tests()
{
#define SYM_PATH_SIZE 5
#define SYM_PATH_SIZE_RESULT 3
    union kowhai_symbol_t symbol_path[SYM_PATH_SIZE] = {0};
    union kowhai_symbol_t symbol_path_settings_oven_temp[SYM_PATH_SIZE] = {SYM_SETTINGS, SYM_OVEN, SYM_TEMP};
    union kowhai_symbol_t symbol_path_settings_fluxcap_gain[SYM_PATH_SIZE] = {SYM_SETTINGS, SYM_FLUXCAPACITOR, SYM_GAIN};
    int symbol_path_size;
    int result;

    printf("kowhai_create_symbol_path* tests!\n");

    symbol_path_size = 2;
    result = kowhai_create_symbol_path(settings_descriptor, &settings_descriptor[8] /* settings.oven.temp */, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    symbol_path_size = SYM_PATH_SIZE;
    result = kowhai_create_symbol_path(settings_descriptor, &settings_descriptor[8] /* settings.oven.temp */, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_oven_temp, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);
    symbol_path_size = SYM_PATH_SIZE;
    result = kowhai_create_symbol_path(settings_descriptor, &settings_descriptor[4] /* settings.fluxcap.gain */, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_fluxcap_gain, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);

    symbol_path_size = 2;
    result = kowhai_create_symbol_path2(&settings_tree, &settings.oven.temp, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    symbol_path_size = SYM_PATH_SIZE;
    result = kowhai_create_symbol_path2(&settings_tree, &settings.oven.temp, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_oven_temp, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);
    symbol_path_size = SYM_PATH_SIZE;
    symbol_path_settings_fluxcap_gain[1].symbol = KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 1);
    result = kowhai_create_symbol_path2(&settings_tree, &settings.flux_capacitor[1].gain, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_fluxcap_gain, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);
    symbol_path_size = SYM_PATH_SIZE;
    symbol_path_settings_fluxcap_gain[2].symbol = KOWHAI_SYMBOL(SYM_COEFFICIENT, 4);
    result = kowhai_create_symbol_path2(&settings_tree, &settings.flux_capacitor[1].coefficient[4], symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_fluxcap_gain, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);

    printf(" passed!\n");
}

void node_pre_write(pkowhai_protocol_server_t server, void* param, uint16_t tree_id, struct kowhai_node_t* node, int offset)
{
    printf("node_pre_write: tree_id: %d, node: %p, offset: %d\n", tree_id, node, offset);
}

void node_post_write(pkowhai_protocol_server_t server, void* param, uint16_t tree_id, struct kowhai_node_t* node, int offset, int bytes_written)
{
    printf("node_post_write: tree_id: %d, node: %p, offset: %d, bytes_written: %d\n", tree_id, node, offset, bytes_written);
}

void server_buffer_send(pkowhai_protocol_server_t server, void* param, void* buffer, size_t buffer_size, struct kowhai_protocol_t* protocol)
{
    xpsocket_handle conn = (xpsocket_handle)param;
    xpsocket_send(conn, buffer, buffer_size);
}

uint32_t unsolicited_mode_start;
void unsolicited_event(struct timer_t* tmr, void* param)
{
    uint32_t time_delta = (uint32_t)time(NULL) - unsolicited_mode_start;
    pkowhai_protocol_server_t server = (pkowhai_protocol_server_t)param;
    kowhai_server_process_event(server, SYM_UNSOLICITEDEVENT, &time_delta, sizeof(uint32_t));
    timer_free(tmr);
}

#define STATUS_RESULT 0xff00ff00
#define BIG_COEFF_RESULT 0xff00ff00
int function_called(pkowhai_protocol_server_t server, void* param, uint16_t function_id)
{
    switch (function_id)
    {
        case SYM_START:
            printf("Function: Start (delay: %d)\n", start.delay);
            shadow.running = 1;
            shadow.status = rand();
            break;
        case SYM_STOP:
            printf("Function: Stop\n");
            shadow.running = 0;
            shadow.status = 0;
            break;
        case SYM_STATUS:
            printf("Function: Status (time: %d)\n", (int)time(NULL));
            status.status = STATUS_RESULT;
            status.time = (uint32_t)time(NULL);
            break;
        case SYM_BIG:
            printf("Function: Big (time: %d)\n", (int)time(NULL));
            big.status = STATUS_RESULT;
            big.time = (uint32_t)time(NULL);
        case SYM_BEEP:
            printf("Function: Beep (freq: %d, duration: %d)\n", beepd.freq, beepd.duration);
            beep(beepd.freq, beepd.duration);
            break;
        case SYM_FAIL:
            printf("Function: Fail\n");
            return 0;
        case SYM_UNSOLICITEDMODE:
        {
            struct timer_t* tmr;
            printf("Function: Unsolicited Mode\n");
            unsolicited_mode_start = (uint32_t)time(NULL);
            tmr = timer_create_(2000, unsolicited_event, server);
            timer_one_shot(tmr);
        }
    }
    return 1;
}

void server_buffer_received(xpsocket_handle conn, void* param, void* buffer, int buffer_size)
{
    int i;
    struct kowhai_protocol_server_t* server = (struct kowhai_protocol_server_t*)param;
    server->send_packet_param = conn;

    // randomize the scope buffer for funzies
    for (i = 0; i < NUM_PIXELS; i++)
        scope.pixels[i] = rand();

    kowhai_server_process_packet(server, buffer, buffer_size);
}

void test_server_protocol()
{
    char packet_buffer[MAX_PACKET_SIZE];
    struct kowhai_protocol_server_t server;
    kowhai_server_init(&server,
        MAX_PACKET_SIZE,
        packet_buffer,
        node_pre_write,
        node_post_write,
        NULL,
        server_buffer_send,
        NULL,
        COUNT_OF(tree_list),
        tree_list,
        tree_id_list,
        COUNT_OF(function_list),
        function_list,
        function_id_list,
        function_called,
        NULL,
        COUNT_OF(symbols),
        symbols);
    printf("test server protocol...\n");
    xpsocket_init();
    xpsocket_serve(server_buffer_received, &server, MAX_PACKET_SIZE);
    xpsocket_cleanup();

}

int _compare_string_arrays(char** arr1, char** arr2, int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (strlen(arr1[i]) != strlen(arr2[i]))
            return 0;
        if (memcmp(arr1[i], arr2[i], strlen(arr1[i])) != 0)
            return 0;
    }
    return 1;
}

void test_client_protocol()
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
        int offset, size, temp;
        char value;
        struct oven_t oven = {0x0102, 321};
        struct flux_capacitor_t flux_cap[2] = {{{"Marty Mc Fly"}, 100, 200, {1, 2, 3, 4, 5, 6}}, {{"Dr Brown"}, 110, 210, {11, 12, 13, 14, 15, 16}}};

        // get protocol version
        POPULATE_PROTOCOL_CMD(prot, KOW_CMD_GET_VERSION, 0);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_GET_VERSION_ACK);
        assert(prot.header.id == 0);
        assert(prot.payload.spec.version == kowhai_version());

        // get tree list
        POPULATE_PROTOCOL_GET_TREE_LIST(prot);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_GET_TREE_LIST_ACK_END);
        assert(prot.header.id == 0);
        assert(prot.payload.spec.id_list.list_count == COUNT_OF(tree_list));
        assert(prot.payload.spec.id_list.offset == 0);
        assert(prot.payload.spec.id_list.size == sizeof(tree_list));
        assert(memcmp(prot.payload.buffer, tree_list, sizeof(tree_list)) == 0);

        // write oven.temp
        temp = 25;
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA_END, SYM_SETTINGS, 3, symbols1, KOW_INT16, 0, sizeof(uint16_t), &temp);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 3);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(union kowhai_symbol_t) * 3) == 0);
        assert(prot.payload.spec.data.memory.type == KOW_INT16);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == sizeof(uint16_t));
        assert(*((uint16_t*)prot.payload.buffer) == temp);
        // write low byte of oven.temp
        value = 255;
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA_END, SYM_SETTINGS, 3, symbols1, KOW_INT16, 1, 1, &value);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 3);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(union kowhai_symbol_t) * 3) == 0);
        assert(prot.payload.spec.data.memory.type == KOW_INT16);
        assert(prot.payload.spec.data.memory.offset == 1);
        assert(prot.payload.spec.data.memory.size == 1);
        assert(*((char*)prot.payload.buffer) == value);
        // double check oven.temp
        POPULATE_PROTOCOL_READ(prot, KOW_CMD_READ_DATA, SYM_SETTINGS, 3, symbols1);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(*((uint16_t*)prot.payload.buffer) >> 8 == (uint8_t)value);
        // write oven
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA_END, SYM_SETTINGS, 2, symbols11, 0, 0, sizeof(oven), &oven);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols11, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == sizeof(oven));
        assert(memcmp(prot.payload.buffer, &oven, sizeof(oven)) == 0);
        // write flux capacitor array
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA, SYM_SETTINGS, 2, symbols3, 0, 0, sizeof(struct flux_capacitor_t) * 2, flux_cap);
        kowhai_protocol_get_overhead(&prot, &overhead);
        prot.payload.spec.data.memory.size = MAX_PACKET_SIZE - overhead;
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == MAX_PACKET_SIZE - overhead);
        assert(memcmp(prot.payload.buffer, flux_cap, MAX_PACKET_SIZE - overhead) == 0);
        offset = prot.payload.spec.data.memory.size;
        size = sizeof(struct flux_capacitor_t) * 2 - offset;
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA_END, SYM_SETTINGS, 2, symbols3, 0, offset, size, (char*)flux_cap + offset);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.memory.offset == offset);
        assert(prot.payload.spec.data.memory.size == size);
        assert(memcmp(prot.payload.buffer, (char*)flux_cap + offset, size) == 0);
        // write flux capacitor[1]
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA_END, SYM_SETTINGS, 2, symbols12, 0, 0, sizeof(struct flux_capacitor_t), &flux_cap[1]);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols12, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == sizeof(struct flux_capacitor_t));
        assert(memcmp(prot.payload.buffer, &flux_cap[1], sizeof(struct flux_capacitor_t)) == 0);
        // read oven.temp
        POPULATE_PROTOCOL_READ(prot, KOW_CMD_READ_DATA, SYM_SETTINGS, 3, symbols1);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DATA_ACK_END);
        assert(prot.payload.spec.data.symbols.count == 3);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(union kowhai_symbol_t) * 3) == 0);
        assert(prot.payload.spec.data.memory.type == KOW_INT16);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == sizeof(int16_t));
        assert(*((int16_t*)prot.payload.buffer) == 0x0102);
        // read flux capacitor array
        POPULATE_PROTOCOL_READ(prot, KOW_CMD_READ_DATA, SYM_SETTINGS, 2, symbols3);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == 0);
        kowhai_protocol_get_overhead(&prot, &overhead);
        assert(overhead == 18);
        assert(prot.payload.spec.data.memory.size == MAX_PACKET_SIZE - overhead);
        assert(memcmp(prot.payload.buffer, flux_cap, prot.payload.spec.data.memory.size) == 0);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DATA_ACK_END);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == MAX_PACKET_SIZE - overhead);
        assert(prot.payload.spec.data.memory.size == sizeof(struct flux_capacitor_t) * 2 - prot.payload.spec.data.memory.offset);
        assert(memcmp(prot.payload.buffer, (char*)flux_cap + prot.payload.spec.data.memory.offset, prot.payload.spec.data.memory.size) == 0);
        // read tree with no data
        POPULATE_PROTOCOL_READ(prot, KOW_CMD_READ_DATA, SYM_UNSOLICITEDEVENT, 2, symbols3);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_ERROR_NO_DATA);
        assert(prot.header.id == SYM_UNSOLICITEDEVENT);
        // read settings tree descriptor
        POPULATE_PROTOCOL_CMD(prot, KOW_CMD_READ_DESCRIPTOR, SYM_SETTINGS);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        // get packet 1
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        assert(kowhai_protocol_parse(buffer, received_size, &prot) == KOW_STATUS_OK);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DESCRIPTOR_ACK);
        assert(prot.payload.spec.descriptor.node_count == COUNT_OF(settings_descriptor));
        assert(prot.payload.spec.descriptor.offset == 0);
        kowhai_protocol_get_overhead(&prot, &overhead);
        assert(overhead == 9);
        assert(prot.payload.spec.descriptor.node_count == COUNT_OF(settings_descriptor));
        assert(prot.payload.spec.descriptor.size == MAX_PACKET_SIZE - overhead);
        assert(memcmp(prot.payload.buffer, settings_descriptor, prot.payload.spec.data.memory.size) == 0);
        // get packet 2
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        assert(kowhai_protocol_parse(buffer, received_size, &prot) == KOW_STATUS_OK);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DESCRIPTOR_ACK);
        assert(prot.payload.spec.descriptor.offset == MAX_PACKET_SIZE - overhead);
        assert(prot.payload.spec.descriptor.size == MAX_PACKET_SIZE - overhead);
        assert(memcmp(prot.payload.buffer, (char*)settings_descriptor + prot.payload.spec.data.memory.offset, prot.payload.spec.data.memory.size) == 0);
        // get packet 3
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        assert(kowhai_protocol_parse(buffer, received_size, &prot) == KOW_STATUS_OK);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DESCRIPTOR_ACK);
        assert(prot.payload.spec.descriptor.offset == (MAX_PACKET_SIZE - overhead) * 2);
        assert(prot.payload.spec.descriptor.size == MAX_PACKET_SIZE - overhead);
        assert(memcmp(prot.payload.buffer, (char*)settings_descriptor + prot.payload.spec.data.memory.offset, prot.payload.spec.data.memory.size) == 0);
        // get packet 4
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        assert(kowhai_protocol_parse(buffer, received_size, &prot) == KOW_STATUS_OK);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DESCRIPTOR_ACK_END);
        assert(prot.payload.spec.descriptor.offset == (MAX_PACKET_SIZE - overhead) * 3);
        assert(prot.payload.spec.descriptor.size == sizeof(settings_descriptor) - prot.payload.spec.descriptor.offset);
        assert(memcmp(prot.payload.buffer, (char*)settings_descriptor + prot.payload.spec.data.memory.offset, prot.payload.spec.data.memory.size) == 0);
        // test invalid tree id
        POPULATE_PROTOCOL_CMD(prot, KOW_CMD_READ_DESCRIPTOR, 255);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == 255);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_TREE_ID);
        // test invalid command
        POPULATE_PROTOCOL_CMD(prot, 255, SYM_SETTINGS);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_INVALID_PROTOCOL_COMMAND);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_COMMAND);
        // test invalid symbol path
        POPULATE_PROTOCOL_READ(prot, KOW_CMD_READ_DATA, SYM_SETTINGS, 2, symbols4);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_SYMBOL_PATH);
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA, SYM_SETTINGS, 2, symbols4, KOW_INT16, 0, sizeof(uint16_t), &temp);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_SYMBOL_PATH);
        // test invalid write sequence (write flux capacitor array)
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA, SYM_SETTINGS, 2, symbols3, 0, 0, 8, flux_cap);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        POPULATE_PROTOCOL_WRITE(prot, KOW_CMD_WRITE_DATA, SYM_SETTINGS, 2, symbols2, 0, 0, 8, flux_cap);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.id == SYM_SETTINGS);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_SEQUENCE);

        // test get function list
        POPULATE_PROTOCOL_GET_FUNCTION_LIST(prot);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_GET_FUNCTION_LIST_ACK_END);
        assert(prot.header.id == 0);
        assert(prot.payload.spec.id_list.list_count == COUNT_OF(function_list));
        assert(prot.payload.spec.id_list.offset == 0);
        assert(prot.payload.spec.id_list.size == sizeof(function_list));
        assert(memcmp(prot.payload.buffer, function_list, sizeof(function_list)) == 0);

        // test get function details
        POPULATE_PROTOCOL_GET_FUNCTION_DETAILS(prot, SYM_BEEP);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_GET_FUNCTION_DETAILS_ACK);
        assert(prot.header.id == SYM_BEEP);
        assert(prot.payload.spec.function_details.tree_in_id == SYM_BEEP);
        assert(prot.payload.spec.function_details.tree_out_id == KOW_UNDEFINED_SYMBOL);

        // test call function
        POPULATE_PROTOCOL_CALL_FUNCTION(prot, 999, 0, 0, NULL);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_FUNCTION_ID);

        POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_START, 999, sizeof(start), &start);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_PAYLOAD_OFFSET);

        POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_START, 0, 30, &start);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_PAYLOAD_SIZE);

        start.delay = 50;
        POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_START, 0, sizeof(start), &start);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_CALL_FUNCTION_RESULT_END);
        assert(prot.header.id == SYM_START);
        assert(prot.payload.spec.function_call.offset == 0);
        assert(prot.payload.spec.function_call.size == 0);

        POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_STOP, 0, 0, NULL);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_CALL_FUNCTION_RESULT_END);
        assert(prot.header.id == SYM_STOP);
        assert(prot.payload.spec.function_call.offset == 0);
        assert(prot.payload.spec.function_call.size == 0);

        beepd.duration = 250;
        beepd.freq = 1000;
        POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_BEEP, 0, sizeof(beepd), &beepd);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_CALL_FUNCTION_RESULT_END);
        assert(prot.header.id == SYM_BEEP);
        assert(prot.payload.spec.function_call.offset == 0);
        assert(prot.payload.spec.function_call.size == 0);

        POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_STATUS, 0, 0, NULL);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_CALL_FUNCTION_RESULT_END);
        assert(prot.header.id == SYM_STATUS);
        assert(prot.payload.spec.function_call.offset == 0);
        assert(prot.payload.spec.function_call.size == sizeof(status));
        {
            struct status_data_t s = {STATUS_RESULT, (uint32_t)time(NULL)};
            assert(memcmp(prot.payload.buffer, &s, sizeof(s)) == 0);
        }

        {
            struct big_data_t b;
            void* data = &big;
            int bytes_written = 0;
            int bytes_to_write;
            big.coeff[BIG_COEFF_COUNT-1] = BIG_COEFF_RESULT;
            prot.header.command = KOW_CMD_CALL_FUNCTION;
            assert(kowhai_protocol_get_overhead(&prot, &overhead) == KOW_STATUS_OK);
            assert(overhead == 7);
        
            POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_BIG, 0,  MAX_PACKET_SIZE - overhead, data);
            while (bytes_written < sizeof(big))
            {
                int prev_bytes_written = bytes_written;
                assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
                xpsocket_send(conn, buffer, bytes_required);
                bytes_written += prot.payload.spec.function_call.size;

                memset(buffer, 0, MAX_PACKET_SIZE);
                xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
                if (bytes_written < sizeof(big))
                {
                    kowhai_protocol_parse(buffer, received_size, &prot);
                    assert(prot.header.command == KOW_CMD_CALL_FUNCTION_ACK);
                    assert(prot.header.id == SYM_BIG);
                    assert(prot.payload.spec.function_call.offset == 0);
                    assert(prot.payload.spec.function_call.size == 0);
                }
                else
                {
                    int bytes_read = 0;
                    int offset, size;
                    while (bytes_read < sizeof(big))
                    {
                        kowhai_protocol_parse(buffer, received_size, &prot);
                        offset = prot.payload.spec.function_call.offset;
                        size = prot.payload.spec.function_call.size;
                        if (bytes_read + size == sizeof(big))
                        {
                            assert(prot.header.command == KOW_CMD_CALL_FUNCTION_RESULT_END);
                            assert(size == sizeof(big) - bytes_read);
                        }
                        else
                        {
                            assert(prot.header.command == KOW_CMD_CALL_FUNCTION_RESULT);
                            assert(size == MAX_PACKET_SIZE - overhead);
                        }
                        assert(prot.header.id == SYM_BIG);
                        assert(offset == bytes_read);
                        memcpy((char*)&b + offset, prot.payload.buffer, size);
                        bytes_read += size;
                        if (prot.header.command == KOW_CMD_CALL_FUNCTION_RESULT_END)
                            break;
                        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
                    }
                    assert(bytes_read == sizeof(big));
                    assert(b.coeff[BIG_COEFF_COUNT-1] == BIG_COEFF_RESULT);
                }
                bytes_to_write = MAX_PACKET_SIZE - overhead;
                if (bytes_to_write > sizeof(big) - bytes_written)
                    bytes_to_write = sizeof(big) - bytes_written;
                POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_BIG, bytes_written, bytes_to_write, (char*)data + bytes_written);
            }
            assert(bytes_written == sizeof(big));
        }

        POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_FAIL, 0, 0, NULL);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_CALL_FUNCTION_FAILED);

        // test events
        {
            uint32_t seconds;
            POPULATE_PROTOCOL_CALL_FUNCTION(prot, SYM_UNSOLICITEDMODE, 0, 0, NULL);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.command == KOW_CMD_CALL_FUNCTION_RESULT_END);
            assert(prot.header.id == SYM_UNSOLICITEDMODE);

            seconds = (uint32_t)time(NULL);
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            seconds = (uint32_t)time(NULL) - seconds;

            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.command == KOW_CMD_EVENT_END);
            assert(prot.header.id == SYM_UNSOLICITEDEVENT);
            assert(prot.payload.spec.function_call.offset == 0);
            assert(prot.payload.spec.function_call.size == sizeof(uint32_t));
            assert(memcmp(prot.payload.buffer, &seconds, sizeof(uint32_t)) == 0);
        }

        // test get symbol list
        {
            char** symbols2;
            char* symbols2_buf;
            int sym_idx, sym_offset;
            POPULATE_PROTOCOL_GET_SYMBOL_LIST(prot);
            assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
            xpsocket_send(conn, buffer, bytes_required);
            // get packet 1
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.command == KOW_CMD_GET_SYMBOL_LIST_ACK);
            assert(prot.header.id == 0);
            assert(prot.payload.spec.string_list.list_count == COUNT_OF(symbols));
            symbols2 = (char**)malloc(prot.payload.spec.string_list.list_count * sizeof(char*));
            assert(prot.payload.spec.string_list.list_total_size == _get_string_list_size(symbols, COUNT_OF(symbols)));
            symbols2_buf = (char*)malloc(prot.payload.spec.string_list.list_total_size);
            assert(prot.payload.spec.string_list.offset == 0);
            assert(prot.payload.spec.string_list.size == 51);
            memcpy(symbols2_buf + prot.payload.spec.string_list.offset, prot.payload.buffer, prot.payload.spec.string_list.size);
            // get packet 2
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.command == KOW_CMD_GET_SYMBOL_LIST_ACK);
            assert(prot.header.id == 0);
            assert(prot.payload.spec.string_list.list_count == COUNT_OF(symbols));
            assert(prot.payload.spec.string_list.list_total_size == _get_string_list_size(symbols, COUNT_OF(symbols)));
            assert(prot.payload.spec.string_list.offset == 51);
            assert(prot.payload.spec.string_list.size == 51);
            memcpy(symbols2_buf + prot.payload.spec.string_list.offset, prot.payload.buffer, prot.payload.spec.string_list.size);
            // get packet 3
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.command == KOW_CMD_GET_SYMBOL_LIST_ACK);
            assert(prot.header.id == 0);
            assert(prot.payload.spec.string_list.list_count == COUNT_OF(symbols));
            assert(prot.payload.spec.string_list.list_total_size == _get_string_list_size(symbols, COUNT_OF(symbols)));
            assert(prot.payload.spec.string_list.offset == 102);
            assert(prot.payload.spec.string_list.size == 51);
            memcpy(symbols2_buf + prot.payload.spec.string_list.offset, prot.payload.buffer, prot.payload.spec.string_list.size);
            // get packet 4
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.command == KOW_CMD_GET_SYMBOL_LIST_ACK);
            assert(prot.header.id == 0);
            assert(prot.payload.spec.string_list.list_count == COUNT_OF(symbols));
            assert(prot.payload.spec.string_list.list_total_size == _get_string_list_size(symbols, COUNT_OF(symbols)));
            assert(prot.payload.spec.string_list.offset == 153);
            assert(prot.payload.spec.string_list.size == 51);
            memcpy(symbols2_buf + prot.payload.spec.string_list.offset, prot.payload.buffer, prot.payload.spec.string_list.size);
            // get packet 5
            memset(buffer, 0, MAX_PACKET_SIZE);
            xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
            kowhai_protocol_parse(buffer, received_size, &prot);
            assert(prot.header.command == KOW_CMD_GET_SYMBOL_LIST_ACK_END);
            assert(prot.header.id == 0);
            assert(prot.payload.spec.string_list.list_count == COUNT_OF(symbols));
            assert(prot.payload.spec.string_list.list_total_size == _get_string_list_size(symbols, COUNT_OF(symbols)));
            assert(prot.payload.spec.string_list.offset == 204);
            assert(prot.payload.spec.string_list.size == 36);
            memcpy(symbols2_buf + prot.payload.spec.string_list.offset, prot.payload.buffer, prot.payload.spec.string_list.size);
            // validate results
            sym_offset = 0;
            for (sym_idx = 0; sym_idx < COUNT_OF(symbols); sym_idx++)
            {
                symbols2[sym_idx] = symbols2_buf + sym_offset;
                sym_offset += strlen(symbols2[sym_idx]) + 1;
            }
            assert(_compare_string_arrays(symbols, symbols2, COUNT_OF(symbols)) == 1);
            free(symbols2_buf);
            free(symbols2);
        }

        xpsocket_free_client(conn);
    }
    xpsocket_cleanup();
    printf("\t\t\t\t\t passed!\n");
}

//
// main
//

int main(int argc, char* argv[])
{
    int test_command = TEST_BASIC;

    KOW_LOG("kowhai logging enabled!\n");

    // determine test command
    if (argc > 1)
    {
        if (strcmp("server", argv[1]) == 0)
            test_command = TEST_PROTOCOL_SERVER;
        else if (strcmp("client", argv[1]) == 0)
            test_command = TEST_PROTOCOL_CLIENT;
    }

    // core tests
    core_tests();
    // test serialization
    serialization_tests();
    // test utils
    diff_tests();
    merge_tests();
    create_symbol_path_tests();
    // test server protocol
    if (test_command == TEST_PROTOCOL_SERVER)
        test_server_protocol();
    // test client protocol
    if (test_command == TEST_PROTOCOL_CLIENT)
        test_client_protocol();

    return 0;
}
