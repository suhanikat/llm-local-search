#include "metadata_extractor.h"
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

FileMetadata MetadataExtractor::extract(const std::string& filePath) {
    FileMetadata meta;
    meta.path = filePath;
    meta.extension = fs::path(filePath).extension().string();
    meta.size = fs::file_size(filePath);

    // Get last modified time
    auto ftime = fs::last_write_time(filePath);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    meta.lastModified = oss.str();

    return meta;
}