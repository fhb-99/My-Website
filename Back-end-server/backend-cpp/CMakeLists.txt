cmake_minimum_required(VERSION 3.16)
project(blog-server VERSION 1.0.0 LANGUAGES C CXX)

# 全局C++17，所有target共用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# C库sqlite3不用C++标准，不动
add_library(sqlite3 STATIC third_party/sqlite3.c)
target_include_directories(sqlite3 PUBLIC third_party)
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_FTS5
)

# ==========关键：SQLiteCpp强制C++17==========
add_library(SQLiteCpp STATIC
    third_party/SQLiteCpp/src/Column.cpp
    third_party/SQLiteCpp/src/Database.cpp
    third_party/SQLiteCpp/src/Statement.cpp
    third_party/SQLiteCpp/src/Transaction.cpp
    third_party/SQLiteCpp/src/Backup.cpp
    third_party/SQLiteCpp/src/Exception.cpp
)
target_include_directories(SQLiteCpp PUBLIC third_party/SQLiteCpp/include)
target_link_libraries(SQLiteCpp PUBLIC sqlite3)
# 手动给SQLiteCpp追加编译参数
target_compile_features(SQLiteCpp PRIVATE cxx_std_17)

# 主程序不变
set(SOURCES
    main.cpp
    handlers/post_handler.cpp
    repo/post_repo_sqlite.cpp
    repo/post_repo.cpp
)
add_executable(blog-server ${SOURCES})
target_include_directories(blog-server PRIVATE . third_party 
                                                third_party/SQLiteCpp/include
                                                models
                                                handlers
                                                repo)
target_link_libraries(blog-server PRIVATE SQLiteCpp sqlite3)

if(WIN32)
    target_link_libraries(blog-server PRIVATE ws2_32)
endif()
if(UNIX)
    target_link_libraries(blog-server PRIVATE pthread dl)
endif()