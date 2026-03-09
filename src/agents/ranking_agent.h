#pragma once
#include <string>
#include <vector>
#include "../storage/vector_store.h"
#include "query_agent.h"

class RankingAgent {
public:
    // Takes raw results and intent, returns improved ranking
    std::vector<SearchResult> rank(const std::vector<SearchResult>& results,
                                   const QueryIntent& intent);

private:
    // Boosts score if filename contains query keywords
    float filenameBoost(const std::string& filePath,
                        const std::vector<std::string>& keywords);

    // Converts string to lowercase
    std::string toLower(const std::string& str);
};