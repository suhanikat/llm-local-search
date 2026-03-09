#pragma once
#include <string>
#include <vector>

// What type of search the user wants
enum class QueryType {
    DOCUMENT,    // user wants PDFs, DOCX, TXT
    CODE,        // user wants code files
    ANY          // no specific type, search everything
};

// Structured understanding of what user is asking
struct QueryIntent {
    QueryType type;                  // what kind of files to search
    std::vector<std::string> keywords; // important words extracted from query
    std::string cleanedQuery;        // query with filler words removed
};

class QueryAgent {
public:
    // Takes raw user query, returns structured intent
    QueryIntent analyze(const std::string& query);

private:
    // Detects if query is asking for code files
    bool isCodeQuery(const std::string& query);

    // Detects if query is asking for documents
    bool isDocumentQuery(const std::string& query);

    // Extracts important keywords from query
    // removes words like "find", "show", "me", "the"
    std::vector<std::string> extractKeywords(const std::string& query);

    // Converts string to lowercase
    std::string toLower(const std::string& str);
};