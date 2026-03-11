#pragma once
#include <string>
#include <vector>
#include "../storage/vector_store.h"
#include "../embedding/embedder.h"
#include "../agents/orchestrator.h"
#include "httplib.h"

class SearchServer {
public:
    SearchServer(VectorStore& store, Embedder& embedder);

    // Start the HTTP server on given port
    void start(int port = 8080);

private:
    VectorStore& store_;
    Embedder& embedder_;
    Orchestrator orchestrator_;

    // Route handlers
    void handleSearch(const httplib::Request& req, httplib::Response& res);
    void handleStatus(const httplib::Request& req, httplib::Response& res);
    void handleIndex(const httplib::Request& req, httplib::Response& res);

    // Returns the HTML page as a string
    std::string getHTML();

    // Converts results to JSON string
    std::string resultsToJSON(const std::vector<SearchResult>& results);

    // Escape special characters for JSON
    std::string escapeJSON(const std::string& text);
};