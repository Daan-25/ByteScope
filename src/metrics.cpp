#include "metrics.h"
#include <algorithm>

static float shannon_entropy(const uint8_t* data, size_t len) {
    if (len == 0) return 0.f;
    size_t hist[256] = {};
    for (size_t i = 0; i < len; ++i) hist[data[i]]++;
    float ent = 0.f;
    for (int i = 0; i < 256; ++i) {
        if (hist[i] == 0) continue;
        float p = float(hist[i]) / float(len);
        ent -= p * std::log2(p);
    }
    return ent;
}

static float average_byte(const uint8_t* data, size_t len) {
    if (len == 0) return 0.f;
    uint64_t sum = 0;
    for (size_t i = 0; i < len; ++i) sum += data[i];
    return float(sum) / float(len);
}

std::vector<BlockMetrics> compute_metrics(const uint8_t* data, size_t size,
                                          size_t block_size) {
    if (block_size == 0) block_size = 256;
    size_t n = (size + block_size - 1) / block_size;
    std::vector<BlockMetrics> out(n);
    for (size_t i = 0; i < n; ++i) {
        size_t off = i * block_size;
        size_t len = std::min(block_size, size - off);
        out[i].offset   = off;
        out[i].length   = len;
        out[i].entropy  = shannon_entropy(data + off, len);
        out[i].avg_byte = average_byte(data + off, len);
    }
    return out;
}

std::vector<BlockMetrics> compute_diff_metrics(const uint8_t* oldData, size_t oldSize,
                                               const uint8_t* newData, size_t newSize,
                                               size_t block_size) {
    if (block_size == 0) block_size = 256;
    size_t maxSize = std::max(oldSize, newSize);
    size_t n = (maxSize + block_size - 1) / block_size;
    std::vector<BlockMetrics> out(n);
    for (size_t i = 0; i < n; ++i) {
        size_t off = i * block_size;
        size_t len = std::min(block_size, maxSize - off);
        out[i].offset = off;
        out[i].length = len;

        // Entropy from old file
        if (off < oldSize) {
            size_t olen = std::min(block_size, oldSize - off);
            out[i].entropy  = shannon_entropy(oldData + off, olen);
            out[i].avg_byte = average_byte(oldData + off, olen);
        }

        // Diff ratio
        size_t diffs = 0;
        for (size_t j = 0; j < len; ++j) {
            uint8_t a = (off + j < oldSize) ? oldData[off + j] : 0;
            uint8_t b = (off + j < newSize) ? newData[off + j] : 0;
            if (a != b) ++diffs;
        }
        out[i].diff_ratio = (len > 0) ? float(diffs) / float(len) : 0.f;
    }
    return out;
}
