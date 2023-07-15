#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include "xlibman.h"

struct list_head g_xlibman_all_module_list = {0};
struct list_head g_xlibman_position_store_list = {0};

int xlibman_record_input_args(_in struct xlibman_module_t *module, _in void *in_data, _in int in_len);
int xlibman_record_output_args(_in struct xlibman_module_t *module, _in void *out_data, _in int out_len);

#define PROCESS_CALLBACK_AND_RECORD_ARGS(module, callback)                                                                                  \
                        {                                                                                                                   \
                            module_data = module->data;                                                                                     \
                            xlibman_record_input_args(module, module_in_data, module_in_len);                                               \
                            callback_ret = callback(module_in_data, module_in_len, module_out_data, &module_out_len, module_data);          \
                            xlibman_record_output_args(module, *module_out_data, module_out_len);                                           \
                            if (*module_out_data != NULL) {                                                                                 \
                                free(*module_out_data);                                                                                     \
                                *module_out_data = NULL;                                                                                    \
                            }                                                                                                               \
                            module_in_data = module->module_out_data;                                                                       \
                            module_in_len = module->module_out_len;                                                                         \
                        }

// 申请一个xlibman_module_t结构
void *xlibman_alloc()
{
    struct xlibman_module_t *module = (struct xlibman_module_t *)malloc(sizeof(struct xlibman_module_t));
    if (module == NULL) {
        return NULL;
    }
    memset(module, 0, sizeof(struct xlibman_module_t));
    return module;
}

// 释放一个xlibman_module_t结构
void xlibman_free(_in struct xlibman_module_t *module)
{
    if (module == NULL) {
        return;
    }
    free(module);
}

// 添加一个前置模块，将pre_module添加到module的前置模块列表pre_array数组中，并将weight设置为权重，再按照weight从大到小排序
int xlibman_add_pre_module(_in struct xlibman_module_t *module, _in struct xlibman_module_t *pre_module, _in int weight)
{
    int ret = 0;

    struct xlibman_pre_or_post_module_t pre_module_item;
    pre_module_item.module = pre_module;
    pre_module_item.weight = weight;

    // 添加到前置模块列表中
    // 判断是否已满
    if (module->pre_num >= MAX_PRE_POST_MODULE_NUM) {
        ret = -1;
        goto out;
    }

    // 添加到前置模块列表中
    module->pre_array[module->pre_num] = pre_module_item;
    module->pre_num++;

    // 按照weight从大到小排序
    int i, j;
    struct xlibman_pre_or_post_module_t tmp;
    for (i = 0; i < module->pre_num - 1; i++) {
        for (j = 0; j < module->pre_num - 1 - i; j++) {
            if (module->pre_array[j].weight < module->pre_array[j + 1].weight) {
                tmp = module->pre_array[j];
                module->pre_array[j] = module->pre_array[j + 1];
                module->pre_array[j + 1] = tmp;
            }
        }
    }

out:
    return ret;
}

// 添加一个后置模块，将post_module添加到module的前置模块列表pre_array数组中，并将weight设置为权重，再按照weight从大到小排序
int xlibman_add_post_module(_in struct xlibman_module_t *module, _in struct xlibman_module_t *post_module, _in int weight)
{
    int ret = 0;

    struct xlibman_pre_or_post_module_t post_module_item;
    post_module_item.module = post_module;
    post_module_item.weight = weight;

    // 添加到后置模块列表中
    // 判断是否已满
    if (module->post_num >= MAX_PRE_POST_MODULE_NUM) {
        ret = -1;
        goto out;
    }

    // 添加到后置模块列表中
    module->post_array[module->post_num] = post_module_item;
    module->post_num++;

    // 按照weight从大到小排序
    int i, j;
    struct xlibman_pre_or_post_module_t tmp;
    for (i = 0; i < module->post_num - 1; i++) {
        for (j = 0; j < module->post_num - 1 - i; j++) {
            if (module->post_array[j].weight < module->post_array[j + 1].weight) {
                tmp = module->post_array[j];
                module->post_array[j] = module->post_array[j + 1];
                module->post_array[j + 1] = tmp;
            }
        }
    }

out:
    return ret;
}

//struct xlibman_module_t test_module[10] = {0};
// 是否已经过滤
//int is_filtered = 0;


