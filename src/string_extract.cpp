#include "string_extract.h"
#include <algorithm>
#include <unordered_set>

static bool is_printable(uint8_t b) {
    return b >= 0x20 && b <= 0x7E;
}

std::vector<ExtractedString> extract_ascii_strings(const uint8_t* data, size_t size,
                                                   size_t min_len) {
    std::vector<ExtractedString> result;
    size_t start = 0;
    bool in_string = false;
    for (size_t i = 0; i < size; ++i) {
        if (is_printable(data[i])) {
            if (!in_string) { start = i; in_string = true; }
        } else {
            if (in_string) {
                size_t len = i - start;
                if (len >= min_len) {
                    ExtractedString es;
                    es.text.assign(reinterpret_cast<const char*>(data + start), len);
                    es.offset = start;
                    es.is_utf16 = false;
                    result.push_back(std::move(es));
                }
                in_string = false;
            }
        }
    }
    if (in_string && (size - start) >= min_len) {
        ExtractedString es;
        es.text.assign(reinterpret_cast<const char*>(data + start), size - start);
        es.offset = start;
        es.is_utf16 = false;
        result.push_back(std::move(es));
    }
    return result;
}

std::vector<ExtractedString> extract_utf16_strings(const uint8_t* data, size_t size,
                                                   size_t min_len) {
    std::vector<ExtractedString> result;
    if (size < 2) return result;
    size_t start = 0;
    bool in_string = false;
    size_t char_count = 0;

    for (size_t i = 0; i + 1 < size; i += 2) {
        uint8_t lo = data[i];
        uint8_t hi = data[i + 1];
        if (is_printable(lo) && hi == 0x00) {
            if (!in_string) { start = i; in_string = true; char_count = 0; }
            ++char_count;
        } else {
            if (in_string && char_count >= min_len) {
                ExtractedString es;
                es.offset = start;
                es.is_utf16 = true;
                for (size_t j = start; j < i; j += 2)
                    es.text += static_cast<char>(data[j]);
                result.push_back(std::move(es));
            }
            in_string = false;
            char_count = 0;
        }
    }
    if (in_string && char_count >= min_len) {
        ExtractedString es;
        es.offset = start;
        es.is_utf16 = true;
        for (size_t j = start; j + 1 < size; j += 2)
            es.text += static_cast<char>(data[j]);
        result.push_back(std::move(es));
    }
    return result;
}

std::vector<ExtractedString> extract_all_strings(const uint8_t* data, size_t size,
                                                 size_t min_len) {
    auto ascii = extract_ascii_strings(data, size, min_len);
    auto utf16 = extract_utf16_strings(data, size, min_len);

    std::unordered_set<std::string> seen;
    std::vector<ExtractedString> out;
    out.reserve(ascii.size() + utf16.size());
    for (auto& s : ascii) {
        if (seen.insert(s.text).second)
            out.push_back(std::move(s));
    }
    for (auto& s : utf16) {
        if (seen.insert(s.text).second)
            out.push_back(std::move(s));
    }
    return out;
}
