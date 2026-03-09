#include "file_scanner.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem; 

FileScanner::FileScanner(const std::string& rootPath)
    : rootPath_(rootPath) {}

std::vector<std::string> FileScanner::scan() {
    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(rootPath_)) {
            if (entry.is_regular_file()) {

                  std::string path = entry.path().string();
                // Skip build directories — cmake junk, not useful for LLM
              if (path.find("/build/") != std::string::npos) continue;
        
                // Skip hidden folders like .vscode, .git
                if (path.find("/.") != std::string::npos) continue;
                files.push_back(path);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    return files;
}
