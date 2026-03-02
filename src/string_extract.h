#pragma once
// Renamed from strings.h to avoid collision with system <strings.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

struct ExtractedString {
    std::string text;
    size_t      offset = 0;
    bool        is_utf16 = false;
};

// Extract ASCII printable strings (bytes 0x20..0x7E) of at least min_len.
std::vector<ExtractedString> extract_ascii_strings(const uint8_t* data, size_t size,
                                                   size_t min_len = 4);

// Extract UTF-16LE strings (low byte printable, high byte 0x00) of at least min_len chars.
std::vector<ExtractedString> extract_utf16_strings(const uint8_t* data, size_t size,
                                                   size_t min_len = 4);

// Extract both ASCII and UTF-16LE strings, deduplicated by (text).
std::vector<ExtractedString> extract_all_strings(const uint8_t* data, size_t size,
                                                 size_t min_len = 4);
