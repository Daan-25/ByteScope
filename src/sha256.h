#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <array>

// Minimal dependency-free SHA-256 implementation.
class SHA256 {
public:
    SHA256();
    void update(const uint8_t* data, size_t len);
    std::array<uint8_t, 32> finalize();
    static std::string hash_to_hex(const std::array<uint8_t, 32>& h);
private:
    void transform(const uint8_t block[64]);
    uint64_t m_total;
    uint32_t m_state[8];
    uint8_t  m_buf[64];
    size_t   m_buflen;
};
