#pragma once

#include "SQLiteCpp/SQLiteCpp.h"

// 初始化服务与测试共用的数据库结构。
bool InitializeDatabaseSchema(SQLite::Database& db);
