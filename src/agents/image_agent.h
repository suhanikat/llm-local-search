#pragma once
#include <string>
#include <vector>

// Result of describing an image
struct ImageDescription {
    std::string filePath;
    std::string description;  // what LLaVA sees in the image
    bool success;
    std::string error;
};

class ImageAgent {
public:
    ImageAgent(const std::string& ollamaUrl = "http://localhost:11434");

    // Takes image path, returns description from LLaVA
    ImageDescription describe(const std::string& filePath);

private:
    std::string ollamaUrl_;

    // Read image file and convert to base64
    std::string imageToBase64(const std::string& filePath);

    // Make HTTP call to LLaVA
    std::string makeHttpRequest(const std::string& payload);

    // base64 encoding table
    static const std::string base64Chars_;
};