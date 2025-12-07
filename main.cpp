#define _WIN32_WINNT 0x0A00  // Windows 10
#include <locale>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "include/Lexicon.hpp"
#include "include/ForwardIndex.hpp"
#include "include/astronomicalunitc.hpp"
#include "include/InvertedIndex.hpp"
#include "include/SearchEngine.hpp"
#include "include/barrels.hpp"
#include "include/external/httplib.h"

namespace fs = std::filesystem;
using namespace httplib;

int main() {
    try {
        std::locale::global(std::locale(""));
    } catch (...) {
        std::cerr << "Warning: Failed to set global UTF-8 locale.\n";
    }

    // =======================
    // 0️⃣ Generate CSV for document offsets
    // =======================
    std::cout << "Generating doc map CSV..." << std::endl;
    AUC auc("Samplefiles/test.json", "Samplefiles/AUC.csv");
    if (!auc.createIndexFile()) {
        std::cerr << "Failed to create CSV index file. Exiting.\n";
        return 1;
    }

    // =======================
    // 1️⃣ Create/load barrels
    // =======================
    std::cout << "Creating barrels..." << std::endl;
    Barrels barrels;
    barrels.makeBarrels("Samplefiles/InvertedIndex.txt");

    // =======================
    // 2️⃣ Load search engine
    // =======================
    SearchEngine engine;

    std::cout << "Loading Engine..." << std::endl;
    engine.loadLexicon("Samplefiles/Lexicon (test).txt");

    // ✅ Do NOT load full inverted index when using barrels
    // engine.loadInvertedIndex("Samplefiles/InvertedIndex.txt");

    engine.loadDocMap("Samplefiles/AUC.csv");
    engine.setDatasetPath("Samplefiles/test.json");

    std::cout << "Engine Ready on http://localhost:8080" << std::endl;

    // =======================
    // 3️⃣ Start HTTP server
    // =======================
    Server svr;

    svr.Get("/search", [&](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*"); // Allow React to connect

        if (req.has_param("q")) {
            std::string query = req.get_param_value("q");
            
            double queryTimeMs = 0.0;
            
            // Search using barrels
            std::vector<json> results = barrels.search(
                query,
                engine.getLexicon(),
                engine.getDocTable(),
                engine.getDatasetPath(),
                &queryTimeMs
            );

            // Convert directly to string and send
            json response_json;
            response_json["results"] = results;
            response_json["query_time_ms"] = queryTimeMs;
            response_json["num_results"] = results.size();
            
            res.set_content(response_json.dump(), "application/json");
            
            // Log to console
            std::cout << "Query: \"" << query << "\" - " 
                      << results.size() << " results in " 
                      << std::fixed << std::setprecision(2) << queryTimeMs << " ms" << std::endl;
        } else {
            res.set_content("[]", "application/json");
        }
    });

    svr.listen("0.0.0.0", 8080);

    return 0;
}
