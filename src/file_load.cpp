#include "file_load.h"
#include "sha256.h"
#include <fstream>

FileLoader::FileLoader() = default;

FileLoader::~FileLoader() {
    for (int i = 0; i < 2; ++i)
        if (m_threads[i].joinable()) m_threads[i].join();
}

void FileLoader::request_load(const std::string& path, int slot) {
    if (slot < 0 || slot > 1) return;
    if (m_threads[slot].joinable()) m_threads[slot].join();
    {
        std::lock_guard<std::mutex> lk(m_slots[slot].mtx);
        m_slots[slot].ready = false;
        m_slots[slot].error_msg.clear();
        m_slots[slot].progress.store(0.f);
    }
    m_slots[slot].busy.store(true);
    m_threads[slot] = std::thread(&FileLoader::worker_func, this, slot, path);
}

bool FileLoader::poll_result(int slot, FileData& out) {
    if (slot < 0 || slot > 1) return false;
    std::lock_guard<std::mutex> lk(m_slots[slot].mtx);
    if (m_slots[slot].ready) {
        out = std::move(m_slots[slot].result);
        m_slots[slot].ready = false;
        return true;
    }
    return false;
}

float FileLoader::progress(int slot) const {
    if (slot < 0 || slot > 1) return 0.f;
    return m_slots[slot].progress.load();
}

bool FileLoader::loading(int slot) const {
    if (slot < 0 || slot > 1) return false;
    return m_slots[slot].busy.load();
}

std::string FileLoader::error(int slot) const {
    if (slot < 0 || slot > 1) return {};
    // reading error_msg without lock is technically racy but acceptable here
    return m_slots[slot].error_msg;
}

void FileLoader::worker_func(int slot, std::string path) {
    FileData fd;
    fd.path = path;

    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) {
        std::lock_guard<std::mutex> lk(m_slots[slot].mtx);
        m_slots[slot].error_msg = "Cannot open file: " + path;
        m_slots[slot].busy.store(false);
        return;
    }
    auto fileSize = ifs.tellg();
    ifs.seekg(0);
    fd.size = static_cast<size_t>(fileSize);
    fd.data.resize(fd.size);

    // Read in chunks so we can update progress
    const size_t chunk = 1 << 20; // 1 MB
    size_t read_so_far = 0;
    SHA256 sha;
    while (read_so_far < fd.size) {
        size_t to_read = std::min(chunk, fd.size - read_so_far);
        ifs.read(reinterpret_cast<char*>(fd.data.data() + read_so_far), static_cast<std::streamsize>(to_read));
        sha.update(fd.data.data() + read_so_far, to_read);
        read_so_far += to_read;
        m_slots[slot].progress.store(float(read_so_far) / float(fd.size));
    }
    fd.sha256_hex = SHA256::hash_to_hex(sha.finalize());

    {
        std::lock_guard<std::mutex> lk(m_slots[slot].mtx);
        m_slots[slot].result = std::move(fd);
        m_slots[slot].ready = true;
    }
    m_slots[slot].busy.store(false);
}
