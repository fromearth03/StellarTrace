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

    AUC a("Dataset/arxiv-metadata.json", "AUC.csv");
    a.createIndexFile();


    // --- STEP 1: GENERATE BARRELS (Ensure fresh data) ---
    std::cout << "--- PHASE 1: GENERATING BARRELS ---" << std::endl;
    BarrelGenerator generator(100);
    // IMPORTANT: Make sure this path is correct!
    generator.createBarrels("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/inverted_index.txt");

    // --- STEP 2: START SEARCH ENGINE ---
    std::cout << "\n--- PHASE 2: STARTING SERVER ---" << std::endl;
    SearchEngine engine;

    engine.loadLexicon("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/Lexicon/Lexicon (arxiv-metadata).txt");
    engine.loadDocMap("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/AUC.csv");
    engine.setDatasetPath("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/Dataset/arxiv-metadata.json");

    std::cout << "Engine Ready on http://localhost:8080" << std::endl;

    Server svr;

    svr.Get("/search", [&](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        if (req.has_param("q")) {
            std::string query = req.get_param_value("q");
            std::vector<json> results = engine.search(query);
            json response_json = results;
            res.set_content(response_json.dump(), "application/json");
        } else {
            res.set_content("[]", "application/json");
        }
    });

    svr.listen("0.0.0.0", 8080);
    return 0;
}
