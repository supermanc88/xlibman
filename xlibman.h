#ifndef XLIBMAN
#define XLIBMAN

#define _in
#define _out
#define MAX_PRE_POST_MODULE_NUM 20

struct xlibman_module_t;

typedef int (*xlibman_init_t)(_in _out struct xlibman_module_t *module);

typedef int (*xlibman_uninit_t)(_in _out struct xlibman_module_t *module);

typedef int (*xlibman_func_ext_callback_t)(_in void *in_data, _out void *out_data, _in _out void *module_data);

typedef int (*xlibman_plug_comm_callback_t)(_in void *in_data, _out void *out_data, _in _out void *module_data);

struct list_head {
    struct list_head *next, *prev;
};

// 锚点位置
enum xlibman_position_t {
    // 一般功能扩展插件都是没有锚点的
    XLIBMAN_POSITION_NONE = 0,
    XLIBMAN_POSITION_HEAD,
    XLIBMAN_POSITION_TAIL,
};


// 模块类型
enum xlibman_type_t {
    XLIBMAN_TYPE_FUNC_EXT = 0,
    XLIBMAN_TYPE_PLUG_COMM,
};

struct xlibman_pre_or_post_module_t {
    // 在执行时，会根据module中的type和callback进行判断，然后执行对应的回调函数
    // 这里直接执行回调，不会去执行pre_list和post_list中的模块
    struct xlibman_module_t *module;        // module
    int weight;                             // module weight
};

struct xlibman_module_t {
    struct list_head list;                  // list of modules
    char name[64];                          // module name
    char path[256];                         // module path
    xlibman_position_t position;            // module position，这里用作锚点位置

    xlibman_type_t type;                    // module type
    union {
        xlibman_func_ext_callback_t func_ext_callback;          // module callback function
        struct {
            char request_type[64];                              // request type
            xlibman_plug_comm_callback_t plug_comm_callback;    // module callback function
        } c;
    } callback;

    // 这里的pre和post是指在模块执行时，会根据结构中的weight权重进行排序，从大到小执行顺序
    struct xlibman_pre_or_post_module_t pre_list[MAX_PRE_POST_MODULE_NUM];          // list of pre modules 前置模块列表
    struct xlibman_pre_or_post_module_t post_list[MAX_PRE_POST_MODULE_NUM];         // list of post modules 后置模块列表
    void *data;                         // module data  模块数据，由模块内部自己申请空间，在模块卸载时释放
    void *exts;                         // 后续需要扩展时留的指针
};


#endif /* XLIBMAN */
