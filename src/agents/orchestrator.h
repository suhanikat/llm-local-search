#pragma once
#include <string>
#include <vector>
#include "query_agent.h"
#include "search_agent.h"
#include "ranking_agent.h"
#include "../storage/vector_store.h"
#include "../embedding/embedder.h"

class Orchestrator {
public:
    Orchestrator(VectorStore& store, Embedder& embedder);

    // Main entry point — takes raw query, returns ranked results
    std::vector<SearchResult> search(const std::string& query, int topK = 5);

private:
    QueryAgent queryAgent_;
    SearchAgent searchAgent_;
    RankingAgent rankingAgent_;
};