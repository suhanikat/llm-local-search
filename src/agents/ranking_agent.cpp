#include "ranking_agent.h"
#include <algorithm>

// ─────────────────────────────────────────
// RANK
// Takes results from search agent
// applies boosts and re-sorts
// ─────────────────────────────────────────
std::vector<SearchResult> RankingAgent::rank(const std::vector<SearchResult>& results,
                                              const QueryIntent& intent) {
    std::vector<SearchResult> ranked = results;

    for (auto& result : ranked) {
        // Boost score if filename contains query keywords
        float boost = filenameBoost(result.filePath, intent.keywords);
        result.similarity += boost;

        // Cap at 1.0
        if (result.similarity > 1.0f) result.similarity = 1.0f;
    }

    // Re-sort after boosting
    std::sort(ranked.begin(), ranked.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.similarity > b.similarity;
        });

    return ranked;
}

// ─────────────────────────────────────────
// FILENAME BOOST
// If query keyword appears in filename, boost score
// "find android resume" + file "android_notes.pdf"
//  → gets a small boost because "android" is in name
// ─────────────────────────────────────────
float RankingAgent::filenameBoost(const std::string& filePath,
                                   const std::vector<std::string>& keywords) {
    std::string lowerPath = toLower(filePath);
    float boost = 0.0f;

    for (const auto& keyword : keywords) {
        if (lowerPath.find(keyword) != std::string::npos) {
            boost += 0.05f; // 5% boost per matching keyword
        }
    }

    return boost;
}

std::string RankingAgent::toLower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}