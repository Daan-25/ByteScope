#include "sha256.h"
#include <cstring>

static inline uint32_t rotr(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }
static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z)  { return (x & y) ^ (~x & z); }
static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
static inline uint32_t ep0(uint32_t x)  { return rotr(x, 2)  ^ rotr(x, 13) ^ rotr(x, 22); }
static inline uint32_t ep1(uint32_t x)  { return rotr(x, 6)  ^ rotr(x, 11) ^ rotr(x, 25); }
static inline uint32_t sig0(uint32_t x) { return rotr(x, 7)  ^ rotr(x, 18) ^ (x >> 3); }
static inline uint32_t sig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

SHA256::SHA256() : m_total(0), m_buflen(0) {
    m_state[0] = 0x6a09e667; m_state[1] = 0xbb67ae85;
    m_state[2] = 0x3c6ef372; m_state[3] = 0xa54ff53a;
    m_state[4] = 0x510e527f; m_state[5] = 0x9b05688c;
    m_state[6] = 0x1f83d9ab; m_state[7] = 0x5be0cd19;
}

void SHA256::transform(const uint8_t block[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = (uint32_t(block[i*4])<<24) | (uint32_t(block[i*4+1])<<16) |
               (uint32_t(block[i*4+2])<<8) | uint32_t(block[i*4+3]);
    for (int i = 16; i < 64; ++i)
        w[i] = sig1(w[i-2]) + w[i-7] + sig0(w[i-15]) + w[i-16];

    uint32_t a=m_state[0],b=m_state[1],c=m_state[2],d=m_state[3];
    uint32_t e=m_state[4],f=m_state[5],g=m_state[6],h=m_state[7];

    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + ep1(e) + ch(e,f,g) + K[i] + w[i];
        uint32_t t2 = ep0(a) + maj(a,b,c);
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    m_state[0]+=a; m_state[1]+=b; m_state[2]+=c; m_state[3]+=d;
    m_state[4]+=e; m_state[5]+=f; m_state[6]+=g; m_state[7]+=h;
}

void SHA256::update(const uint8_t* data, size_t len) {
    m_total += len;
    while (len > 0) {
        size_t space = 64 - m_buflen;
        size_t copy = (len < space) ? len : space;
        std::memcpy(m_buf + m_buflen, data, copy);
        m_buflen += copy; data += copy; len -= copy;
        if (m_buflen == 64) { transform(m_buf); m_buflen = 0; }
    }
}

std::array<uint8_t,32> SHA256::finalize() {
    uint64_t bits = m_total * 8;
    uint8_t pad = 0x80;
    update(&pad, 1);
    pad = 0;
    while (m_buflen != 56) update(&pad, 1);
    uint8_t len_be[8];
    for (int i = 7; i >= 0; --i) { len_be[i] = uint8_t(bits); bits >>= 8; }
    update(len_be, 8);

    std::array<uint8_t,32> out;
    for (int i = 0; i < 8; ++i) {
        out[i*4+0] = uint8_t(m_state[i] >> 24);
        out[i*4+1] = uint8_t(m_state[i] >> 16);
        out[i*4+2] = uint8_t(m_state[i] >> 8);
        out[i*4+3] = uint8_t(m_state[i]);
    }
    return out;
}

std::string SHA256::hash_to_hex(const std::array<uint8_t,32>& h) {
    static const char* hex = "0123456789abcdef";
    std::string s;
    s.reserve(64);
    for (auto b : h) { s += hex[b >> 4]; s += hex[b & 0xf]; }
    return s;
}
