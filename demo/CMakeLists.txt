cmake_minimum_required(VERSION 3.0)

project(demo)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES test.c ../xlibman.c module_parse_json.c yyjson.c module_login_auth.c)
add_executable(demo ${SOURCE_FILES})