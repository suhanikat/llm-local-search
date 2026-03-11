#include "search_server.h"
#include <iostream>
#include <sstream>

SearchServer::SearchServer(VectorStore& store, Embedder& embedder)
    : store_(store), embedder_(embedder), orchestrator_(store, embedder) {}

// ─────────────────────────────────────────
// START
// Sets up all routes and starts listening
// ─────────────────────────────────────────
void SearchServer::start(int port) {
    httplib::Server server;

    // Serve the HTML page at root
    server.Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_content(getHTML(), "text/html");
    });

    // Search endpoint — takes JSON query, returns JSON results
    server.Post("/search", [this](const httplib::Request& req, httplib::Response& res) {
        handleSearch(req, res);
    });

    // Status endpoint — returns how many chunks indexed
    server.Get("/status", [this](const httplib::Request& req, httplib::Response& res) {
        handleStatus(req, res);
    });

    // Allow cross origin requests from browser
    server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        return httplib::Server::HandlerResponse::Unhandled;
    });

    std::cout << "\n🔍 Local Search Engine\n";
    std::cout << "Open your browser at: http://localhost:" << port << "\n";
    std::cout << "Press Ctrl+C to stop\n\n";

    system("open http://localhost:8080");

    server.listen("0.0.0.0", port);
}

// ─────────────────────────────────────────
// HANDLE SEARCH
// Receives query from browser
// Returns JSON results
// ─────────────────────────────────────────
void SearchServer::handleSearch(const httplib::Request& req, httplib::Response& res) {
    // Extract query from request body
    // Browser sends: {"query": "android resume"}
    std::string body = req.body;
    std::string query;

    // Parse query from JSON manually
    size_t start = body.find("\"query\":");
    if (start != std::string::npos) {
        start = body.find('\"', start + 8) + 1;
        size_t end = body.find('\"', start);
        query = body.substr(start, end - start);
    }

    if (query.empty()) {
        res.set_content("{\"error\": \"empty query\"}", "application/json");
        return;
    }

    std::cout << "Search: " << query << "\n";

    // Run search through orchestrator
    auto results = orchestrator_.search(query, 8);

    // Convert results to JSON
    std::string json = resultsToJSON(results);
    res.set_content(json, "application/json");
}

// ─────────────────────────────────────────
// HANDLE STATUS
// Returns info about the current index
// ─────────────────────────────────────────
void SearchServer::handleStatus(const httplib::Request& req, httplib::Response& res) {
    std::string json = "{\"chunks\": " + std::to_string(store_.size()) + "}";
    res.set_content(json, "application/json");
}

// ─────────────────────────────────────────
// RESULTS TO JSON
// Converts vector of SearchResult to JSON string
// ─────────────────────────────────────────
std::string SearchServer::resultsToJSON(const std::vector<SearchResult>& results) {
    std::ostringstream json;
    json << "{\"results\": [";

    for (size_t i = 0; i < results.size(); i++) {
        const auto& r = results[i];

        // Get file extension for icon
        std::string ext;
        size_t dotPos = r.filePath.find_last_of('.');
        if (dotPos != std::string::npos) {
            ext = r.filePath.substr(dotPos);
        }

        // Get filename only (not full path)
        std::string filename;
        size_t slashPos = r.filePath.find_last_of('/');
        if (slashPos != std::string::npos) {
            filename = r.filePath.substr(slashPos + 1);
        } else {
            filename = r.filePath;
        }

        json << "{";
        json << "\"path\": \"" << escapeJSON(r.filePath) << "\",";
        json << "\"filename\": \"" << escapeJSON(filename) << "\",";
        json << "\"extension\": \"" << escapeJSON(ext) << "\",";
        json << "\"score\": " << r.similarity << ",";
        json << "\"text\": \"" << escapeJSON(r.chunkText.substr(0, 300)) << "\"";
        json << "}";

        if (i < results.size() - 1) json << ",";
    }

    json << "]}";
    return json.str();
}

// ─────────────────────────────────────────
// ESCAPE JSON
// Escapes special characters in strings
// so they're safe inside JSON
// ─────────────────────────────────────────
std::string SearchServer::escapeJSON(const std::string& text) {
    std::string escaped;
    for (char c : text) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += " "; break;  // replace backslash with space
            case '\n': escaped += " "; break;   // replace newline with space
            case '\r': escaped += " "; break;
            case '\t': escaped += " "; break;
            default:
                if (c >= 32) escaped += c;
                break;
        }
    }
    return escaped;
}

