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
#include <chrono>
using namespace std;
using Clock1 = std::chrono::high_resolution_clock;
namespace fs = std::filesystem;
using namespace httplib;
using namespace std;

int main() {
    try {
        std::locale::global(std::locale(""));
    } catch (...) {
        std::cerr << "Warning: Failed to set global UTF-8 locale.\n";
    }


    // ================= PHASE 1: BUILD BARRELS =================
    cout << "--- PHASE 1: GENERATING BARRELS ---" << endl;
    // auto t1 = Clock1::now();
    //
    // BarrelGenerator generator(100);
    // generator.createBarrels(
    //     "/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/inverted_index.txt"
    // );
    //
    // auto t2 = Clock1::now();
    // cout << "[TIME] Barrel generation took "
    //      << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count()
    //      << " ms\n";
    //

    // ================= PHASE 2: INIT SEARCH ENGINE =================
    cout << "\n--- PHASE 2: INITIALIZING SEARCH ENGINE ---" << endl;
    auto t3 = Clock1::now();

    SearchEngine engine;

    engine.loadLexicon(
        "/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/Lexicon/Lexicon (arxiv-metadata).txt"
    );

    engine.loadDocMap(
        "/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/AUC.csv"
    );

    engine.loadBarrels();   // 🔥 REQUIRED

    engine.setDatasetPath(
        "/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/Dataset/arxiv-metadata.json"
    );

    auto t4 = Clock1::now();
    cout << "[TIME] Engine initialization took "
         << chrono::duration_cast<chrono::milliseconds>(t4 - t3).count()
         << " ms\n";

    cout << "[OK] Search engine ready\n";


    // ================= PHASE 3: START HTTP SERVER =================
    cout << "\n--- PHASE 3: STARTING HTTP SERVER ---" << endl;

    Server svr;

    svr.Get("/search", [&](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        if (!req.has_param("q")) {
            res.set_content("[]", "application/json");
            return;
        }

        string query = req.get_param_value("q");

        // ⏱ START QUERY TIMER
        auto qs = Clock1::now();

        auto results = engine.search(query);

        auto qe = Clock1::now();
        auto durationMs =
            chrono::duration_cast<chrono::milliseconds>(qe - qs).count();

        cout << "[TIME] Query \"" << query
             << "\" took " << durationMs << " ms\n";

        json response = results;
        res.set_content(response.dump(), "application/json");
    });

    cout << "🚀 Server running at http://localhost:8080/search?q=your+query\n";

    svr.listen("0.0.0.0", 8080);

    return 0;
}