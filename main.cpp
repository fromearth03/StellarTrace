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
#include <atomic>   // 🔥 ADDED
#include "include/Lexicon.hpp"
#include "include/ForwardIndex.hpp"
#include "include/astronomicalunitc.hpp"
#include "include/InvertedIndex.hpp"
#include "include/SearchEngine.hpp"
#include "include/barrels.hpp"
#include "include/DynamicIndexer.hpp"
#include "include/external/httplib.h"
#include <chrono>
#include "include/Autocomplete.hpp"
#include <json.hpp>
using namespace std;
using Clock1 = std::chrono::high_resolution_clock;
namespace fs = std::filesystem;
using namespace httplib;

// 🔥 ADDED: restart flag
std::atomic<bool> RESTART_SERVER(false);

int main() {
    try {
        std::locale::global(std::locale(""));
    } catch (...) {
        std::cerr << "Warning: Failed to set global UTF-8 locale.\n";
    }

    Autocomplete autocomplete;
    autocomplete.loadLexicon(
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/Lexicon (arxiv-metadata)_org.txt"
    );

    cout << "--- PHASE 1: GENERATING BARRELS ---" << endl;

    cout << "\n--- PHASE 2: INITIALIZING SEARCH ENGINE ---" << endl;
    auto t3 = Clock1::now();

    SearchEngine engine;

    engine.loadLexicon(
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/Lexicon (arxiv-metadata)_org.txt"
    );

    engine.loadDocMap(
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/AUC_org.csv"
    );

    engine.loadBarrels();

    engine.setDatasetPath(
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/arxiv-metadata.json"
    );

    auto t4 = Clock1::now();
    cout << "[TIME] Engine initialization took "
         << chrono::duration_cast<chrono::milliseconds>(t4 - t3).count()
         << " ms\n";

    cout << "[OK] Search engine ready\n";

    DynamicIndexer indexer(
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/arxiv-metadata.json",
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/Lexicon (arxiv-metadata)_org.txt",
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/ForwardIndex_org.txt",
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/AUC_org.csv",
        "/home/aliakbar/CLionProjects/StellarTrace/OriginalData/Bar_org"
    );

    cout << "\n--- PHASE 3: STARTING HTTP SERVER ---" << endl;

    Server svr;

    svr.Options(".*", [&](const Request&, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header(
            "Access-Control-Allow-Headers",
            "Content-Type, ngrok-skip-browser-warning"
        );
        res.status = 204;
    });

    // SEARCH
    svr.Get("/search", [&](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        if (!req.has_param("q")) {
            res.set_content("[]", "application/json");
            return;
        }

        string query = req.get_param_value("q");

        auto qs = Clock1::now();
        auto results = engine.search(query);
        auto qe = Clock1::now();

        cout << "[TIME] Query \"" << query
             << "\" took "
             << chrono::duration_cast<chrono::milliseconds>(qe - qs).count()
             << " ms\n";

        res.set_content(json(results).dump(), "application/json");
    });

    svr.Options("/adddoc", [&](const Request&, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    // ADD DOCUMENT
    svr.Post("/adddoc", [&](const Request& req, Response& res) {
     res.set_header("Access-Control-Allow-Origin", "*");

     try {
         json doc = json::parse(req.body);

         bool ok = indexer.addDocument(doc);
         if (!ok) {
             res.status = 500;
             res.set_content(R"({"status":"error"})", "application/json");
             return;
         }

         RESTART_SERVER.store(true);   // already there
         res.status = 200;
         res.set_content(R"({"status":"ok","restart":true})", "application/json");

         // 🔥 THIS IS THE MISSING LINE
         std::thread([&svr]() {
             std::this_thread::sleep_for(std::chrono::milliseconds(100));
             svr.stop();
         }).detach();

     } catch (...) {
         res.status = 400;
         res.set_content(R"({"status":"invalid json"})", "application/json");
     }
 });


    svr.Get("/autocomplete", [&](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        if (!req.has_param("q")) {
            res.set_content("[]", "application/json");
            return;
        }

        json j = autocomplete.suggest(req.get_param_value("q"));
        res.set_content(j.dump(), "application/json");
    });

    cout << "Server running at:\n";
    cout << "   GET  http://localhost:8080/search?q=your+query\n";
    cout << "   POST http://localhost:8080/adddoc\n";

    svr.listen("0.0.0.0", 8080);

    // 🔥 CLEAN RESTART POINT
    if (RESTART_SERVER.load()) {
        cout << "[SERVER] Restarting to reload indexes...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        std::exit(0);
    }

    return 0;
}
