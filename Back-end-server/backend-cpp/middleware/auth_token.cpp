#include "middleware/auth_token.h"

#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {

std::random_device& RandomDevice()
{
    static std::random_device device;
    return device;
}

} // namespace

std::string Authorization::GenerateToken(std::size_t byte_count)
{
    if (byte_count == 0) {
        throw std::invalid_argument("token byte count must be greater than zero");
    }

    std::ostringstream token;
    token << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < byte_count; ++i) {
        token << std::setw(2) << static_cast<int>(GenerateRandomByte());
    }

    return token.str();
}

unsigned char Authorization::GenerateRandomByte()
{
    // std::random_device 在 Linux 上通常来自系统随机源，适合生成登录 token。
    return static_cast<unsigned char>(RandomDevice()() & 0xff);
}
