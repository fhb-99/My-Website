#pragma once

#include "auth_token.h"

#include <cstddef>
#include <string>

class Authorization
{
public:
    Authorization() = default;

    // 生成后台登录 token。默认 32 字节熵，输出 64 位十六进制字符串。
    static std::string GenerateToken(std::size_t byte_count = 32);

    // token 原文只给前端保存；数据库只保存 SHA-256 哈希，避免数据库泄露后 token 可直接使用。
    static std::string HashToken(const std::string& token);

    // 校验 init_admin.py 生成的 PBKDF2-SHA256 密码哈希。
    static bool VerifyPassword(const std::string& password,
                               const std::string& salt_hex,
                               int iterations,
                               const std::string& expected_hash_hex);

private:
    static unsigned char GenerateRandomByte();
};
