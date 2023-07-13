#ifndef XLIBMAN
#define XLIBMAN

#include "list_helper.h"

#define _in
#define _out
#define MAX_PRE_POST_MODULE_NUM 20
#define MAX_POSITION_MODULE_NUM 20

struct xlibman_module_t;

// 初始化函数，用来填充xlibman_module_t结构，如果一个库中有多个模块，如何注册多个模块呢？
// 该函数的返回值为0时，表示初始化成功，否则表示初始化失败
#define XLIBMAN_MODULE_INIT_FUNC_NAME "xlibman_module_init"
#define XLIBMAN_MODULE_UNINIT_FUNC_NAME "xlibman_module_uninit"

// 插件需要实现的初始化函数，框架在加载插件时，会调用该函数
typedef int (*xlibman_init_t)(_in _out void *args);

// 插件需要实现的反初始化函数，框架在卸载插件时，会调用该函数
typedef int (*xlibman_uninit_t)(_in _out void *args);

typedef int (*xlibman_func_ext_callback_t)(_in void *in_data, _out void **out_data, _in _out int *out_len, _in _out void *module_data);

typedef int (*xlibman_plug_comm_callback_t)(_in void *in_data, _out void **out_data, _in _out int *out_len, _in _out void *module_data);


// 锚点位置，预置10个位置，不够再添加
enum xlibman_position_t {
    // 一般功能扩展插件都是没有锚点的
    XLIBMAN_POSITION_ANYWHERE0 = 0,
    XLIBMAN_POSITION_ANYWHERE1,
    XLIBMAN_POSITION_ANYWHERE2,
    XLIBMAN_POSITION_ANYWHERE3,
    XLIBMAN_POSITION_ANYWHERE4,
    XLIBMAN_POSITION_ANYWHERE5,
    XLIBMAN_POSITION_ANYWHERE6,
    XLIBMAN_POSITION_ANYWHERE7,
    XLIBMAN_POSITION_ANYWHERE8,
    XLIBMAN_POSITION_ANYWHERE9,
};

// 模块回调函数执行方式
// PROCESS_CALLBACK_ONLY_SELF: 只执行自己的回调函数，不执行pre_list和post_list中的模块
#define PROCESS_CALLBACK_ONLY_SELF          1
// PROCESS_CALLBACK_PRE_SELF_POST: 先执行pre_list中的模块，然后执行自己的回调函数，最后执行post_list中的模块
#define PROCESS_CALLBACK_PRE_SELF_POST      2
// PROCESS_CALLBACK_PRE_SELF: 先执行pre_list中的模块，然后执行自己的回调函数，不执行post_list中的模块
#define PROCESS_CALLBACK_PRE_SELF           3
// PROCESS_CALLBACK_SELF_POST: 先执行自己的回调函数，然后执行post_list中的模块，不执行pre_list中的模块
#define PROCESS_CALLBACK_SELF_POST          4

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
    union {
        xlibman_func_ext_callback_t func_ext_callback;          // module callback function
        struct {
            char request_type[64];                              // request type
            xlibman_plug_comm_callback_t plug_comm_callback;    // module callback function
        } c;
    } callback;

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
    struct xlibman_module_t modules[MAX_POSITION_MODULE_NUM];     // module
    int is_sorted;                          // is sorted
};

// 申请一个xlibman_module_t结构
void *xlibman_alloc();

// 释放一个xlibman_module_t结构
void xlibman_free(_in struct xlibman_module_t *module);

// xlibman 添加一个模块
int xlibman_add_module(_in struct xlibman_module_t *module);

// 添加一个前置模块
int xlibman_add_pre_module(_in struct xlibman_module_t *module, _in struct xlibman_module_t *pre_module, _in int weight);

// 添加一个后置模块
int xlibman_add_post_module(_in struct xlibman_module_t *module, _in struct xlibman_module_t *post_module, _in int weight);

// 锚点
int xlibman_hook(_in enum xlibman_position_t position, _in void *in_data, _out void **out_data, _in _out int *out_len);

int xlibman_hook_via_request_type(_in char *request_type, _in void *in_data, _out void **out_data, _in _out int *out_len);

// 通过别名获取模块
int xlibman_get_any_module(_in char *alias, _out struct xlibman_module_t **module);

// 获取锚点位置的模块的执行结果
int xlibman_get_any_position_result(_in enum xlibman_position_t position, _in _out void **out_data, _in _out int *out_len);

// xlibman init，此函数由框架调用
int xlibman_init();


#endif /* XLIBMAN */
