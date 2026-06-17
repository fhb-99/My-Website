#pragma once

#include <cstddef>
#include <string>

class Authorization
{
public:
    Authorization() = default;

    // 生成后台登录 token。默认 32 字节熵，输出 64 位十六进制字符串。
    static std::string GenerateToken(std::size_t byte_count = 32);

private:
    static unsigned char GenerateRandomByte();
};
