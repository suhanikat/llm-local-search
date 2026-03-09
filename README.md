# LLM Local Search Engine

A fully native C++ semantic search engine for local files. Search your documents, PDFs, and code files using natural language — no cloud, no internet, completely private.

## What it does
Type "find resume with android experience" and it finds the right files even if the filename doesn't match — by understanding the meaning of your query.

## Architecture
```
FileScanner → TypeClassifier → ContentExtractor → Chunker → Embedder → VectorStore
                                                                            ↓
                                    QueryAgent → SearchAgent → RankingAgent → Results
```

## Tech Stack
- **Language:** C++17
- **PDF extraction:** Poppler
- **DOCX extraction:** libzip + XML parsing
- **Embeddings:** Ollama (nomic-embed-text) via libcurl — GPU accelerated
- **Vector search:** Custom cosine similarity engine
- **Build system:** CMake

## Features
- Recursively scans any local folder
- Extracts text from PDF, DOCX, TXT, MD, CSV, and code files
- Splits files into overlapping chunks for better search accuracy
- Generates 768-dimensional embeddings locally via Ollama
- Stores vectors in a custom binary vector store
- Multi-agent architecture for intelligent query understanding
- Deduplicates results by file
- Fully offline and private — no data leaves your machine

## Setup

### Prerequisites
- macOS with Ollama installed
- MacPorts: `poppler`, `libzip`
- CMake 3.10+

### Install dependencies
```bash
sudo port install poppler
sudo port install libzip
ollama pull nomic-embed-text
```

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### Usage
```bash
# Index a folder
./indexer index /path/to/folder

# Search
./indexer search
🔍 Search: find resume with android experience
🔍 Search: show me python code files
🔍 Search: find documents about machine learning
```

## Project Structure
```
src/
├── indexing/
│   ├── file_scanner        — recursive file discovery
│   ├── metadata_extractor  — file size, type, modified date
│   ├── type_classifier     — categorizes files, filters junk
│   ├── content_extractor   — reads PDF, DOCX, plain text
│   └── chunker             — splits text into overlapping chunks
├── embedding/
│   └── embedder            — HTTP calls to Ollama, returns 768-dim vectors
├── storage/
│   └── vector_store        — binary vector storage, cosine similarity search
└── agents/
    ├── query_agent         — extracts intent and keywords from query
    ├── search_agent        — searches with file type filtering
    ├── ranking_agent       — boosts results by filename match
    └── orchestrator        — coordinates all agents
```

## Roadmap
- [ ] Image search using LLaVA vision model
- [ ] Full document chunking with better boundary detection  
- [ ] VS Code extension UI
- [ ] Support for more file types


## How It Works

### Indexing Sequence
```mermaid
sequenceDiagram
    participant M as main.cpp
    participant FS as FileScanner
    participant ME as MetadataExtractor
    participant TC as TypeClassifier
    participant CE as ContentExtractor
    participant CH as Chunker
    participant EM as Embedder
    participant VS as VectorStore

    M->>FS: scan(folderPath)
    FS-->>M: [file1, file2, ...fileN]

    loop For every file
        M->>ME: extract(filePath)
        ME-->>M: {size, extension, modified}

        M->>TC: classify(extension)
        TC-->>M: FileType + isLLMReadable

        alt is LLM readable
            M->>CE: extract(filePath, extension)
            CE-->>M: {text, success}

            M->>CH: chunk(text, filePath)
            CH-->>M: [chunk0, chunk1, ...chunkN]

            loop For every chunk
                M->>EM: embed(chunk.text)
                EM-->>M: {embedding[768], success}
                M->>VS: addEntry(path, text, embedding)
            end
        end
    end

    M->>VS: saveToDisk()
    VS-->>M: vectors.db saved
```

### Search Sequence
```mermaid
sequenceDiagram
    participant U as User
    participant M as main.cpp
    participant O as Orchestrator
    participant QA as QueryAgent
    participant SA as SearchAgent
    participant RA as RankingAgent
    participant EM as Embedder
    participant VS as VectorStore

    U->>M: "find resume with android"
    M->>O: search(query)

    O->>QA: analyze(query)
    QA-->>O: {type: DOCUMENT, keywords: [resume, android]}

    O->>SA: search(intent)
    SA->>EM: embed(query)
    EM-->>SA: queryVector[768]

    SA->>VS: search(queryVector, topK*3)
    VS-->>SA: allResults (cosine similarity)

    SA->>SA: filter by DOCUMENT type only
    SA-->>O: filteredResults

    O->>RA: rank(results, intent)
    RA->>RA: boost files with keywords in filename
    RA-->>O: rankedResults

    O-->>M: top 5 results
    M-->>U: display results
```