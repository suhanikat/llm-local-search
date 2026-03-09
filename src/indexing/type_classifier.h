#pragma once
#include <string>

// All possible categories a file can belong to
enum class FileType {
    DOCUMENT,       // pdf, docx, txt, md
    CODE,           // cpp, h, py, js, ts
    IMAGE,          // jpg, png, gif, webp
    VIDEO,          // mp4, mov, avi
    AUDIO,          // mp3, wav
    ARCHIVE,        // zip, tar, rar
    BUILD_ARTIFACT, // o, cmake, make — junk for LLM
    UNKNOWN         // anything else
};

class TypeClassifier {
public:
    // Takes a file extension like ".pdf" and returns its category
    FileType classify(const std::string& extension);

    // Converts the FileType enum to a readable string like "DOCUMENT"
    std::string typeToString(FileType type);

    // Returns true if the LLM should process this file
    // false for images, videos, build artifacts etc
    bool isLLMReadable(FileType type);
};