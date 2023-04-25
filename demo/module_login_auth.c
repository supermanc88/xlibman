//
// Created by 程贺明 on 2023/4/25.
//

#include "module_login_auth.h"

#include "module_parse_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include "../xlibman.h"

int ext_callback_t(_in void *in_data, _out void **out_data, _in _out int *out_len, _in _out void *module_data)
{
    int ret = 0;

    printf("enter ext callback\n");

    return ret;
}


int login_auth_module_callback_t(_in void *in_data, _out void **out_data, _in _out int *out_len, _in _out void *module_data)
{
    int ret = 0;
    struct user_info_t *user_info = (struct user_info_t *)in_data;


    if (strcmp(user_info->user, "auser") == 0) {
        printf("login auth success\n");
    }

    void *test_in_data = NULL;
    void *test_out_data = NULL;
    int test_out_len = 0;
    xlibman_hook(2, test_in_data, &test_out_data, &test_out_len);

    // 后面的插件也需要这个数据
    *out_data = user_info;
    *out_len = sizeof(struct user_info_t);

    return ret;
}