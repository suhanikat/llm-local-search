#include "embedder.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

// ─────────────────────────────────────────
// CURL CALLBACK
// curl needs this to collect the HTTP response into a string
// every time curl receives data it calls this function
// ─────────────────────────────────────────
static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Embedder::Embedder(const std::string& ollamaUrl) : ollamaUrl_(ollamaUrl) {}

// ─────────────────────────────────────────
// MAIN EMBED FUNCTION
// Takes text → sends to ollama → returns vector of 768 floats
// ─────────────────────────────────────────
EmbeddingResult Embedder::embed(const std::string& text) {
    EmbeddingResult result;
    result.text = text;
    result.success = false;

    // Build JSON payload manually
    // Ollama expects: {"model": "nomic-embed-text", "prompt": "your text"}
    std::string payload = "{\"model\": \"" + modelName_ + "\", \"prompt\": \"";

    // Escape special characters in text so JSON stays valid
    for (char c : text) {
        if (c == '"') payload += "\\\"";
        else if (c == '\n') payload += "\\n";
        else if (c == '\r') payload += "\\r";
        else if (c == '\\') payload += "\\\\";
        else payload += c;
    }
    payload += "\"}";

    // Make HTTP request to ollama
    std::string response = makeHttpRequest(payload);
    if (response.empty()) {
        result.error = "No response from Ollama — is it running?";
        return result;
    }

    // Parse the embedding from response
    result.embedding = parseEmbedding(response);
    if (result.embedding.empty()) {
        result.error = "Failed to parse embedding from response";
        return result;
    }

    result.success = true;
    return result;
}

// ─────────────────────────────────────────
// BATCH EMBED
// Just calls embed() for each chunk
// ─────────────────────────────────────────
std::vector<EmbeddingResult> Embedder::embedBatch(const std::vector<std::string>& chunks) {
    std::vector<EmbeddingResult> results;

    for (size_t i = 0; i < chunks.size(); i++) {
        std::cout << "Embedding chunk " << (i+1) << "/" << chunks.size() << "\r" << std::flush;
        results.push_back(embed(chunks[i]));
    }
    std::cout << "\n";

    return results;
}

// ─────────────────────────────────────────
// HTTP REQUEST
// Uses libcurl to POST to ollama's API
// Returns the raw response string
// ─────────────────────────────────────────
std::string Embedder::makeHttpRequest(const std::string& payload) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl) {
        std::cerr << "Failed to init curl\n";
        return "";
    }

    std::string url = ollamaUrl_ + "/api/embeddings";

    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

    // Set content type header
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Tell curl where to write the response
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Make the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Curl error: " << curl_easy_strerror(res) << "\n";
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}

// ─────────────────────────────────────────
// PARSE EMBEDDING
// Ollama returns JSON like:
// {"embedding": [0.23, -0.45, 0.12, ...]}
// We manually parse out the numbers
// ─────────────────────────────────────────
std::vector<float> Embedder::parseEmbedding(const std::string& response) {
    std::vector<float> embedding;

    // Find the embedding array in the response
    // Look for "embedding":[
    size_t start = response.find("\"embedding\":[");
    if (start == std::string::npos) {
        std::cerr << "Could not find embedding in response\n";
        return embedding;
    }

    // Move past "embedding":[ to the actual numbers
    start = response.find('[', start) + 1;
    size_t end = response.find(']', start);

    if (end == std::string::npos) return embedding;

    // Extract just the numbers part
    std::string numbersStr = response.substr(start, end - start);

    // Parse comma separated floats
    std::stringstream ss(numbersStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        try {
            embedding.push_back(std::stof(token));
        } catch (...) {
            // skip malformed numbers
        }
    }

    return embedding;
}