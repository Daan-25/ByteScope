#pragma once
#include "metrics.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

struct DiffSummary {
    size_t old_size = 0;
    size_t new_size = 0;
    std::string old_sha256;
    std::string new_sha256;
    bool   match = false;
    size_t differing_bytes = 0;
};

struct ChangedBlock {
    size_t index = 0;
    size_t offset = 0;
    float  diff_ratio = 0.f;
};

// Count total differing bytes between two buffers.
size_t count_diff_bytes(const uint8_t* a, size_t aSize,
                        const uint8_t* b, size_t bSize);

// Get top N most-changed blocks sorted by diff_ratio descending.
std::vector<ChangedBlock> top_changed_blocks(const std::vector<BlockMetrics>& metrics,
                                             size_t topN = 10);

// Export diff ranges to a text file.
bool export_diff_ranges(const std::string& path,
                        const std::vector<BlockMetrics>& metrics,
                        size_t block_size);
