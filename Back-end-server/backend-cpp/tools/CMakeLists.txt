# tools/CMakeLists.txt — 独立工具的构建配置
cmake_minimum_required(VERSION 3.16)

# 复用主项目的 SQLite3 + SQLiteCpp 库
add_executable(import_post import_post.cc)
target_include_directories(import_post PRIVATE .. ../third_party ../third_party/SQLiteCpp/include)
target_link_libraries(import_post PRIVATE sqlite3 SQLiteCpp)

if(WIN32)
    target_link_libraries(import_post PRIVATE ws2_32)
endif()
