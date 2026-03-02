#include "diff.h"
#include <algorithm>
#include <fstream>
#include <cstdio>

size_t count_diff_bytes(const uint8_t* a, size_t aSize,
                        const uint8_t* b, size_t bSize) {
    size_t maxSz = std::max(aSize, bSize);
    size_t diffs = 0;
    for (size_t i = 0; i < maxSz; ++i) {
        uint8_t va = (i < aSize) ? a[i] : 0;
        uint8_t vb = (i < bSize) ? b[i] : 0;
        if (va != vb) ++diffs;
    }
    return diffs;
}

std::vector<ChangedBlock> top_changed_blocks(const std::vector<BlockMetrics>& metrics,
                                             size_t topN) {
    std::vector<ChangedBlock> all;
    all.reserve(metrics.size());
    for (size_t i = 0; i < metrics.size(); ++i) {
        if (metrics[i].diff_ratio > 0.f) {
            all.push_back({i, metrics[i].offset, metrics[i].diff_ratio});
        }
    }
    std::sort(all.begin(), all.end(), [](const ChangedBlock& a, const ChangedBlock& b) {
        return a.diff_ratio > b.diff_ratio;
    });
    if (all.size() > topN) all.resize(topN);
    return all;
}

bool export_diff_ranges(const std::string& path,
                        const std::vector<BlockMetrics>& metrics,
                        size_t block_size) {
    std::ofstream ofs(path);
    if (!ofs) return false;
    ofs << "# ByteScope Diff Export\n";
    ofs << "# block_size=" << block_size << "\n";
    ofs << "# block_index, offset_start, offset_end, diff_ratio\n";
    for (size_t i = 0; i < metrics.size(); ++i) {
        if (metrics[i].diff_ratio > 0.f) {
            ofs << i << ", 0x"
                << std::hex << metrics[i].offset << ", 0x"
                << (metrics[i].offset + metrics[i].length) << std::dec
                << ", " << metrics[i].diff_ratio << "\n";
        }
    }
    return true;
}
