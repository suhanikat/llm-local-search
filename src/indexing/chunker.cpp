#include "chunker.h"
#include <regex>
#include <iostream>

// Constructor — store chunk size and overlap
Chunker::Chunker(int chunkSize, int overlap)
    : chunkSize_(chunkSize), overlap_(overlap) {}

// ─────────────────────────────────────────
// CLEAN TEXT
// Removes extra whitespace and normalizes the text
// before we split it into chunks
// ─────────────────────────────────────────
std::string Chunker::cleanText(const std::string& text) {
    // Replace multiple spaces/newlines with single space
    std::regex extraWhitespace("\\s+");
    std::string cleaned = std::regex_replace(text, extraWhitespace, " ");

    // Trim leading and trailing spaces
    size_t start = cleaned.find_first_not_of(" ");
    size_t end = cleaned.find_last_not_of(" ");

    if (start == std::string::npos) return ""; // empty string
    return cleaned.substr(start, end - start + 1);
}

// ─────────────────────────────────────────
// CHUNK
// Main function — splits text into overlapping chunks
// 
// Example with chunkSize=500, overlap=100:
// Chunk 1: chars 0   → 500
// Chunk 2: chars 400 → 900   (starts 100 chars back = overlap)
// Chunk 3: chars 800 → 1300
// ─────────────────────────────────────────
std::vector<TextChunk> Chunker::chunk(const std::string& text,
                                       const std::string& sourceFile) {
    std::vector<TextChunk> chunks;

    // Clean the text first
    std::string cleanedText = cleanText(text);

    // If text is empty or too small, return as single chunk
    if (cleanedText.empty()) return chunks;
    if ((int)cleanedText.size() <= chunkSize_) {
        TextChunk single;
        single.text = cleanedText;
        single.sourceFile = sourceFile;
        single.chunkIndex = 0;
        single.startChar = 0;
        single.endChar = cleanedText.size();
        chunks.push_back(single);
        return chunks;
    }

    int start = 0;
    int chunkIndex = 0;
    int textLength = cleanedText.size();

    while (start < textLength) {
        // Calculate end of this chunk
        int end = start + chunkSize_;
        if (end > textLength) end = textLength;

        // Try to end at a sentence boundary (. ! ?)
        // so we don't cut a sentence in half
        if (end < textLength) {
            int searchFrom = end - 50; // look back 50 chars for punctuation
            if (searchFrom < start) searchFrom = start;

            for (int i = end; i > searchFrom; i--) {
                if (cleanedText[i] == '.' ||
                    cleanedText[i] == '!' ||
                    cleanedText[i] == '?') {
                    end = i + 1; // include the punctuation
                    break;
                }
            }
        }

        // Create the chunk
        TextChunk chunk;
        chunk.text = cleanedText.substr(start, end - start);
        chunk.sourceFile = sourceFile;
        chunk.chunkIndex = chunkIndex;
        chunk.startChar = start;
        chunk.endChar = end;
        chunks.push_back(chunk);

        // Move start forward by (chunkSize - overlap)
        // this creates the overlapping effect
        start += (chunkSize_ - overlap_);
        chunkIndex++;
    }

    return chunks;
}