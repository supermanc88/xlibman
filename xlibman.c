#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include "xlibman.h"


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

struct xlibman_module_t test_module[10] = {0};
// 是否已经过滤
int is_filtered = 0;
struct list_head all_module_list = {0};

// 锚点
int xlibman_hook(_in enum xlibman_position_t position)
{
    int ret = 0;

    if (is_filtered == 0) {
        // 遍历all_module_list,将其中模块position和position相同的模块添加到test_module数组中
        struct list_head *pos = NULL;
        struct xlibman_module_t *module = NULL;
        int i = 0;
        list_for_each(pos, &all_module_list) {
            module = list_entry(pos, struct xlibman_module_t, list);
            if (module->position == position) {
                test_module[i] = *module;
                i++;
            }
        }
        is_filtered = 1;

    } else {
        // 已经过滤了，直接执行其中的回调函数
    }

    xlibman_plug_comm_callback_t callback = NULL;
    xlibman_func_ext_callback_t ext_callback = NULL;

    // 遍历test_module数组，执行其中的回调函数
    int i = 0;
    for (i = 0; i < 10; i++) {
        if (test_module[i].position == position) {
            callback = (xlibman_plug_comm_callback_t)test_module[i].callback.c.plug_comm_callback;
            int pre_num = test_module[i].pre_num;
            int post_num = test_module[i].post_num;
            int j = 0;
            for (j = 0; j < pre_num; j++) {
                ext_callback = (xlibman_func_ext_callback_t)test_module[i].pre_array[j].module->callback.func_ext_callback;
                ext_callback(NULL, NULL, NULL);
            }
            callback(NULL, NULL, NULL);
            for (j = 0; j < post_num; j++) {
                ext_callback = (xlibman_func_ext_callback_t)test_module[i].post_array[j].module->callback.func_ext_callback;
                ext_callback(NULL, NULL, NULL);
            }
        }
    }

    return ret;
}


int ext_callback1(_in void *param1, _in void *param2, _in void *param3)
{
    int ret = 0;

    printf("ext_callback1\n");

    return ret;
}

int ext_callback2(_in void *param1, _in void *param2, _in void *param3)
{
    int ret = 0;

    printf("ext_callback2\n");

    return ret;
}

int plug_comm_callback(_in void *param1, _in void *param2, _in void *param3)
{
    int ret = 0;

    printf("plug_comm_callback\n");

    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;

    struct xlibman_module_t ext_module1 = {0};
    ext_module1.position = 0;
    ext_module1.callback.func_ext_callback = (void *)ext_callback1;

    struct xlibman_module_t ext_module2 = {0};
    ext_module2.position = 0;
    ext_module2.callback.func_ext_callback = (void *)ext_callback2;

    struct xlibman_module_t plug_comm_module = {0};
    plug_comm_module.position = 1;
    plug_comm_module.callback.c.plug_comm_callback = (void *)plug_comm_callback;

    INIT_LIST_HEAD(&all_module_list);
    list_add(&ext_module1.list, &all_module_list);
    list_add(&ext_module2.list, &all_module_list);
    list_add(&plug_comm_module.list, &all_module_list);


    xlibman_add_pre_module(&plug_comm_module, &ext_module1, 10);
    xlibman_add_pre_module(&plug_comm_module, &ext_module2, 20);

    xlibman_add_post_module(&plug_comm_module, &ext_module1, 20);
    xlibman_add_post_module(&plug_comm_module, &ext_module2, 10);

    xlibman_hook(1);

    printf("Hello World\n");

    xlibman_hook(1);




    return ret;
}