#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <thread>

// Holds a loaded file's data and metadata.
struct FileData {
    std::string          path;
    std::vector<uint8_t> data;
    size_t               size = 0;
    std::string          sha256_hex;
};

// Background loader that keeps the UI responsive.
class FileLoader {
public:
    FileLoader();
    ~FileLoader();

    // Request loading a file (non-blocking). slot = 0 (old) or 1 (new).
    void request_load(const std::string& path, int slot);

    // Poll result. Returns true when the load for slot finished since last check.
    bool poll_result(int slot, FileData& out);

    // Progress 0.0 .. 1.0 for each slot.
    float progress(int slot) const;

    // Is a load in progress?
    bool loading(int slot) const;

    // Error message (empty if no error).
    std::string error(int slot) const;

private:
    void worker_func(int slot, std::string path);

    struct SlotState {
        std::atomic<bool>  busy{false};
        std::atomic<float> progress{0.f};
        std::mutex         mtx;
        bool               ready = false;
        FileData           result;
        std::string        error_msg;
    };
    SlotState m_slots[2];
    std::thread m_threads[2];
};
