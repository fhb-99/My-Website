cmake_minimum_required(VERSION 3.16)

project(blog-server VERSION 1.0.0 LANGUAGES C CXX)

include(CTest)
option(BUILD_BLOG_SERVER "Build the HTTP server executable" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 将可执行文件输出到 backend-cpp/bin，方便 Linux 部署时直接运行。
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# SQLite3 底层 C 库。
add_library(sqlite3 STATIC third_party/sqlite3.c)
target_include_directories(sqlite3 PUBLIC third_party)
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_FTS5
)

# SQLiteCpp RAII 封装库，依赖 sqlite3。
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

if(BUILD_BLOG_SERVER)
    set(SOURCES
        main.cpp
        handlers/post_handler.cpp
        handlers/admin_content_handler.cpp
        handlers/public_content_handler.cpp
        repo/database_schema.cpp
        repo/post_repo_sqlite.cpp
        middleware/auth_token.cpp
        middleware/deepseek_moderation.cpp
    )

    add_executable(blog-server ${SOURCES})

    find_package(OpenSSL REQUIRED)

    target_compile_definitions(blog-server PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

    target_include_directories(blog-server PRIVATE
        .
        third_party
        third_party/SQLiteCpp/include
        models
        handlers
        middleware
        repo
    )

    target_link_libraries(blog-server PRIVATE SQLiteCpp sqlite3 OpenSSL::SSL OpenSSL::Crypto)

    if(WIN32)
        target_link_libraries(blog-server PRIVATE ws2_32)
    endif()

    if(UNIX)
        target_link_libraries(blog-server PRIVATE pthread dl)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(blog-server-public-content-test
        tests/public_content_repository_test.cpp
        repo/database_schema.cpp
        repo/post_repo_sqlite.cpp
    )
    target_include_directories(blog-server-public-content-test PRIVATE
        .
        third_party
        third_party/SQLiteCpp/include
        models
        repo
    )
    target_link_libraries(blog-server-public-content-test PRIVATE SQLiteCpp sqlite3)
    add_test(NAME public-content-repository COMMAND blog-server-public-content-test)

    add_executable(blog-server-public-content-handler-test
        tests/public_content_handler_test.cpp
        handlers/admin_content_handler.cpp
        handlers/public_content_handler.cpp
        repo/database_schema.cpp
        repo/post_repo_sqlite.cpp
    )
    target_include_directories(blog-server-public-content-handler-test PRIVATE
        .
        third_party
        third_party/SQLiteCpp/include
        models
        handlers
        repo
    )
    target_link_libraries(blog-server-public-content-handler-test PRIVATE SQLiteCpp sqlite3)
    add_test(NAME public-content-handler COMMAND blog-server-public-content-handler-test)
endif()
