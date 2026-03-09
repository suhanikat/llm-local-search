#pragma once
#include <string>
#include <vector>

// Represents one chunk of text from a file
struct TextChunk {
    std::string text;           // the actual chunk content
    std::string sourceFile;     // which file this came from
    int chunkIndex;             // chunk number (0, 1, 2...)
    int startChar;              // where in original text this starts
    int endChar;                // where in original text this ends
};

class Chunker {
public:
    // Constructor — set chunk size and overlap
    // chunkSize: how many characters per chunk
    // overlap: how many characters repeat between chunks
    Chunker(int chunkSize = 500, int overlap = 100);

    // Takes full text and file path, returns vector of chunks
    std::vector<TextChunk> chunk(const std::string& text,
                                  const std::string& sourceFile);

private:
    int chunkSize_;
    int overlap_;

    // Cleans up text before chunking
    // removes extra whitespace, newlines etc
    std::string cleanText(const std::string& text);
};