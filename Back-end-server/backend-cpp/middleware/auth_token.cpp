#include "middleware/auth_token.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

typedef std::array<unsigned char, 32> Sha256Digest;

const std::uint32_t kSha256InitialHash[8] = {
    0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
    0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
};

const std::uint32_t kSha256RoundConstants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

std::random_device& RandomDevice()
{
    static std::random_device device;
    return device;
}

std::uint32_t RotateRight(std::uint32_t value, int bits)
{
    return (value >> bits) | (value << (32 - bits));
}

std::vector<unsigned char> StringToBytes(const std::string& value)
{
    return std::vector<unsigned char>(value.begin(), value.end());
}

unsigned char HexValue(char c)
{
    if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
    throw std::invalid_argument("invalid hex string");
}

std::vector<unsigned char> HexToBytes(const std::string& hex)
{
    if (hex.size() % 2 != 0) {
        throw std::invalid_argument("hex string length must be even");
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        bytes.push_back(static_cast<unsigned char>((HexValue(hex[i]) << 4) | HexValue(hex[i + 1])));
    }
    return bytes;
}

std::string BytesToHex(const unsigned char* data, std::size_t size)
{
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < size; ++i) {
        out << std::setw(2) << static_cast<int>(data[i]);
    }
    return out.str();
}

std::string BytesToHex(const std::vector<unsigned char>& bytes)
{
    if (bytes.empty()) return "";
    return BytesToHex(&bytes[0], bytes.size());
}

