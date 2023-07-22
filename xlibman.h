#ifndef XLIBMAN
#define XLIBMAN

#define _in
#define _out
#define MAX_PRE_POST_MODULE_NUM 20
#define MAX_POSITION_MODULE_NUM 20

#define XLIBMAN_IGNORE_EXECUTE_START    if (0) {
#define XLIBMAN_IGNORE_EXECUTE_END      }
#define XLIBMAN_CODE_SNIPPET_START      {
#define XLIBMAN_CODE_SNIPPET_END        }
#define XLIBMAN_FUNC_FINISH_WITH_RET(ret) \
    return ret;
#define XLIBMAN_FUNC_FINISH_WITHOUT_RET \
    return;

#define XLIBMAN_RET_SUCCESSED       0
#define XLIBMAN_RET_FAILED          -1
#define XLIBMAN_RET_CONTINUE        0
#define XLIBMAN_RET_SUSPEND         1

struct xlibman_module_t;

struct xlibman_pre_or_post_module_t;

struct xlibman_module_t;

struct xlibman_position_store_t;

// 锚点位置，预置20个位置，不够再添加
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
    XLIBMAN_POSITION_ANYWHERE10,
    XLIBMAN_POSITION_ANYWHERE11,
    XLIBMAN_POSITION_ANYWHERE12,
    XLIBMAN_POSITION_ANYWHERE13,
    XLIBMAN_POSITION_ANYWHERE14,
    XLIBMAN_POSITION_ANYWHERE15,
    XLIBMAN_POSITION_ANYWHERE16,
    XLIBMAN_POSITION_ANYWHERE17,
    XLIBMAN_POSITION_ANYWHERE18,
    XLIBMAN_POSITION_ANYWHERE19,
};

// 模块回调函数执行方式，保留记录，暂时不用
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
    XLIBMAN_MODULE_TYPE1 = 1,
    // type2类型插件，在执行时，会执行pre_array中的模块，然后执行回调函数，最后执行post_array中的模块
    XLIBMAN_MODULE_TYPE2,
    // type3类型插件为按需调用模块，在此类型插件中，需要在回调函数中分配好调用规则，然后调用对应的模块，保留记录，暂时不用
    XLIBMAN_PLUG_TYPE3,
};

// 初始化函数，用来填充xlibman_module_t结构，如果一个库中有多个模块，如何注册多个模块呢？
// 该函数的返回值为0时，表示初始化成功，否则表示初始化失败
#define XLIBMAN_MODULE_INIT_FUNC_NAME "xlibman_module_init"
#define XLIBMAN_MODULE_UNINIT_FUNC_NAME "xlibman_module_uninit"

// 插件需要实现的初始化函数，框架在加载插件时，会调用该函数
typedef int (*xlibman_init_t)(_in _out void *args);

// 插件需要实现的反初始化函数，框架在卸载插件时，会调用该函数
typedef int (*xlibman_uninit_t)(_in _out void *args);

typedef int (*xlibman_func_comm_callback_t)(_in void *in_data, _in int in_len, _out void **out_data, _in _out int *out_len, _in _out void *module_data);

// 申请一个xlibman_module_t结构
void *xlibman_alloc();

// 释放一个xlibman_module_t结构
void xlibman_free(_in struct xlibman_module_t *module);

void xlibman_set_module_type(_in struct xlibman_module_t *module, _in enum xlibman_type_t type);

void xlibman_set_module_position(_in struct xlibman_module_t *module, _in enum xlibman_position_t position);

void xlibman_set_module_weight(_in struct xlibman_module_t *module, _in int weight);

void xlibman_set_module_func(_in struct xlibman_module_t *module, _in xlibman_func_comm_callback_t func);

// 释放一个xlibman_module_t结构中记录参数的内存
void xlibman_free_module_args(_in struct xlibman_module_t *module);

// xlibman 添加一个模块
int xlibman_add_module(_in struct xlibman_module_t *module);

// 添加一个前置模块
int xlibman_add_pre_module(_in struct xlibman_module_t *module, _in struct xlibman_module_t *pre_module, _in int weight);

// 添加一个后置模块
int xlibman_add_post_module(_in struct xlibman_module_t *module, _in struct xlibman_module_t *post_module, _in int weight);

// 锚点
int xlibman_hook(_in enum xlibman_position_t position, _in void *in_data, _in int in_len, _out void **out_data, _in _out int *out_len);

int xlibman_hook_via_alias(_in char *alias, _in void *in_data, _in int in_len, _out void **out_data, _in _out int *out_len);

// 通过别名获取模块，暂未实现
//int xlibman_get_any_module(_in char *alias, _out struct xlibman_module_t **module);

// 获取锚点位置的模块的执行结果， 暂未实现
//int xlibman_get_any_position_result(_in enum xlibman_position_t position, _in _out void **out_data, _in _out int *out_len);

// xlibman init，此函数由框架调用
int xlibman_init();


#endif /* XLIBMAN */
