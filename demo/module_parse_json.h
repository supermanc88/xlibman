//
// Created by 程贺明 on 2023/4/24.
//

#ifndef XLIBMAN_MODULE_PARSE_JSON_H
#define XLIBMAN_MODULE_PARSE_JSON_H

#include "../xlibman.h"


struct user_info_t {
    int version;
    char user[256];
    char announce[32];
    char session_id[64];
    char request_type[32];
    char processor[32];
};

int parse_json_module_callback_t(_in void *in_data, _out void **out_data, _in _out int *out_len, _in _out void *module_data);

#endif //XLIBMAN_MODULE_PARSE_JSON_H
