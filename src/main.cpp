#include <iostream>
#include <string>
#include "indexing/file_scanner.h"
#include "indexing/metadata_extractor.h"
#include "indexing/type_classifier.h"
#include "indexing/content_extractor.h"
#include "embedding/embedder.h"
#include "storage/vector_store.h"
#include "indexing/chunker.h"

// ─────────────────────────────────────────
// INDEX MODE
// Scans folder, extracts content, embeds, saves to disk
// Run this first to build the index
// ─────────────────────────────────────────
void indexFolder(const std::string& folderPath) {
    FileScanner scanner(folderPath);
    MetadataExtractor metaExtractor;
    TypeClassifier classifier;
    ContentExtractor contentExtractor;
    Chunker chunker(500, 100);      // 500 char chunks, 100 char overlap
    Embedder embedder;
    VectorStore store("vectors.db");

    auto files = scanner.scan();
    std::cout << "Found " << files.size() << " files — indexing...\n\n";

    for (const auto& file : files) {
        FileMetadata meta = metaExtractor.extract(file);
        FileType type = classifier.classify(meta.extension);

        if (type == FileType::BUILD_ARTIFACT || type == FileType::UNKNOWN) continue;
        if (!classifier.isLLMReadable(type)) continue;

        FileContent content = contentExtractor.extract(file, meta.extension);
        if (!content.success) continue;

        // Split full file into chunks
        auto chunks = chunker.chunk(content.text, file);

        std::cout << "Indexing: " << meta.path 
                  << " (" << chunks.size() << " chunks)\n";

        // Embed every chunk separately
        for (const auto& chunk : chunks) {
            EmbeddingResult result = embedder.embed(chunk.text);
            if (result.success) {
                store.addEntry(meta.path, chunk.text, 
                               result.embedding, chunk.chunkIndex);
            }
        }
    }

    store.saveToDisk();
    std::cout << "\n✅ Indexing complete — " << store.size() 
              << " chunks saved to vectors.db\n";
}

// ─────────────────────────────────────────
// SEARCH MODE
// Interactive loop — type queries, get results
// Loads vectors from disk, no re-indexing needed
// ─────────────────────────────────────────
void searchMode() {
    Embedder embedder;
    VectorStore store("vectors.db");

    // Load existing index from disk
    store.loadFromDisk();

    if (store.size() == 0) {
        std::cout << "No index found. Run: ./indexer index <folder_path>\n";
        return;
    }

    std::cout << "\n🔍 Local Search Ready — " << store.size() << " chunks indexed\n";
    std::cout << "Type your query (or 'quit' to exit)\n\n";

    // Interactive search loop
    while (true) {
        std::cout << "🔍 Search: ";
        std::string query;
        std::getline(std::cin, query);

        // Exit condition
        if (query == "quit" || query == "exit" || query == "q") {
            std::cout << "Goodbye!\n";
            break;
        }

        if (query.empty()) continue;

        // Convert query to vector
        std::cout << "Searching...\n";
        EmbeddingResult queryEmb = embedder.embed(query);

        if (!queryEmb.success) {
            std::cout << "Failed to embed query: " << queryEmb.error << "\n";
            continue;
        }

        // Search vector store
        auto results = store.search(queryEmb.embedding, 5);

        std::cout << "\n--- Results for: \"" << query << "\" ---\n\n";

        for (size_t i = 0; i < results.size(); i++) {
            const auto& r = results[i];

            // Convert similarity to percentage for readability
            int scorePercent = static_cast<int>(r.similarity * 100);

            std::cout << "#" << (i+1) << " [" << scorePercent << "% match]\n";
            std::cout << "File: " << r.filePath << "\n";
            std::cout << "Text: " << r.chunkText.substr(0, 200) << "...\n";
            std::cout << "\n";
        }
        std::cout << "---\n\n";
    }
}

// ─────────────────────────────────────────
// MAIN
// Two modes:
//   ./indexer index <folder>  → build the index
//   ./indexer search          → interactive search
// ─────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  ./indexer index <folder_path>  — index a folder\n";
        std::cout << "  ./indexer search               — search your files\n";
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "index") {
        if (argc < 3) {
            std::cout << "Please provide a folder path\n";
            std::cout << "Usage: ./indexer index <folder_path>\n";
            return 1;
        }
        indexFolder(argv[2]);

    } else if (mode == "search") {
        searchMode();

    } else {
        std::cout << "Unknown mode: " << mode << "\n";
        std::cout << "Use 'index' or 'search'\n";
    }

    return 0;
}