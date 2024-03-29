#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include "xlibman.h"
#include "list_helper.h"


struct xlibman_pre_or_post_module_t {
    // 在执行时，会根据module中的type和callback进行判断，然后执行对应的回调函数
    // 这里直接执行回调，不会去执行pre_list和post_list中的模块
    struct xlibman_module_t *module;        // module
    int weight;                             // module weight
};

struct xlibman_module_t {
    struct list_head list;                          // 用于链接到所有模块的链表
    char name[64];                                  // module name，暂未使用
    char path[256];                                 // module path，暂未使用
    enum xlibman_position_t position;               // module position，这里用作锚点位置编号，表示模块插入到哪个锚点位置
    struct list_head position_list;                 // list of modules with same position  有相同锚点的模块的链表,deprecated
    struct list_head module_list;                   // list of modules with same name  有相同名称的模块的链表，同一个库中可能有多个模块,deprecated
    char alias[64];                                 // module alias，暂未使用

    // module type，可选值为1、2
    // 1表示普通插件
    // 2表示拥有前置或后置插件的插件
    // type1类型插件，在执行时，只执行回调函数，不会执行pre_array和post_array中的模块
    // type2类型插件，在执行时，会执行pre_array中的模块，然后执行回调函数，最后执行post_array中的模块
    enum xlibman_type_t type;
    void *module_in_args;                           // module input data
    int module_in_len;                              // module input data length
    void *module_out_args;                          // module output data
    int module_out_len;                             // module output data length

    xlibman_func_comm_callback_t func_comm_callback;          // module callback function

    // 这里的权重是批在hook位置时，会根据权重进行排序，从大到小执行顺序
    int weight;                         // module weight

    // 这里的pre和post是指在模块执行时，会根据结构中的weight权重进行排序，从大到小执行顺序
    int pre_num;                        // number of pre modules 前置模块数量
    struct xlibman_pre_or_post_module_t pre_array[MAX_PRE_POST_MODULE_NUM];          // list of pre modules 前置模块列表
    int post_num;                       // number of post modules 后置模块数量
    struct xlibman_pre_or_post_module_t post_array[MAX_PRE_POST_MODULE_NUM];         // list of post modules 后置模块列表
    void *data;                         // module data  模块数据，由模块内部自己申请空间，在模块卸载时释放
    void *exts;                         // 后续需要扩展时留的指针
};


struct xlibman_position_store_t {
    struct list_head list;                  // list of position store
    enum xlibman_position_t position;       // position
    int module_num;                         // module number
    struct xlibman_module_t *modules[MAX_POSITION_MODULE_NUM];     // module
    int is_sorted;                          // is sorted
};


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
                            module_in_data = module->module_out_args;                                                                       \
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
    if (module->module_in_args != NULL) {
        free(module->module_in_args);
        module->module_in_args = NULL;
        module->module_in_len = 0;
    }
    if (module->module_out_args != NULL) {
        free(module->module_out_args);
        module->module_out_args = NULL;
        module->module_out_len = 0;
    }
    free(module);
}

void xlibman_set_module_type(_in struct xlibman_module_t *module, _in enum xlibman_type_t type)
{
    if (module == NULL) {
        return;
    }
    module->type = type;
}

void xlibman_set_module_position(_in struct xlibman_module_t *module, _in enum xlibman_position_t position)
{
    if (module == NULL) {
        return;
    }
    module->position = position;
}

void xlibman_set_module_weight(_in struct xlibman_module_t *module, _in int weight)
{
    if (module == NULL) {
        return;
    }
    module->weight = weight;
}

void xlibman_set_module_func(_in struct xlibman_module_t *module, _in xlibman_func_comm_callback_t func)
{
    if (module == NULL) {
        return;
    }
    module->func_comm_callback = func;
}


