#include "image_agent.h"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <sstream>

const std::string ImageAgent::base64Chars_ =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t writeCallback(void* contents, size_t size, 
                             size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;   //We must return the number of bytes we processed. If we return anything different, curl thinks there was an error and stops.

}

ImageAgent::ImageAgent(const std::string& ollamaUrl) 
    : ollamaUrl_(ollamaUrl) {}

// ─────────────────────────────────────────
// DESCRIBE
// Main function — reads image, sends to LLaVA,
// returns natural language description
// ─────────────────────────────────────────
ImageDescription ImageAgent::describe(const std::string& filePath) {
    ImageDescription result;
    result.filePath = filePath;
    result.success = false;

    // Convert image to base64
    // LLaVA API expects images as base64 encoded strings
    std::string base64Image = imageToBase64(filePath);
    if (base64Image.empty()) {
        result.error = "Could not read image file";
        return result;
    }

    // Build JSON payload for LLaVA
    // moondream expects: model, prompt, and images array
    std::string payload = "{"
        "\"model\": \"moondream\","
        "\"prompt\": \"Describe this image in detail. "
        "What objects, people, places, colors, and activities are visible? "
        "Be specific so this description can be used for search.\","
        "\"images\": [\"" + base64Image + "\"],"
        "\"stream\": false"
        "}";

    // Send to Ollama
    std::string response = makeHttpRequest(payload);
    if (response.empty()) {
        result.error = "No response from Ollama";
        return result;
    }

    std::cout << "  Raw response: " << response.substr(0, 200) << "\n";

    // Extract description from response
    // Response format: {"response": "A yellow sunflower..."}
    size_t start = response.find("\"response\":\"");
    if (start == std::string::npos) {
        start = response.find("\"response\": \"");
    }
    if (start == std::string::npos) {
        result.error = "Could not parse response";
        return result;
    }

    // Move past "response":"
    start = response.find('\"', start + 11) + 1;
    size_t end = response.find('\"', start);

    if (end == std::string::npos) {
        result.error = "Could not find end of response";
        return result;
    }

    result.description = response.substr(start, end - start);
    result.success = true;
    return result;
}

// ─────────────────────────────────────────
// IMAGE TO BASE64
// Reads image file as binary and encodes to base64
// Base64 converts binary data to safe text characters
// so it can be sent in JSON
// ─────────────────────────────────────────
std::string ImageAgent::imageToBase64(const std::string& filePath) {
    // Open image as binary
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return "";

    // Read entire file into string
    std::string imageData((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    file.close();

    // Base64 encode
    std::string encoded;
    int i = 0;
    unsigned char char3[3], char4[4];
    int len = imageData.size();
    int idx = 0;

    while (len--) {
        char3[i++] = imageData[idx++];
        if (i == 3) {
            char4[0] = (char3[0] & 0xfc) >> 2;
            char4[1] = ((char3[0] & 0x03) << 4) + ((char3[1] & 0xf0) >> 4);
            char4[2] = ((char3[1] & 0x0f) << 2) + ((char3[2] & 0xc0) >> 6);
            char4[3] = char3[2] & 0x3f;
            for (i = 0; i < 4; i++) encoded += base64Chars_[char4[i]];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++) char3[j] = '\0';
        char4[0] = (char3[0] & 0xfc) >> 2;
        char4[1] = ((char3[0] & 0x03) << 4) + ((char3[1] & 0xf0) >> 4);
        char4[2] = ((char3[1] & 0x0f) << 2) + ((char3[2] & 0xc0) >> 6);
        for (int j = 0; j < i + 1; j++) encoded += base64Chars_[char4[j]];
        while (i++ < 3) encoded += '=';
    }

    return encoded;
}

// ─────────────────────────────────────────
// HTTP REQUEST
// Posts to Ollama's generate endpoint
// Different from embeddings endpoint — 
// this one returns text, not vectors
// ─────────────────────────────────────────
std::string ImageAgent::makeHttpRequest(const std::string& payload) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl) return "";

    // LLaVA uses /api/generate not /api/embeddings
    std::string url = ollamaUrl_ + "/api/generate";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());

    // Set timeout — LLaVA takes longer than embeddings , if not pointed out , curl will timeout before it gets a response.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Curl error: " << curl_easy_strerror(res) << "\n";
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}