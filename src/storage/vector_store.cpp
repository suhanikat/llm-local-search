#include "vector_store.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <unordered_set>


VectorStore::VectorStore(const std::string& dbPath) : dbPath_(dbPath) {}

// ─────────────────────────────────────────
// ADD ENTRY
// Adds a chunk + its vector to memory
// ─────────────────────────────────────────
void VectorStore::addEntry(const std::string& filePath,
                            const std::string& chunkText,
                            const std::vector<float>& embedding,
                            int chunkIndex) {
    VectorEntry entry;
    entry.filePath = filePath;
    entry.chunkText = chunkText;
    entry.embedding = embedding;
    entry.chunkIndex = chunkIndex;
    entries_.push_back(entry);
}

// ─────────────────────────────────────────
// SEARCH
// Takes query vector, compares with every stored vector
// Returns top K most similar results
// ─────────────────────────────────────────
std::vector<SearchResult> VectorStore::search(const std::vector<float>& queryEmbedding,
                                               int topK) {
    std::vector<SearchResult> allResults;

    // Compare query vector against every stored entry
    for (const auto& entry : entries_) {
        float similarity = cosineSimilarity(queryEmbedding, entry.embedding);

        SearchResult result;
        result.filePath = entry.filePath;
        result.chunkText = entry.chunkText;
        result.similarity = similarity;
        result.chunkIndex = entry.chunkIndex;
        allResults.push_back(result);
    }

    // Sort by similarity — highest first
    std::sort(allResults.begin(), allResults.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.similarity > b.similarity;
        });

    // ─────────────────────────────────────────
    // DEDUPLICATE BY FILE PATH
    // If same file appears multiple times,
    // keep only the highest scoring chunk
    // since results are already sorted, first
    // occurrence of each file is the best one
    // ─────────────────────────────────────────
    std::vector<SearchResult> deduplicated;
    std::unordered_set<std::string> seenFiles;

    for (const auto& result : allResults) {
        // If we haven't seen this file yet, add it
        if (seenFiles.find(result.filePath) == seenFiles.end()) {
            seenFiles.insert(result.filePath);
            deduplicated.push_back(result);
        }
        // If file already seen, skip — we already have
        // a higher scoring chunk from this file
    }

    // Return only top K results
    if (deduplicated.size() > topK) {
        deduplicated.resize(topK);
    }

    return deduplicated;
}

// ─────────────────────────────────────────
// COSINE SIMILARITY
// Measures angle between two vectors
// Returns 1.0 for identical, 0.0 for completely different
// ─────────────────────────────────────────
float VectorStore::cosineSimilarity(const std::vector<float>& a,
                                     const std::vector<float>& b) {
    if (a.size() != b.size()) return 0.0f;

    float dotProduct = 0.0f;   // A · B
    float magnitudeA = 0.0f;   // |A|
    float magnitudeB = 0.0f;   // |B|

    // Calculate all three in one loop for efficiency
    for (size_t i = 0; i < a.size(); i++) {
        dotProduct += a[i] * b[i];      // multiply each pair
        magnitudeA += a[i] * a[i];      // square each number
        magnitudeB += b[i] * b[i];
    }

    // Square root to get actual magnitude
    magnitudeA = std::sqrt(magnitudeA);
    magnitudeB = std::sqrt(magnitudeB);

    // Avoid division by zero
    if (magnitudeA == 0.0f || magnitudeB == 0.0f) return 0.0f;

    return dotProduct / (magnitudeA * magnitudeB);
}

// ─────────────────────────────────────────
// SAVE TO DISK
// Writes all vectors to a binary file
// Format per entry:
//   [pathLen][path][textLen][text][chunkIndex][768 floats]
// ─────────────────────────────────────────
void VectorStore::saveToDisk() {
    std::ofstream file(dbPath_, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Could not open " << dbPath_ << " for writing\n";
        return;
    }

    // Write number of entries first
    size_t count = entries_.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& entry : entries_) {
        // Write file path
        size_t pathLen = entry.filePath.size();
        file.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
        file.write(entry.filePath.c_str(), pathLen);

        // Write chunk text
        size_t textLen = entry.chunkText.size();
        file.write(reinterpret_cast<const char*>(&textLen), sizeof(textLen));
        file.write(entry.chunkText.c_str(), textLen);

        // Write chunk index
        file.write(reinterpret_cast<const char*>(&entry.chunkIndex),
                   sizeof(entry.chunkIndex));

        // Write 768 floats
        size_t embSize = entry.embedding.size();
        file.write(reinterpret_cast<const char*>(&embSize), sizeof(embSize));
        file.write(reinterpret_cast<const char*>(entry.embedding.data()),
                   embSize * sizeof(float));
    }

    file.close();
    std::cout << "Saved " << count << " vectors to " << dbPath_ << "\n";
}

// ─────────────────────────────────────────
// LOAD FROM DISK
// Reads vectors.db back into memory on startup
// So you don't have to re-embed everything each run
// ─────────────────────────────────────────
void VectorStore::loadFromDisk() {
    std::ifstream file(dbPath_, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "No existing vector store found — starting fresh\n";
        return;
    }

    // Read number of entries
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (size_t i = 0; i < count; i++) {
        VectorEntry entry;

        // Read file path
        size_t pathLen;
        file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
        entry.filePath.resize(pathLen);
        file.read(&entry.filePath[0], pathLen);

        // Read chunk text
        size_t textLen;
        file.read(reinterpret_cast<char*>(&textLen), sizeof(textLen));
        entry.chunkText.resize(textLen);
        file.read(&entry.chunkText[0], textLen);

        // Read chunk index
        file.read(reinterpret_cast<char*>(&entry.chunkIndex),
                  sizeof(entry.chunkIndex));

        // Read 768 floats
        size_t embSize;
        file.read(reinterpret_cast<char*>(&embSize), sizeof(embSize));
        entry.embedding.resize(embSize);
        file.read(reinterpret_cast<char*>(entry.embedding.data()),
                  embSize * sizeof(float));

        entries_.push_back(entry);
    }

    file.close();
    std::cout << "Loaded " << count << " vectors from " << dbPath_ << "\n";
}