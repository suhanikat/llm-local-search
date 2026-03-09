#include <iostream>
#include <string>
#include "indexing/file_scanner.h"
#include "indexing/metadata_extractor.h"
#include "indexing/type_classifier.h"
#include "indexing/content_extractor.h"
#include "embedding/embedder.h"
#include "storage/vector_store.h"
#include "indexing/chunker.h"
#include "agents/orchestrator.h"
#include "agents/image_agent.h"

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
    Chunker chunker(500, 100);
    Embedder embedder;
    ImageAgent imageAgent;
    VectorStore store("vectors.db");

    auto files = scanner.scan();
    std::cout << "Found " << files.size() << " files — indexing...\n\n";

    for (const auto& file : files) {
        FileMetadata meta = metaExtractor.extract(file);
        FileType type = classifier.classify(meta.extension);

        if (type == FileType::BUILD_ARTIFACT || type == FileType::UNKNOWN) continue;
        if (!classifier.isLLMReadable(type)) continue;

        // ─────────────────────────────────────────
        // IMAGE FILES — describe with LLaVA
        // then embed the description
        // ─────────────────────────────────────────
        if (type == FileType::IMAGE) {
            std::cout << "Describing image: " << meta.path << "\n";

            ImageDescription desc = imageAgent.describe(file);
            if (desc.success) {
                std::cout << "  Description: " << desc.description.substr(0, 100) << "...\n";

                // Embed the description not the image
                EmbeddingResult result = embedder.embed(desc.description);
                if (result.success) {
                    store.addEntry(meta.path, desc.description, result.embedding, 0);
                    std::cout << "  ✓ image indexed\n";
                }
            } else {
                std::cout << "  ✗ failed: " << desc.error << "\n";
            }
            continue;
        }

        // ─────────────────────────────────────────
        // TEXT FILES — extract, chunk, embed as before
        // ─────────────────────────────────────────
        FileContent content = contentExtractor.extract(file, meta.extension);
        if (!content.success) continue;

        auto chunks = chunker.chunk(content.text, file);
        std::cout << "Indexing: " << meta.path 
                  << " (" << chunks.size() << " chunks)\n";

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
    store.loadFromDisk();

    if (store.size() == 0) {
        std::cout << "No index found. Run: ./indexer index <folder>\n";
        return;
    }

    // Orchestrator coordinates all agents
    Orchestrator orchestrator(store, embedder);

    std::cout << "\n🔍 Local Search Ready — " << store.size() << " chunks indexed\n";
    std::cout << "Type your query (or 'quit' to exit)\n\n";

    while (true) {
        std::cout << "🔍 Search: ";
        std::string query;
        std::getline(std::cin, query);

        if (query == "quit" || query == "exit" || query == "q") break;
        if (query.empty()) continue;

        // Single call to orchestrator — handles everything
        auto results = orchestrator.search(query, 5);

        std::cout << "\n--- Results for: \"" << query << "\" ---\n\n";

        for (size_t i = 0; i < results.size(); i++) {
            int scorePercent = static_cast<int>(results[i].similarity * 100);
            std::cout << "#" << (i+1) << " [" << scorePercent << "% match]\n";
            std::cout << "File: " << results[i].filePath << "\n";
            std::cout << "Text: " << results[i].chunkText.substr(0, 200) << "...\n\n";
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