Sha256Digest Sha256(const std::vector<unsigned char>& input)
{
    std::vector<unsigned char> data(input);
    const std::uint64_t bit_len = static_cast<std::uint64_t>(data.size()) * 8U;

    data.push_back(0x80U);
    while ((data.size() % 64U) != 56U) {
        data.push_back(0U);
    }

    for (int i = 7; i >= 0; --i) {
        data.push_back(static_cast<unsigned char>((bit_len >> (i * 8)) & 0xffU));
    }

    std::uint32_t hash[8];
    std::copy(kSha256InitialHash, kSha256InitialHash + 8, hash);

    for (std::size_t offset = 0; offset < data.size(); offset += 64) {
        std::uint32_t words[64] = {0};

        for (int i = 0; i < 16; ++i) {
            const std::size_t j = offset + static_cast<std::size_t>(i) * 4U;
            words[i] =
                (static_cast<std::uint32_t>(data[j]) << 24) |
                (static_cast<std::uint32_t>(data[j + 1]) << 16) |
                (static_cast<std::uint32_t>(data[j + 2]) << 8) |
                static_cast<std::uint32_t>(data[j + 3]);
        }

        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = RotateRight(words[i - 15], 7) ^
                                     RotateRight(words[i - 15], 18) ^
                                     (words[i - 15] >> 3);
            const std::uint32_t s1 = RotateRight(words[i - 2], 17) ^
                                     RotateRight(words[i - 2], 19) ^
                                     (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }

        std::uint32_t a = hash[0];
        std::uint32_t b = hash[1];
        std::uint32_t c = hash[2];
        std::uint32_t d = hash[3];
        std::uint32_t e = hash[4];
        std::uint32_t f = hash[5];
        std::uint32_t g = hash[6];
        std::uint32_t h = hash[7];

        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + s1 + ch + kSha256RoundConstants[i] + words[i];
            const std::uint32_t s0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    Sha256Digest digest;
    for (int i = 0; i < 8; ++i) {
        digest[static_cast<std::size_t>(i) * 4U] = static_cast<unsigned char>((hash[i] >> 24) & 0xffU);
        digest[static_cast<std::size_t>(i) * 4U + 1U] = static_cast<unsigned char>((hash[i] >> 16) & 0xffU);
        digest[static_cast<std::size_t>(i) * 4U + 2U] = static_cast<unsigned char>((hash[i] >> 8) & 0xffU);
        digest[static_cast<std::size_t>(i) * 4U + 3U] = static_cast<unsigned char>(hash[i] & 0xffU);
    }
    return digest;
}

std::vector<unsigned char> HmacSha256(std::vector<unsigned char> key,
                                      const std::vector<unsigned char>& message)
{
    const std::size_t block_size = 64;
    if (key.size() > block_size) {
        Sha256Digest hashed_key = Sha256(key);
        key.assign(hashed_key.begin(), hashed_key.end());
    }
    key.resize(block_size, 0);

    std::vector<unsigned char> outer_key(block_size);
    std::vector<unsigned char> inner_key(block_size);
    for (std::size_t i = 0; i < block_size; ++i) {
        outer_key[i] = static_cast<unsigned char>(key[i] ^ 0x5cU);
        inner_key[i] = static_cast<unsigned char>(key[i] ^ 0x36U);
    }

    std::vector<unsigned char> inner(inner_key);
    inner.insert(inner.end(), message.begin(), message.end());
    Sha256Digest inner_digest = Sha256(inner);

    std::vector<unsigned char> outer(outer_key);
    outer.insert(outer.end(), inner_digest.begin(), inner_digest.end());
    Sha256Digest digest = Sha256(outer);
    return std::vector<unsigned char>(digest.begin(), digest.end());
}

std::vector<unsigned char> Pbkdf2Sha256(const std::string& password,
                                        const std::vector<unsigned char>& salt,
                                        int iterations,
                                        std::size_t output_len)
{
    if (iterations <= 0 || output_len == 0) {
        throw std::invalid_argument("invalid pbkdf2 parameters");
    }

    const std::vector<unsigned char> password_bytes = StringToBytes(password);
    const std::size_t digest_len = 32;
    const std::size_t block_count = (output_len + digest_len - 1U) / digest_len;
    std::vector<unsigned char> output;
    output.reserve(block_count * digest_len);

    for (std::size_t block = 1; block <= block_count; ++block) {
        std::vector<unsigned char> block_salt(salt);
        block_salt.push_back(static_cast<unsigned char>((block >> 24) & 0xffU));
        block_salt.push_back(static_cast<unsigned char>((block >> 16) & 0xffU));
        block_salt.push_back(static_cast<unsigned char>((block >> 8) & 0xffU));
        block_salt.push_back(static_cast<unsigned char>(block & 0xffU));

        std::vector<unsigned char> u = HmacSha256(password_bytes, block_salt);
        std::vector<unsigned char> t(u);

        for (int i = 1; i < iterations; ++i) {
            u = HmacSha256(password_bytes, u);
            for (std::size_t j = 0; j < t.size(); ++j) {
                t[j] = static_cast<unsigned char>(t[j] ^ u[j]);
            }
        }

        output.insert(output.end(), t.begin(), t.end());
    }

    output.resize(output_len);
    return output;
}

bool ConstantTimeEquals(const std::string& left, const std::string& right)
{
    if (left.size() != right.size()) {
        return false;
    }

    unsigned char diff = 0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        diff = static_cast<unsigned char>(diff | (left[i] ^ right[i]));
    }
    return diff == 0;
}

} // namespace

std::string Authorization::GenerateToken(std::size_t byte_count)
{
    if (byte_count == 0) {
        throw std::invalid_argument("token byte count must be greater than zero");
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(byte_count);
    for (std::size_t i = 0; i < byte_count; ++i) {
        bytes.push_back(GenerateRandomByte());
    }
    return BytesToHex(bytes);
}

std::string Authorization::HashToken(const std::string& token)
{
    const std::vector<unsigned char> bytes = StringToBytes(token);
    const Sha256Digest digest = Sha256(bytes);
    return BytesToHex(digest.data(), digest.size());
}

bool Authorization::VerifyPassword(const std::string& password,
                                   const std::string& salt_hex,
                                   int iterations,
                                   const std::string& expected_hash_hex)
{
    try {
        const std::vector<unsigned char> salt = HexToBytes(salt_hex);
        const std::vector<unsigned char> expected = HexToBytes(expected_hash_hex);
        const std::vector<unsigned char> actual = Pbkdf2Sha256(password, salt, iterations, expected.size());
        return ConstantTimeEquals(BytesToHex(actual), expected_hash_hex);
    } catch (const std::exception&) {
        return false;
    }
}

unsigned char Authorization::GenerateRandomByte()
{
    // std::random_device 在 Linux 上通常来自系统随机源，适合生成登录 token。
    return static_cast<unsigned char>(RandomDevice()() & 0xffU);
}
