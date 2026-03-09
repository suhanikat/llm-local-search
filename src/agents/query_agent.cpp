#include "query_agent.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

// ─────────────────────────────────────────
// ANALYZE
// Main function — reads query and figures out intent
// ─────────────────────────────────────────
QueryIntent QueryAgent::analyze(const std::string& query) {
    QueryIntent intent;
    std::string lowerQuery = toLower(query);

    // Figure out what type of search this is
    if (isCodeQuery(lowerQuery)) {
        intent.type = QueryType::CODE;
    } else if (isDocumentQuery(lowerQuery)) {
        intent.type = QueryType::DOCUMENT;
    } else {
        intent.type = QueryType::ANY;
    }

    // Extract meaningful keywords
    intent.keywords = extractKeywords(query);

    // Store cleaned query
    intent.cleanedQuery = query;

    return intent;
}

// ─────────────────────────────────────────
// IS CODE QUERY
// Checks if user is asking for code files
// ─────────────────────────────────────────
bool QueryAgent::isCodeQuery(const std::string& query) {
    // Words that signal user wants code
    std::vector<std::string> codeSignals = {
        "code", "function", "class", "cpp", "python",
        "javascript", "script", "program", "implementation",
        "header", "source", "file scanner", "embedder"
    };

    for (const auto& signal : codeSignals) {
        if (query.find(signal) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────
// IS DOCUMENT QUERY
// Checks if user is asking for documents
// ─────────────────────────────────────────
bool QueryAgent::isDocumentQuery(const std::string& query) {
    // Words that signal user wants documents
    std::vector<std::string> docSignals = {
        "resume", "pdf", "document", "docx", "report",
        "notes", "letter", "cv", "certificate", "paper"
    };

    for (const auto& signal : docSignals) {
        if (query.find(signal) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────
// EXTRACT KEYWORDS
// Removes filler words, keeps meaningful ones
// "find me the resume with android" 
//  → ["resume", "android"]
// ─────────────────────────────────────────
std::vector<std::string> QueryAgent::extractKeywords(const std::string& query) {
    // Filler words to ignore
    std::unordered_set<std::string> stopWords = {
        "find", "show", "me", "my", "the", "a", "an",
        "with", "about", "get", "give", "search", "look",
        "for", "in", "of", "and", "or", "is", "are", "files"
    };

    std::vector<std::string> keywords;
    std::stringstream ss(query);
    std::string word;

    // Split query into words
    while (ss >> word) {
        // Convert to lowercase for comparison
        std::string lowerWord = toLower(word);

        // Keep word if it's not a stop word
        if (stopWords.find(lowerWord) == stopWords.end()) {
            keywords.push_back(lowerWord);
        }
    }

    return keywords;
}

// ─────────────────────────────────────────
// TO LOWER
// Converts string to lowercase
// ─────────────────────────────────────────
std::string QueryAgent::toLower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}