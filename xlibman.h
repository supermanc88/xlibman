#ifndef XLIBMAN
#define XLIBMAN

#include "list_helper.h"

#define _in
#define _out
#define MAX_PRE_POST_MODULE_NUM 20

struct xlibman_module_t;

// 初始化函数，用来填充xlibman_module_t结构，如果一个库中有多个模块，如何注册多个模块呢？
// 该函数的返回值为0时，表示初始化成功，否则表示初始化失败
#define XLIBMAN_MODULE_INIT_FUNC_NAME "xlibman_module_init"
#define XLIBMAN_MODULE_UNINIT_FUNC_NAME "xlibman_module_uninit"

typedef int (*xlibman_init_t)(_in _out struct xlibman_module_t **module, _in _out int *module_num);

typedef int (*xlibman_uninit_t)(_in _out struct xlibman_module_t **module, _in int module_num);

typedef int (*xlibman_func_ext_callback_t)(_in void *in_data, _out void *out_data, _in _out void *module_data);

typedef int (*xlibman_plug_comm_callback_t)(_in void *in_data, _out void *out_data, _in _out void *module_data);


// 锚点位置，需要添加新的锚点时，在这里添加
enum xlibman_position_t {
    // 一般功能扩展插件都是没有锚点的
    XLIBMAN_POSITION_NONE = 0,
    XLIBMAN_POSITION_HEAD,
    XLIBMAN_POSITION_TAIL,
};


// 模块类型
enum xlibman_type_t {
    // type1类型插件，在执行时，只执行回调函数，不会执行pre_array和post_array中的模块
    XLIBMAN_PLUG_TYPE1 = 1,
    // type2类型插件，在执行时，会执行pre_array中的模块，然后执行回调函数，最后执行post_array中的模块
    XLIBMAN_PLUG_TYPE2,
};

struct xlibman_pre_or_post_module_t {
    // 在执行时，会根据module中的type和callback进行判断，然后执行对应的回调函数
    // 这里直接执行回调，不会去执行pre_list和post_list中的模块
    struct xlibman_module_t *module;        // module
    int weight;                             // module weight
};

struct xlibman_module_t {
    struct list_head list;                  // list of modules 所有模块的链表
    char name[64];                          // module name
    char path[256];                         // module path
    enum xlibman_position_t position;            // module position，这里用作锚点位置
    struct list_head position_list;         // list of modules with same position  有相同锚点的模块的链表
    struct list_head module_list;           // list of modules with same name  有相同名称的模块的链表，同一个库中可能有多个模块

    enum xlibman_type_t type;                    // module type
    union {
        xlibman_func_ext_callback_t func_ext_callback;          // module callback function
        struct {
            char request_type[64];                              // request type
            xlibman_plug_comm_callback_t plug_comm_callback;    // module callback function
        } c;
    } callback;

    // 这里的pre和post是指在模块执行时，会根据结构中的weight权重进行排序，从大到小执行顺序
    int pre_num;                        // number of pre modules 前置模块数量
    struct xlibman_pre_or_post_module_t pre_array[MAX_PRE_POST_MODULE_NUM];          // list of pre modules 前置模块列表
    int post_num;                       // number of post modules 后置模块数量
    struct xlibman_pre_or_post_module_t post_array[MAX_PRE_POST_MODULE_NUM];         // list of post modules 后置模块列表
    void *data;                         // module data  模块数据，由模块内部自己申请空间，在模块卸载时释放
    void *exts;                         // 后续需要扩展时留的指针
};

// 申请一个xlibman_module_t结构
void *xlibman_alloc();

// 释放一个xlibman_module_t结构
void xlibman_free(_in struct xlibman_module_t *module);

// 添加一个前置模块
int xlibman_add_pre_module(_in struct xlibman_module_t *module, _in struct xlibman_module_t *pre_module, _in int weight);

// 添加一个后置模块
int xlibman_add_post_module(_in struct xlibman_module_t *module, _in struct xlibman_module_t *post_module, _in int weight);

// 锚点
int xlibman_hook(_in enum xlibman_position_t position);


#endif /* XLIBMAN */
