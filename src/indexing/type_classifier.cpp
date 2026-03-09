#include "type_classifier.h"
#include <unordered_map>
#include <algorithm>

FileType TypeClassifier::classify(const std::string& extension) {

    // Convert extension to lowercase so .PDF and .pdf both work
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Map every known extension to its category
    static const std::unordered_map<std::string, FileType> extensionMap = {
        // Documents — LLM can read these
        {".pdf",  FileType::DOCUMENT},
        {".docx", FileType::DOCUMENT},
        {".txt",  FileType::DOCUMENT},
        {".md",   FileType::DOCUMENT},
        {".pptx", FileType::DOCUMENT},
        {".csv",  FileType::DOCUMENT},

        // Code files — LLM can read these
        {".cpp",  FileType::CODE},
        {".h",    FileType::CODE},
        {".py",   FileType::CODE},
        {".js",   FileType::CODE},
        {".ts",   FileType::CODE},
        {".java", FileType::CODE},
        {".go",   FileType::CODE},

        // Images — need vision LLM to describe
        {".jpg",  FileType::IMAGE},
        {".jpeg", FileType::IMAGE},
        {".png",  FileType::IMAGE},
        {".gif",  FileType::IMAGE},
        {".webp", FileType::IMAGE},

        // Videos — skip for now
        {".mp4",  FileType::VIDEO},
        {".mov",  FileType::VIDEO},
        {".avi",  FileType::VIDEO},

        // Audio — skip for now
        {".mp3",  FileType::AUDIO},
        {".wav",  FileType::AUDIO},

        // Archives — skip, cant read inside
        {".zip",  FileType::ARCHIVE},
        {".tar",  FileType::ARCHIVE},
        {".rar",  FileType::ARCHIVE},

        // Build artifacts — total junk, skip these
        {".o",      FileType::BUILD_ARTIFACT},
        {".cmake",  FileType::BUILD_ARTIFACT},
        {".make",   FileType::BUILD_ARTIFACT},
        {".a",      FileType::BUILD_ARTIFACT},
        {".dylib",  FileType::BUILD_ARTIFACT},
    };

    // Look up the extension in the map
    auto it = extensionMap.find(ext);
    if (it != extensionMap.end()) {
        return it->second; // found it, return the category
    }

    return FileType::UNKNOWN; // not in our map
}

std::string TypeClassifier::typeToString(FileType type) {
    // Convert enum to human readable string for printing
    switch (type) {
        case FileType::DOCUMENT:       return "DOCUMENT";
        case FileType::CODE:           return "CODE";
        case FileType::IMAGE:          return "IMAGE";
        case FileType::VIDEO:          return "VIDEO";
        case FileType::AUDIO:          return "AUDIO";
        case FileType::ARCHIVE:        return "ARCHIVE";
        case FileType::BUILD_ARTIFACT: return "BUILD_ARTIFACT";
        default:                       return "UNKNOWN";
    }
}

bool TypeClassifier::isLLMReadable(FileType type) {
    // Only documents and code are worth sending to the LLM
    // Everything else gets filtered out
    return type == FileType::DOCUMENT || type == FileType::CODE;
}