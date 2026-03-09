#include "orchestrator.h"
#include <iostream>

Orchestrator::Orchestrator(VectorStore& store, Embedder& embedder)
    : searchAgent_(store, embedder) {}

// ─────────────────────────────────────────
// SEARCH
// Coordinates all agents:
// 1. Query Agent  → understand intent
// 2. Search Agent → find matching chunks
// 3. Ranking Agent → improve ordering
// ─────────────────────────────────────────
std::vector<SearchResult> Orchestrator::search(const std::string& query, int topK) {

    // Step 1 — Query Agent understands what user wants
    QueryIntent intent = queryAgent_.analyze(query);

    // Print what the query agent understood
    std::cout << "Intent: ";
    if (intent.type == QueryType::CODE)          std::cout << "CODE search\n";
    else if (intent.type == QueryType::DOCUMENT) std::cout << "DOCUMENT search\n";
    else                                          std::cout << "ANY search\n";

    std::cout << "Keywords: ";
    for (const auto& kw : intent.keywords) std::cout << kw << " ";
    std::cout << "\n";

    // Step 2 — Search Agent finds matching chunks
    auto results = searchAgent_.search(intent, topK * 2);

    // Step 3 — Ranking Agent improves ordering
    auto ranked = rankingAgent_.rank(results, intent);

    // Trim to topK
    if (ranked.size() > topK) ranked.resize(topK);

    return ranked;
}