// ─────────────────────────────────────────
// GET HTML
// Returns the full HTML page as a string
// This is what the browser renders
// ─────────────────────────────────────────
std::string SearchServer::getHTML() {
    std::string html = "";
    html += "<!DOCTYPE html>\n";
    html += "<html lang='en'>\n";
    html += "<head>\n";
    html += "<meta charset='UTF-8'>\n";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
    html += "<title>Local Search Engine</title>\n";
    html += "<style>\n";
    html += "* { margin: 0; padding: 0; box-sizing: border-box; }\n";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #0f0f0f; color: #e0e0e0; min-height: 100vh; }\n";
    html += ".header { text-align: center; padding: 60px 20px 40px; }\n";
    html += ".header h1 { font-size: 2.5rem; font-weight: 700; background: linear-gradient(135deg, #667eea, #764ba2); -webkit-background-clip: text; -webkit-text-fill-color: transparent; margin-bottom: 8px; }\n";
    html += ".header p { color: #888; font-size: 1rem; }\n";
    html += ".search-container { max-width: 700px; margin: 0 auto; padding: 0 20px; }\n";
    html += ".search-box { display: flex; align-items: center; background: #1a1a1a; border: 1px solid #333; border-radius: 16px; padding: 16px 20px; gap: 12px; transition: border-color 0.2s; }\n";
    html += ".search-box:focus-within { border-color: #667eea; }\n";
    html += ".search-icon { font-size: 1.2rem; color: #666; }\n";
    html += ".search-input { flex: 1; background: none; border: none; outline: none; font-size: 1.1rem; color: #e0e0e0; }\n";
    html += ".search-input::placeholder { color: #555; }\n";
    html += ".status { text-align: center; color: #555; font-size: 0.85rem; margin-top: 12px; }\n";
    html += ".results { max-width: 700px; margin: 30px auto; padding: 0 20px; display: flex; flex-direction: column; gap: 12px; }\n";
    html += ".result-card { background: #1a1a1a; border: 1px solid #2a2a2a; border-radius: 12px; padding: 20px; cursor: pointer; transition: all 0.2s; animation: fadeIn 0.3s ease; }\n";
    html += ".result-card:hover { border-color: #667eea; transform: translateY(-1px); }\n";
    html += "@keyframes fadeIn { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }\n";
    html += ".result-header { display: flex; align-items: center; gap: 12px; margin-bottom: 10px; }\n";
    html += ".file-icon { font-size: 1.5rem; }\n";
    html += ".file-info { flex: 1; }\n";
    html += ".filename { font-weight: 600; font-size: 0.95rem; color: #e0e0e0; }\n";
    html += ".filepath { font-size: 0.75rem; color: #555; margin-top: 2px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }\n";
    html += ".score-badge { background: linear-gradient(135deg, #667eea, #764ba2); color: white; padding: 4px 10px; border-radius: 20px; font-size: 0.8rem; font-weight: 600; white-space: nowrap; }\n";
    html += ".result-text { font-size: 0.85rem; color: #888; line-height: 1.5; border-top: 1px solid #2a2a2a; padding-top: 10px; }\n";
    html += ".loading { text-align: center; color: #667eea; padding: 40px; display: none; }\n";
    html += ".spinner { width: 30px; height: 30px; border: 3px solid #333; border-top-color: #667eea; border-radius: 50%; animation: spin 0.8s linear infinite; margin: 0 auto 12px; }\n";
    html += "@keyframes spin { to { transform: rotate(360deg); } }\n";
    html += ".empty { text-align: center; color: #555; padding: 60px 20px; display: none; }\n";
    html += ".empty-icon { font-size: 3rem; margin-bottom: 12px; }\n";
    html += "</style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "<div class='header'>\n";
    html += "    <h1>Local Search</h1>\n";
    html += "    <p>Search your files using natural language</p>\n";
    html += "</div>\n";
    html += "<div class='search-container'>\n";
    html += "    <div class='search-box'>\n";
    html += "        <span class='search-icon'>🔍</span>\n";
    html += "        <input type='text' class='search-input' id='searchInput' placeholder='Find resume with android experience...' autocomplete='off' />\n";
    html += "    </div>\n";
    html += "    <div class='status' id='status'>Loading index...</div>\n";
    html += "</div>\n";
    html += "<div class='loading' id='loading'>\n";
    html += "    <div class='spinner'></div>\n";
    html += "    Searching...\n";
    html += "</div>\n";
    html += "<div class='empty' id='empty'>\n";
    html += "    <div class='empty-icon'>🗂️</div>\n";
    html += "    No results found\n";
    html += "</div>\n";
    html += "<div class='results' id='results'></div>\n";
    html += "<script>\n";
    html += "var icons = {'.pdf':'📄','.docx':'📝','.txt':'📃','.md':'📋','.cpp':'⚙️','.h':'⚙️','.py':'🐍','.js':'🟨','.jpg':'🖼️','.jpeg':'🖼️','.png':'🖼️','.csv':'📊'};\n";
    html += "function getIcon(ext) { return icons[ext.toLowerCase()] || '📄'; }\n";
    html += "function loadStatus() {\n";
    html += "    fetch('/status').then(function(r) { return r.json(); }).then(function(d) {\n";
    html += "        document.getElementById('status').textContent = d.chunks + ' chunks indexed';\n";
    html += "    }).catch(function() {\n";
    html += "        document.getElementById('status').textContent = 'Index not loaded';\n";
    html += "    });\n";
    html += "}\n";
    html += "var debounceTimer;\n";
    html += "document.getElementById('searchInput').addEventListener('input', function(e) {\n";
    html += "    clearTimeout(debounceTimer);\n";
    html += "    var query = e.target.value.trim();\n";
    html += "    if (query.length < 2) {\n";
    html += "        document.getElementById('results').innerHTML = '';\n";
    html += "        document.getElementById('empty').style.display = 'none';\n";
    html += "        return;\n";
    html += "    }\n";
    html += "    debounceTimer = setTimeout(function() { search(query); }, 500);\n";
    html += "});\n";
    html += "function search(query) {\n";
    html += "    document.getElementById('loading').style.display = 'block';\n";
    html += "    document.getElementById('results').innerHTML = '';\n";
    html += "    document.getElementById('empty').style.display = 'none';\n";
    html += "    fetch('/search', {\n";
    html += "        method: 'POST',\n";
    html += "        headers: { 'Content-Type': 'application/json' },\n";
    html += "        body: JSON.stringify({ query: query })\n";
    html += "    }).then(function(r) { return r.json(); }).then(function(data) {\n";
    html += "        document.getElementById('loading').style.display = 'none';\n";
    html += "        if (!data.results || data.results.length === 0) {\n";
    html += "            document.getElementById('empty').style.display = 'block';\n";
    html += "            return;\n";
    html += "        }\n";
    html += "        var container = document.getElementById('results');\n";
    html += "        container.innerHTML = data.results.map(function(r) {\n";
    html += "            return '<div class=\"result-card\" onclick=\"openFile(\\'' + r.path + '\\')\">'\n";
    html += "                + '<div class=\"result-header\">'\n";
    html += "                + '<span class=\"file-icon\">' + getIcon(r.extension) + '</span>'\n";
    html += "                + '<div class=\"file-info\">'\n";
    html += "                + '<div class=\"filename\">' + r.filename + '</div>'\n";
    html += "                + '<div class=\"filepath\">' + r.path + '</div>'\n";
    html += "                + '</div>'\n";
    html += "                + '<span class=\"score-badge\">' + Math.round(r.score * 100) + '%</span>'\n";
    html += "                + '</div>'\n";
    html += "                + '<div class=\"result-text\">' + r.text + '</div>'\n";
    html += "                + '</div>';\n";
    html += "        }).join('');\n";
    html += "    }).catch(function(e) {\n";
    html += "        document.getElementById('loading').style.display = 'none';\n";
    html += "        console.error('Search failed:', e);\n";
    html += "    });\n";
    html += "}\n";
    html += "function openFile(path) {\n";
    html += "    fetch('/open?path=' + encodeURIComponent(path));\n";
    html += "}\n";
    html += "loadStatus();\n";
    html += "</script>\n";
    html += "</body>\n";
    html += "</html>\n";
    return html;
}