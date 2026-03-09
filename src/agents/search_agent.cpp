#include "search_agent.h"
#include <algorithm>

SearchAgent::SearchAgent(VectorStore& store, Embedder& embedder)
    : store_(store), embedder_(embedder) {}

// ─────────────────────────────────────────
// SEARCH
// Converts query to vector, searches store
// filters results by file type based on intent
// ─────────────────────────────────────────
std::vector<SearchResult> SearchAgent::search(const QueryIntent& intent, int topK) {

    // Convert query to vector
    EmbeddingResult queryEmb = embedder_.embed(intent.cleanedQuery);
    if (!queryEmb.success) return {};

    // Get more results than needed so we have room to filter
    auto allResults = store_.search(queryEmb.embedding, topK * 3);

    // Filter by file type if query has specific type
    std::vector<SearchResult> filtered;
    for (const auto& result : allResults) {
        if (matchesType(result.filePath, intent.type)) {
            filtered.push_back(result);
        }
    }

    // Trim to topK
    if (filtered.size() > topK) filtered.resize(topK);

    return filtered;
}

// ─────────────────────────────────────────
// MATCHES TYPE
// Checks if file extension matches query type
// ─────────────────────────────────────────
bool SearchAgent::matchesType(const std::string& filePath, QueryType type) {
    // ANY type matches everything
    if (type == QueryType::ANY) return true;

    // Get file extension
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) return false;
    std::string ext = filePath.substr(dotPos);

    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (type == QueryType::CODE) {
        // Code file extensions
        return ext == ".cpp" || ext == ".h" || ext == ".py" ||
               ext == ".js"  || ext == ".ts" || ext == ".java" ||
               ext == ".go"  || ext == ".c"  || ext == ".cs";
    }

    if (type == QueryType::DOCUMENT) {
        // Document file extensions
        return ext == ".pdf"  || ext == ".docx" || ext == ".txt" ||
               ext == ".md"   || ext == ".csv"  || ext == ".pptx";
    }

    return false;
}