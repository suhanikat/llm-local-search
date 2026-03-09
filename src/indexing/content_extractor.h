#pragma once
#include <string>

// Holds the result of extracting content from any file
struct FileContent {
    std::string path;           // which file this came from
    std::string text;           // the actual extracted text
    bool success;               // did it work?
    std::string error;          // if not, what went wrong
};

class ContentExtractor {
public:
    // Main function — looks at extension and calls the right extractor
    FileContent extract(const std::string& filePath, const std::string& extension);

private:
    // Plain text files — .txt .md .csv .cpp .h etc
    FileContent extractPlainText(const std::string& filePath);

    // PDFs — uses poppler to decode binary format
    FileContent extractPDF(const std::string& filePath);

    // DOCX — uses libzip to open zip, then parses XML inside
    FileContent extractDOCX(const std::string& filePath);
};