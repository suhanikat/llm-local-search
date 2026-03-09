#pragma once
#include <string>
#include <vector>

// Represents one stored entry
// Every chunk of every file gets one of these
struct VectorEntry {
    std::string filePath;           // which file this chunk came from
    std::string chunkText;          // the actual text of this chunk
    std::vector<float> embedding;   // 768 numbers representing meaning
    int chunkIndex;                 // which chunk number in the file
};

// Represents a search result returned to the user
struct SearchResult {
    std::string filePath;           // which file matched
    std::string chunkText;          // the matching text
    float similarity;               // how close 0.0 to 1.0
    int chunkIndex;                 // which chunk in the file
};

class VectorStore {
public:
    // Constructor — takes path where vectors.db will be saved
    VectorStore(const std::string& dbPath = "vectors.db");

    // Add a single entry to the store
    void addEntry(const std::string& filePath,
                  const std::string& chunkText,
                  const std::vector<float>& embedding,
                  int chunkIndex);

    // Search — takes a query vector, returns top N matches
    std::vector<SearchResult> search(const std::vector<float>& queryEmbedding,
                                      int topK = 5);

    // Save all vectors to disk so they persist between runs
    void saveToDisk();

    // Load vectors from disk on startup
    void loadFromDisk();

    // How many entries are stored
    size_t size() const { return entries_.size(); }

private:
    std::string dbPath_;
    std::vector<VectorEntry> entries_;  // all stored vectors in memory

    // The core math — computes similarity between two vectors
    float cosineSimilarity(const std::vector<float>& a,
                           const std::vector<float>& b);
};