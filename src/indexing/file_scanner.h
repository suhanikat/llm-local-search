#pragma once

#include <string>
#include <vector>

class FileScanner {
public:
    explicit FileScanner(const std::string& rootPath);
    std::vector<std::string> scan();

private:
    std::string rootPath_;
};