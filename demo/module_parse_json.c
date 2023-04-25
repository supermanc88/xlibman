//
// Created by 程贺明 on 2023/4/24.
//

#include "module_parse_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>


#include "yyjson.h"



int parse_json_module_callback_t(_in void *in_data, _out void **out_data, _in _out int *out_len, _in _out void *module_data)
{
    int ret = 0;

    struct user_info_t *user_info = malloc(sizeof(struct user_info_t));

    yyjson_doc *doc = yyjson_read(in_data, strlen(in_data), 0);

    yyjson_val *root = yyjson_doc_get_root(doc);

    yyjson_val *header = yyjson_obj_get(root, "Header");
    yyjson_val *body = yyjson_obj_get(root, "Body");

    yyjson_val *version = yyjson_obj_get(header, "Version");
    yyjson_val *user = yyjson_obj_get(header, "User");
    yyjson_val *announce = yyjson_obj_get(header, "Announce");
    yyjson_val *session_id = yyjson_obj_get(header, "SessionID");
    yyjson_val *request_type = yyjson_obj_get(header, "RequestType");

    yyjson_val *processor = yyjson_obj_get(body, "Processor");

    user_info->version = yyjson_get_int(version);
    strcpy(user_info->user, yyjson_get_str(user));
    strcpy(user_info->announce, yyjson_get_str(announce));
    strcpy(user_info->session_id, yyjson_get_str(session_id));
    strcpy(user_info->request_type, yyjson_get_str(request_type));
    strcpy(user_info->processor, yyjson_get_str(processor));


    yyjson_doc_free(doc);

    *out_data = user_info;
    *out_len = sizeof(struct user_info_t);
    return ret;
}
