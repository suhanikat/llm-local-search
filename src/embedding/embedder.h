#pragma once
#include <string>
#include <vector>

// Holds the result of embedding a single text chunk
struct EmbeddingResult {
    std::string text;                    // original text chunk
    std::vector<float> embedding;        // 768 numbers representing meaning
    bool success;
    std::string error;
};

class Embedder {
public:
    // Constructor — takes the ollama API URL
    Embedder(const std::string& ollamaUrl = "http://localhost:11434");

    // Takes a text chunk, sends to ollama, returns 768 numbers
    EmbeddingResult embed(const std::string& text);

    // Embed multiple chunks at once
    std::vector<EmbeddingResult> embedBatch(const std::vector<std::string>& chunks);

private:
    std::string ollamaUrl_;
    std::string modelName_ = "nomic-embed-text";

    // Makes the actual HTTP call to ollama
    std::string makeHttpRequest(const std::string& payload);

    // Parses the JSON response and extracts the 768 numbers
    std::vector<float> parseEmbedding(const std::string& response);
};