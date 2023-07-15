//
// Created by 程贺明 on 2023/4/24.
//

#include "test.h"

#include "module_parse_json.h"
#include "module_login_auth.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "../xlibman.h"


int main(int argc, char *argv[])
{
    int ret = 0;

    struct xlibman_module_t *parse_json_module = NULL;
    struct xlibman_module_t *login_auth_module = NULL;
    struct xlibman_module_t *ext_module = NULL;

    parse_json_module = (struct xlibman_module_t *)xlibman_alloc();
    login_auth_module = (struct xlibman_module_t *)xlibman_alloc();
    ext_module = (struct xlibman_module_t *)xlibman_alloc();

    parse_json_module->callback.func_ext_callback = parse_json_module_callback_t;
    login_auth_module->callback.func_ext_callback = login_auth_module_callback_t;
    ext_module->callback.func_ext_callback = ext_callback_t;

    parse_json_module->type = XLIBMAN_MODULE_TYPE1;
    login_auth_module->type = XLIBMAN_MODULE_TYPE1;
    ext_module->type = XLIBMAN_MODULE_TYPE1;

    parse_json_module->weight = 2;
    login_auth_module->weight = 1;
    ext_module->weight = 33;

    parse_json_module->position = 1;
    login_auth_module->position = 1;

    ext_module->position = 2;


    xlibman_init();


    xlibman_add_module(parse_json_module);
    xlibman_add_module(login_auth_module);
    xlibman_add_module(ext_module);

    const char *test_json_str = "{ \"Header\":{ \"Version\":1, \"User\":\"auser\", \"Announce\":\"293717623\", \"SessionID\":\"ee2a82f68416c38feb4e1971c8b35a47350bf\", \"RequestType\":\"UserManager\" }, \"Body\": { \"Processor\": \"AddUser\", \"RequestMessage\": { \"UserInfo\": { \"name\": \"xxxxxx\", \"available\": \"true\", \"authcode\": \"xxxxxx\", \"upks\": [\"upk1\", \"upk2\", \"upk3\"], \"storemode\": \"xxxxxx\", \"doublekeymode\": \"xxxxxx\", \"createtime\": \"xxxxxx\", \"expiretime\": \"xxxxxx\", \"keycountlimit\": 111, \"checkweekalg\": \"xxxxxx\", \"tokenlife\": 111, \"maxtoken\": 111, \"ontokenover\": \"xxxxxx\", \"cryptopow\": { \"hsm\": 11, \"cpu\": 11, \"remote\": 11 } } } } }";

    void *out_data = NULL;
    int out_len = 0;
    xlibman_hook(1, test_json_str, &out_data, &out_len);


    struct user_info_t *user_info = (struct user_info_t *)out_data;
    printf("user_info->version: %d\n", user_info->version);
    printf("user_info->user: %s\n", user_info->user);
    printf("user_info->announce: %s\n", user_info->announce);
    printf("user_info->session_id: %s\n", user_info->session_id);
    printf("user_info->request_type: %s\n", user_info->request_type);
    printf("user_info->processor: %s\n", user_info->processor);

    free(out_data);


    xlibman_free(parse_json_module);
    xlibman_free(login_auth_module);

    return ret;
}