// 释放一个xlibman_module_t结构中记录参数的内存
void xlibman_free_module_args(_in struct xlibman_module_t *module)
{
    if (module == NULL) {
        return;
    }
    if (module->module_in_args != NULL) {
        free(module->module_in_args);
        module->module_in_args = NULL;
        module->module_in_len = 0;
    }
    if (module->module_out_args != NULL) {
        free(module->module_out_args);
        module->module_out_args = NULL;
        module->module_out_len = 0;
    }
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
    for (i = 0; i < position_store->module_num; i++) {
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
                        if (ext_callback == NULL) {
                            continue;
                        }
                        PROCESS_CALLBACK_AND_RECORD_ARGS(pre_module, ext_callback);
                        if (callback_ret == XLIBMAN_RET_SUSPEND) {
                            break;
                        }
                    }
                    if (callback != NULL) {
                        PROCESS_CALLBACK_AND_RECORD_ARGS(module, callback);
                        if (callback_ret == XLIBMAN_RET_SUSPEND) {
                            break;
                        }
                    }
                    for (j = 0; j < post_num; j++) {
                        xlibman_func_comm_callback_t ext_callback = NULL;
                        struct xlibman_module_t *post_module = NULL;
                        post_module = position_store->modules[i]->post_array[j].module;
                        ext_callback = (xlibman_func_comm_callback_t)post_module->func_comm_callback;
                        if (ext_callback == NULL) {
                            continue;
                        }
                        PROCESS_CALLBACK_AND_RECORD_ARGS(post_module, ext_callback);
                        if (callback_ret == XLIBMAN_RET_SUSPEND) {
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
                if (ext_callback == NULL) {
                    continue;
                }
                PROCESS_CALLBACK_AND_RECORD_ARGS(pre_module, ext_callback);
                if (callback_ret == XLIBMAN_RET_SUSPEND) {
                    break;
                }
            }
            if (callback != NULL) {
                PROCESS_CALLBACK_AND_RECORD_ARGS(module, callback);
                if (callback_ret == XLIBMAN_RET_SUSPEND) {
                    break;
                }
            }
            for (j = 0; j < post_num; j++) {
                xlibman_func_comm_callback_t ext_callback = NULL;
                struct xlibman_module_t *post_module = NULL;
                post_module = module->post_array[j].module;
                ext_callback = (xlibman_func_comm_callback_t)post_module->func_comm_callback;
                if (ext_callback == NULL) {
                    continue;
                }
                PROCESS_CALLBACK_AND_RECORD_ARGS(post_module, ext_callback);
                if (callback_ret == XLIBMAN_RET_SUSPEND) {
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

    if (module->module_in_args != NULL) {
        free(module->module_in_args);
        module->module_in_args = NULL;
        module->module_in_len = 0;
    }

    if (in_len > 0) {
        module->module_in_args = malloc(in_len);
        memcpy(module->module_in_args, in_data, in_len);
        module->module_in_len = in_len;
    } else {
        module->module_in_args = NULL;
        module->module_in_len = 0;
    }

    return ret;
}

int xlibman_record_output_args(_in struct xlibman_module_t *module, _in void *out_data, _in int out_len)
{
    int ret = 0;

    if (module->module_out_args != NULL) {
        free(module->module_out_args);
        module->module_out_args = NULL;
        module->module_out_len = 0;
    }

    if (out_len > 0) {
        module->module_out_args = malloc(out_len);
        memcpy(module->module_out_args, out_data, out_len);
        module->module_out_len = out_len;
    } else {
        module->module_out_args = NULL;
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

int ext_callback1(_in void *param1, _in int *in_len, _in void **param2, _in int *out_len, _in void *param3)
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

int ext_callback2(_in void *param1, _in int *in_len, _in void **param2, _in int *out_len, _in void *param3)
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

int plug_comm_callback(_in void *param1, _in int *in_len, _in void **param2, _in int *out_len, _in void *param3)
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

int plug_comm_callback2(_in void *param1, _in int *in_len, _in void **param2, _in int *out_len, _in void *param3)
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

int ext_callback3(_in void *param1, _in int *in_len, _in void **param2, _in int *out_len, _in void *param3)
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
    ext_module1->position = XLIBMAN_POSITION_ANYWHERE0;
    ext_module1->func_comm_callback = (void *)ext_callback1;

    struct xlibman_module_t *ext_module2 = (struct xlibman_module_t *)xlibman_alloc();
    ext_module2->type = XLIBMAN_MODULE_TYPE1;
    ext_module2->position = XLIBMAN_POSITION_ANYWHERE0;
    ext_module2->func_comm_callback = (void *)ext_callback2;

    struct xlibman_module_t *ext_module3 = (struct xlibman_module_t *)xlibman_alloc();
    ext_module3->type = XLIBMAN_MODULE_TYPE1;
    ext_module3->position = XLIBMAN_POSITION_ANYWHERE1;
    ext_module3->weight = 30;
    ext_module3->func_comm_callback = (void *)ext_callback3;

    struct xlibman_module_t *plug_comm_module = (struct xlibman_module_t *)xlibman_alloc();
    plug_comm_module->type = XLIBMAN_MODULE_TYPE2;
    plug_comm_module->position = XLIBMAN_POSITION_ANYWHERE1;
    plug_comm_module->weight = 10;
    plug_comm_module->func_comm_callback = (void *)plug_comm_callback;

    struct xlibman_module_t *plug_comm_module2 = (struct xlibman_module_t *)xlibman_alloc();
    plug_comm_module2->type = XLIBMAN_MODULE_TYPE2;
    plug_comm_module2->position = XLIBMAN_POSITION_ANYWHERE0;
    plug_comm_module2->weight = 20;
//    strcpy(plug_comm_module2->callback.c.request_type, "request_type2");
    plug_comm_module2->func_comm_callback = (void *)plug_comm_callback2;

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

    xlibman_hook(1, &in_data, sizeof(int), &out_data, &out_len);

    printf("out_data = %d\n", *out_data);

//    free(out_data);

    printf("Hello World\n");

//    xlibman_hook(1, &in_data, &out_data, &out_len);

    xlibman_free(ext_module1);
    xlibman_free(ext_module2);
    xlibman_free(ext_module3);
    xlibman_free(plug_comm_module);
    xlibman_free(plug_comm_module2);

    return ret;
}

#endif