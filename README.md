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
### Web UI
```bash
# Start the web interface
./indexer serve

# Open browser at
http://localhost:8080
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

## How It Works

### Indexing Sequence
```mermaid
sequenceDiagram
    participant M as main.cpp
    participant FS as FileScanner
    participant ME as MetadataExtractor
    participant TC as TypeClassifier
    participant CE as ContentExtractor
    participant IA as ImageAgent
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

        alt is IMAGE
            M->>IA: describe(filePath)
            IA->>IA: imageToBase64(filePath)
            IA->>IA: buildJSON(base64, prompt)
            IA->>EM: POST /api/generate (LLaVA)
            EM-->>IA: "A yellow sunflower in a garden..."
            IA-->>M: ImageDescription {text}
            M->>EM: embed(description)
            EM-->>M: embedding[768]
            M->>VS: addEntry(path, description, embedding)

        else is DOCUMENT or CODE
            M->>CE: extract(filePath, extension)
            CE-->>M: {text, success}

            M->>CH: chunk(text, filePath)
            CH-->>M: [chunk0, chunk1, ...chunkN]

            loop For every chunk
                M->>EM: embed(chunk.text)
                EM-->>M: embedding[768]
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

    U->>M: "find sunflower picture"
    M->>O: search(query)

    O->>QA: analyze(query)
    QA->>QA: isCodeQuery? → NO
    QA->>QA: isDocumentQuery? → NO
    QA->>QA: extractKeywords → [sunflower, picture]
    QA-->>O: {type: ANY, keywords: [sunflower, picture]}

    O->>SA: search(intent)
    SA->>EM: embed(query)
    EM-->>SA: queryVector[768]

    SA->>VS: search(queryVector, topK*3)
    VS->>VS: cosineSimilarity(query, every stored vector)
    VS->>VS: sort by similarity
    VS->>VS: deduplicate by file
    VS-->>SA: allResults

    SA->>SA: filter by file type (ANY = no filter)
    SA-->>O: filteredResults

    O->>RA: rank(results, intent)
    RA->>RA: boost files with keywords in filename
    RA->>RA: re-sort after boosting
    RA-->>O: rankedResults

    O-->>M: top 5 results
    M-->>U: #1 IMG_4521.jpg - "A yellow sunflower in a garden..."
```

### Image Indexing Deep Dive
```mermaid
sequenceDiagram
    participant M as main.cpp
    participant IA as ImageAgent
    participant LV as LLaVA (Ollama)
    participant EM as Embedder
    participant VS as VectorStore

    M->>IA: describe("photo.jpg")
    IA->>IA: open file as binary bytes
    IA->>IA: encode bytes → base64 string
    Note over IA: FF D8 FF → "/9j/4AAQ..."

    IA->>LV: POST /api/generate
    Note over IA,LV: {model:llava, images:[base64], prompt:"describe..."}

    LV->>LV: decode base64 → pixels
    LV->>LV: vision model analyzes image
    LV-->>IA: "A bright yellow sunflower with dark brown center..."

    IA-->>M: ImageDescription {description, success}

    M->>EM: embed(description)
    Note over EM: same pipeline as text files
    EM-->>M: [0.45, -0.23, 0.78, ...] 768 numbers

    M->>VS: addEntry("photo.jpg", description, embedding)
    VS-->>M: stored ✅
```