// 锚点
int xlibman_hook(_in enum xlibman_position_t position, _in void *in_data, _in int in_len, _out void **out_data, _in _out int *out_len)
{
    int ret = 0;
    if (out_data == NULL || out_len == NULL) {
        ret = -1;
        goto out;
    }

    // 通过position从g_xlibman_position_store_list中找到对应的position_store
    struct list_head *pos = NULL;
    struct xlibman_position_store_t *position_store = NULL;
    int is_found = 0;
    list_for_each(pos, &g_xlibman_position_store_list) {
        position_store = list_entry(pos, struct xlibman_position_store_t, list);
        if (position_store->position == position) {
            is_found = 1;
            break;
        }
    }

    // 如果没有找到，说明该position还没有注册过模块，直接返回
    if (is_found == 0) {
        goto out;
    }

    // 找到后，如果该position_store中的module_num为0，说明该position还没有注册过模块，直接返回
    if (position_store->module_num == 0) {
        goto out;
    }

    xlibman_func_comm_callback_t callback = NULL;
    void *module_data = NULL;
    void *module_in_data = NULL;
    void **module_out_data = NULL;
    int module_in_len = 0;
    int module_out_len = 0;

    module_in_data = in_data;
    module_out_data = out_data;
    module_in_len = in_len;
    module_out_len = 0;
    // 遍历test_module数组，执行其中的回调函数
    int i = 0;
    int callback_ret = 0;
    for (i = 0; i < MAX_POSITION_MODULE_NUM; i++) {
        if (position_store->modules[i]->position == position) {
            if (position_store->modules[i]->type == XLIBMAN_MODULE_TYPE2) {
                do {
                    struct xlibman_module_t *module = NULL;
                    module = position_store->modules[i];
                    callback = module->func_comm_callback;
                    int pre_num = position_store->modules[i]->pre_num;
                    int post_num = position_store->modules[i]->post_num;
                    int j = 0;
                    for (j = 0; j < pre_num; j++) {
                        xlibman_func_comm_callback_t ext_callback = NULL;
                        struct xlibman_module_t *pre_module = NULL;
                        pre_module = position_store->modules[i]->pre_array[j].module;
                        ext_callback = (xlibman_func_comm_callback_t)pre_module->func_comm_callback;
                        PROCESS_CALLBACK_AND_RECORD_ARGS(pre_module, ext_callback);
                        if (callback_ret == RET_SUSPEND) {
                            break;
                        }
                    }
                    PROCESS_CALLBACK_AND_RECORD_ARGS(module, callback);
                    if (callback_ret == RET_SUSPEND) {
                        break;
                    }
                    for (j = 0; j < post_num; j++) {
                        xlibman_func_comm_callback_t ext_callback = NULL;
                        struct xlibman_module_t *post_module = NULL;
                        post_module = position_store->modules[i]->post_array[j].module;
                        ext_callback = (xlibman_func_comm_callback_t)post_module->func_comm_callback;
                        PROCESS_CALLBACK_AND_RECORD_ARGS(post_module, ext_callback);
                        if (callback_ret == RET_SUSPEND) {
                            break;
                        }
                    }
                } while (0);
            } else if (position_store->modules[i]->type == XLIBMAN_MODULE_TYPE1) {
                struct xlibman_module_t *module = NULL;
                module = position_store->modules[i];
                callback = module->func_comm_callback;
                PROCESS_CALLBACK_AND_RECORD_ARGS(module, callback);
            }
        }
    }

    *out_data = module_in_data;
    *out_len = module_in_len;

out:
    return ret;
}

