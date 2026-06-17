cmake_minimum_required(VERSION 3.16)
# 关键1：同时启用C和C++语言（必须，因为sqlite3是C库）
project(blog-server VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 关键2：设置可执行文件输出到项目根目录下的bin目录
# CMAKE_SOURCE_DIR 是CMakeLists.txt所在的目录（项目根目录）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
# 确保bin目录存在，避免编译失败
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# SQLite3 底层引擎（C语言库）
add_library(sqlite3 STATIC third_party/sqlite3.c)
target_include_directories(sqlite3 PUBLIC third_party)
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_FTS5
)

# SQLiteCpp RAII 封装（C++库，依赖sqlite3）
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

# 添加源文件
set(SOURCES
    main.cpp
    handlers/post_handler.cpp
    repo/post_repo_sqlite.cpp
    middleware/auth_token.cpp
)

# 主程序
add_executable(blog-server ${SOURCES})

target_include_directories(blog-server PRIVATE . third_party
                                                third_party/SQLiteCpp/include
                                                models
                                                handlers
                                                repo
                                                middleware)

target_link_libraries(blog-server PRIVATE SQLiteCpp sqlite3)

if(WIN32)
    target_link_libraries(blog-server PRIVATE ws2_32)
endif()

if(UNIX)
    target_link_libraries(blog-server PRIVATE pthread dl)
endif()
