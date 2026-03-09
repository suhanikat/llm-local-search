#pragma once
#include <string>
#include <vector>
#include "../storage/vector_store.h"
#include "../embedding/embedder.h"
#include "query_agent.h"

class SearchAgent {
public:
    SearchAgent(VectorStore& store, Embedder& embedder);

    // Takes query intent, searches vector store with filters
    std::vector<SearchResult> search(const QueryIntent& intent, int topK = 10);

private:
    VectorStore& store_;
    Embedder& embedder_;

    // Checks if a file path matches the query type
    // e.g. CODE type only matches .cpp .h .py files
    bool matchesType(const std::string& filePath, QueryType type);
};