int xlibman_hook_via_alias(_in char *alias, _in void *in_data, _in int in_len, _out void **out_data, _in _out int *out_len)
{
    int ret = 0;

    // 遍历g_xlibman_all_module_list，查找其中是否有request_type的entry
    struct list_head *pos = NULL;
    struct xlibman_module_t *module = NULL;
    int is_found = 0;
    list_for_each(pos, &g_xlibman_all_module_list) {
        module = list_entry(pos, struct xlibman_module_t, list);
        if (strcmp(module->alias, alias) == 0) {
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        ret = -1;
        goto out;
    }

    // 执行回调函数
    xlibman_func_comm_callback_t callback = NULL;
    void *module_data = NULL;
    void *module_in_data = NULL;
    void **module_out_data = NULL;
    int module_in_len = 0;
    int module_out_len = 0;

    module_in_data = in_data;
    module_out_data = out_data;
    module_in_len = in_len;
    module_out_len = 0;

    callback = (xlibman_func_comm_callback_t)module->func_comm_callback;
    int callback_ret = 0;
    if (module->type == XLIBMAN_MODULE_TYPE2) {
        do {
            int pre_num = module->pre_num;
            int post_num = module->post_num;
            int j = 0;
            for (j = 0; j < pre_num; j++) {
                xlibman_func_comm_callback_t ext_callback = NULL;
                struct xlibman_module_t *pre_module = NULL;
                pre_module = module->pre_array[j].module;
                ext_callback = (xlibman_func_comm_callback_t)pre_module->func_comm_callback;
                PROCESS_CALLBACK_AND_RECORD_ARGS(pre_module, ext_callback);
                if (callback_ret == RET_SUSPEND) {
                    break;
                }
            }
            PROCESS_CALLBACK_AND_RECORD_ARGS(module, callback);
            if (callback_ret == RET_SUSPEND) {
                break;
            }
            for (j = 0; j < post_num; j++) {
                xlibman_func_comm_callback_t ext_callback = NULL;
                struct xlibman_module_t *post_module = NULL;
                post_module = module->post_array[j].module;
                ext_callback = (xlibman_func_comm_callback_t)post_module->func_comm_callback;
                PROCESS_CALLBACK_AND_RECORD_ARGS(post_module, ext_callback);
                if (callback_ret == RET_SUSPEND) {
                    break;
                }
            }
        } while (0);

    } else if (module->type == XLIBMAN_MODULE_TYPE1) {
        PROCESS_CALLBACK_AND_RECORD_ARGS(module, callback);
    }

    *out_data = module_in_data;
    *out_len = module_in_len;

out:
    return ret;
}

int xlibman_record_input_args(_in struct xlibman_module_t *module, _in void *in_data, _in int in_len)
{
    int ret = 0;

    if (module->module_in_data != NULL) {
        free(module->module_in_data);
        module->module_in_data = NULL;
        module->module_in_len = 0;
    }

    if (in_len > 0) {
        module->module_in_data = malloc(in_len);
        memcpy(module->module_in_data, in_data, in_len);
        module->module_in_len = in_len;
    } else {
        module->module_in_data = NULL;
        module->module_in_len = 0;
    }

    return ret;
}

int xlibman_record_output_args(_in struct xlibman_module_t *module, _in void *out_data, _in int out_len)
{
    int ret = 0;

    if (module->module_out_data != NULL) {
        free(module->module_out_data);
        module->module_out_data = NULL;
        module->module_out_len = 0;
    }

    if (out_len > 0) {
        module->module_out_data = malloc(out_len);
        memcpy(module->module_out_data, out_data, out_len);
        module->module_out_len = out_len;
    } else {
        module->module_out_data = NULL;
        module->module_out_len = 0;
    }

    return ret;
}

// xlibman init
int xlibman_init()
{
    // 初始化g_xlibman_all_module_list
    INIT_LIST_HEAD(&g_xlibman_all_module_list);

    // 初始化g_xlibman_position_store_list
    INIT_LIST_HEAD(&g_xlibman_position_store_list);

    return 0;
}

// xlibman 添加一个模块
int xlibman_add_module(_in struct xlibman_module_t *module)
{
    int ret = 0;

    // 将module添加到g_xlibman_all_module_list中
    list_add_tail(&module->list, &g_xlibman_all_module_list);

    int position = module->position;

    // 遍历g_xlibman_position_store_list，查找其中是否有positionr的entry
    struct list_head *pos = NULL;
    struct xlibman_position_store_t *position_store = NULL;
    int is_exist = 0;
    list_for_each(pos, &g_xlibman_position_store_list) {
        position_store = list_entry(pos, struct xlibman_position_store_t, list);
        if (position_store->position == position) {
            is_exist = 1;
            break;
        }
    }

    // 如果没有找到position的entry，则创建一个新的entry
    if (is_exist == 0) {
        position_store = (struct xlibman_position_store_t *)malloc(sizeof(struct xlibman_position_store_t));
        memset(position_store, 0, sizeof(struct xlibman_position_store_t));
        position_store->position = position;
        list_add_tail(&position_store->list, &g_xlibman_position_store_list);
    }

    // 将module添加到position_store->modules数组中
    int position_store_num = position_store->module_num;
    position_store->modules[position_store_num] = module;

    position_store->module_num++;

    // 将position_store->modules数组按照weight进行从大到小排序
    int i = 0;
    int j = 0;
    struct xlibman_module_t *tmp;
    for (i = 0; i < position_store->module_num - 1; i++) {
        for (j = 0; j < position_store->module_num - 1 - i; j++) {
            if (position_store->modules[j]->weight < position_store->modules[j + 1]->weight) {
                tmp = position_store->modules[j];
                position_store->modules[j] = position_store->modules[j + 1];
                position_store->modules[j + 1] = tmp;
            }
        }
    }


    return ret;
}

#if 0

int ext_callback1(_in void *param1, _in void **param2, _in int *out_len, _in void *param3)
{
    int ret = 0;

    int *in_data = (int *)param1;

    printf("ext_callback1 in_data = %d\n", *in_data);

    int *out_data = (int *)malloc(sizeof(int));
    *param2 = out_data;
    *out_data = *in_data + 1;

    *out_len = sizeof(int);

    printf("ext_callback1\n");

    return ret;
}

int ext_callback2(_in void *param1, _in void **param2, _in int *out_len, _in void *param3)
{
    int ret = 0;

    int *in_data = (int *)param1;

    printf("ext_callback2 in_data = %d\n", *in_data);

    int *out_data = (int *)malloc(sizeof(int));
    *param2 = out_data;
    *out_data = *in_data + 1;

    *out_len = sizeof(int);

    printf("ext_callback2\n");

    return ret;
}

int plug_comm_callback(_in void *param1, _in void **param2, _in int *out_len, _in void *param3)
{
    int ret = 0;

    int *in_data = (int *)param1;

    printf("plug_comm_callback in_data = %d\n", *in_data);

    int *out_data = (int *)malloc(sizeof(int));
    *param2 = out_data;
    *out_data = *in_data + 1;

    *out_len = sizeof(int);

    printf("plug_comm_callback\n");

    return ret;
}

int plug_comm_callback2(_in void *param1, _in void **param2, _in int *out_len, _in void *param3)
{
    int ret = 0;

    int *in_data = (int *)param1;

    printf("plug_comm_callback2 in_data = %d\n", *in_data);

    int *out_data = (int *)malloc(sizeof(int));
    *param2 = out_data;
    *out_data = *in_data + 1;

    *out_len = sizeof(int);

    printf("plug_comm_callback2\n");

    return ret;
}

int ext_callback3(_in void *param1, _in void **param2, _in int *out_len, _in void *param3)
{
    int ret = 0;

    int *in_data = (int *)param1;

    printf("ext_callback3 in_data = %d\n", *in_data);

    int *out_data = (int *)malloc(sizeof(int));
    *param2 = out_data;
    *out_data = *in_data + 1;

    *out_len = sizeof(int);

    printf("ext_callback3\n");

    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;

    struct xlibman_module_t *ext_module1 = (struct xlibman_module_t *)xlibman_alloc();
    ext_module1->type = XLIBMAN_MODULE_TYPE1;
    ext_module1->position = 0;
    ext_module1->callback.func_ext_callback = (void *)ext_callback1;

    struct xlibman_module_t *ext_module2 = (struct xlibman_module_t *)xlibman_alloc();
    ext_module2->type = XLIBMAN_MODULE_TYPE1;
    ext_module2->position = 0;
    ext_module2->callback.func_ext_callback = (void *)ext_callback2;

    struct xlibman_module_t *ext_module3 = (struct xlibman_module_t *)xlibman_alloc();
    ext_module3->type = XLIBMAN_MODULE_TYPE1;
    ext_module3->position = 1;
    ext_module3->weight = 30;
    ext_module3->callback.func_ext_callback = (void *)ext_callback3;

    struct xlibman_module_t *plug_comm_module = (struct xlibman_module_t *)xlibman_alloc();
    plug_comm_module->type = XLIBMAN_MODULE_TYPE2;
    plug_comm_module->position = 1;
    plug_comm_module->weight = 10;
    plug_comm_module->callback.c.plug_comm_callback = (void *)plug_comm_callback;

    struct xlibman_module_t *plug_comm_module2 = (struct xlibman_module_t *)xlibman_alloc();
    plug_comm_module2->type = XLIBMAN_MODULE_TYPE2;
    plug_comm_module2->position = 0;
    plug_comm_module2->weight = 20;
    strcpy(plug_comm_module2->callback.c.request_type, "request_type2");
    plug_comm_module2->callback.c.plug_comm_callback = (void *)plug_comm_callback2;

    // INIT_LIST_HEAD(&g_xlibman_all_module_list);
    // list_add(&ext_module1.list, &g_xlibman_all_module_list);
    // list_add(&ext_module2.list, &g_xlibman_all_module_list);
    // list_add(&plug_comm_module.list, &g_xlibman_all_module_list);
    // list_add(&plug_comm_module2.list, &g_xlibman_all_module_list);

    xlibman_init();


    xlibman_add_pre_module(plug_comm_module, ext_module1, 10);
    xlibman_add_pre_module(plug_comm_module, ext_module2, 20);

    xlibman_add_post_module(plug_comm_module, ext_module1, 20);
    xlibman_add_post_module(plug_comm_module, ext_module2, 10);

    xlibman_add_module(ext_module1);
    xlibman_add_module(ext_module2);
    xlibman_add_module(ext_module3);
    xlibman_add_module(plug_comm_module);
    xlibman_add_module(plug_comm_module2);


    int in_data = 1;
    int *out_data = NULL;
    int out_len = 0;

    xlibman_hook(1, &in_data, &out_data, &out_len);

    printf("out_data = %d\n", *out_data);

    free(out_data);

    printf("Hello World\n");

//    xlibman_hook(1, &in_data, &out_data, &out_len);

    xlibman_hook_via_request_type("request_type2", &in_data, &out_data, &out_len);

    printf("out_data = %d\n", *out_data);

    free(out_data);


    xlibman_free(ext_module1);
    xlibman_free(ext_module2);
    xlibman_free(ext_module3);
    xlibman_free(plug_comm_module);
    xlibman_free(plug_comm_module2);

    return ret;
}

#endif