#include "content_extractor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>

// Poppler for PDF extraction
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>

// Libzip for DOCX extraction
#include <zip.h>

// ─────────────────────────────────────────
// MAIN ENTRY POINT
// Looks at extension, decides which extractor to call
// ─────────────────────────────────────────
FileContent ContentExtractor::extract(const std::string& filePath,
                                       const std::string& extension) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".pdf") {
        return extractPDF(filePath);
    } else if (ext == ".docx") {
        return extractDOCX(filePath);
    } else {
        // .txt .md .csv .cpp .h etc — just read directly
        return extractPlainText(filePath);
    }
}

// ─────────────────────────────────────────
// PLAIN TEXT EXTRACTOR
// Just opens the file and reads everything into a string
// ─────────────────────────────────────────
FileContent ContentExtractor::extractPlainText(const std::string& filePath) {
    FileContent result;
    result.path = filePath;
    result.success = false;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        result.error = "Could not open file";
        return result;
    }

    // Read entire file into string
    std::ostringstream buffer;
    buffer << file.rdbuf();
    result.text = buffer.str();
    result.success = true;

    return result;
}

// ─────────────────────────────────────────
// PDF EXTRACTOR
// Poppler decodes the binary PDF, we loop page by page
// ─────────────────────────────────────────
FileContent ContentExtractor::extractPDF(const std::string& filePath) {
    FileContent result;
    result.path = filePath;
    result.success = false;

    // Load PDF document
    auto doc = poppler::document::load_from_file(filePath);
    if (!doc) {
        result.error = "Could not open PDF";
        return result;
    }

    std::string fullText;

    // Loop every page and extract text
    for (int i = 0; i < doc->pages(); i++) {
        auto page = doc->create_page(i);
        if (page) {
            poppler::byte_array bytes = page->text().to_utf8();
            fullText += std::string(bytes.begin(), bytes.end());
            fullText += "\n";
        }
    }

    result.text = fullText;
    result.success = true;
    return result;
}

// ─────────────────────────────────────────
// DOCX EXTRACTOR
// .docx is a zip file containing word/document.xml
// We unzip it, read the XML, strip tags → clean text
// ─────────────────────────────────────────
FileContent ContentExtractor::extractDOCX(const std::string& filePath) {
    FileContent result;
    result.path = filePath;
    result.success = false;

    // Open .docx as zip
    int zipError = 0;
    zip_t* archive = zip_open(filePath.c_str(), ZIP_RDONLY, &zipError);
    if (!archive) {
        result.error = "Could not open DOCX as zip";
        return result;
    }

    // Find word/document.xml inside the zip
    zip_file_t* xmlFile = zip_fopen(archive, "word/document.xml", 0);
    if (!xmlFile) {
        zip_close(archive);
        result.error = "Could not find document.xml inside DOCX";
        return result;
    }

    // Read XML content
    std::string xmlContent;
    char buffer[4096];
    zip_int64_t bytesRead;
    while ((bytesRead = zip_fread(xmlFile, buffer, sizeof(buffer))) > 0) {
        xmlContent.append(buffer, bytesRead);
    }

    zip_fclose(xmlFile);
    zip_close(archive);

    // Strip XML tags — <w:t>Hello World</w:t> → Hello World
    std::regex xmlTags("<[^>]+>");
    std::string cleanText = std::regex_replace(xmlContent, xmlTags, " ");

    // Remove HTML entities like &quot; &amp; &lt;
    std::regex htmlEntities("&[a-zA-Z0-9#]+;");
    cleanText = std::regex_replace(cleanText, htmlEntities, " ");

    // Remove pure number tokens (XML artifacts like 60325 60960)
    std::regex numberTokens("\\b\\d{4,}\\b");
    cleanText = std::regex_replace(cleanText, numberTokens, " ");

    // Clean up extra whitespace
    std::regex extraSpaces("\\s+");
    cleanText = std::regex_replace(cleanText, extraSpaces, " ");

    result.text = cleanText;
    result.success = true;
    return result;
}