#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <cmath>

// Per-block metrics.
struct BlockMetrics {
    size_t offset    = 0;   // byte offset in file
    size_t length    = 0;   // actual bytes in block (may be < block_size for last)
    float  entropy   = 0.f; // Shannon entropy 0..8
    float  avg_byte  = 0.f; // average byte value 0..255
    float  diff_ratio= 0.f; // 0..1 fraction of differing bytes (diff mode only)
};

// Compute metrics for all blocks in one file.
std::vector<BlockMetrics> compute_metrics(const uint8_t* data, size_t size,
                                          size_t block_size);

// Compute diff ratios between two files (block-aligned comparison).
// Updates diff_ratio field in the returned vector (based on oldData metrics structure).
std::vector<BlockMetrics> compute_diff_metrics(const uint8_t* oldData, size_t oldSize,
                                               const uint8_t* newData, size_t newSize,
                                               size_t block_size);
