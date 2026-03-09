#pragma once

#include <string>

struct FileMetadata {
    std::string path;
    std::string extension;
    uintmax_t size;        // in bytes
    std::string lastModified;
};

class MetadataExtractor {
public:
    FileMetadata extract(const std::string& filePath);
};