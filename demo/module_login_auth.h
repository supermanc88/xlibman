//
// Created by 程贺明 on 2023/4/25.
//

#ifndef XLIBMAN_MODULE_LOGIN_AUTH_H
#define XLIBMAN_MODULE_LOGIN_AUTH_H

#include "../xlibman.h"

int login_auth_module_callback_t(_in void *in_data, _out void **out_data, _in _out int *out_len, _in _out void *module_data);

int ext_callback_t(_in void *in_data, _out void **out_data, _in _out int *out_len, _in _out void *module_data);

#endif //XLIBMAN_MODULE_LOGIN_AUTH_H
