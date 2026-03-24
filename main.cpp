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
#include <atomic>

#include "include/Lexicon.hpp"
#include "include/ForwardIndex.hpp"
#include "include/astronomicalunitc.hpp"
#include "include/InvertedIndex.hpp"
#include "include/SearchEngine.hpp"
#include "include/barrels.hpp"
#include "include/DynamicIndexer.hpp"
#include "include/external/httplib.h"
#include "include/Autocomplete.hpp"

#include <chrono>
#include <json.hpp>

using namespace std;
using namespace httplib;
using json = nlohmann::json;
namespace fs = std::filesystem;
using Clock1 = std::chrono::high_resolution_clock;

std::atomic<bool> RESTART_SERVER(false);


struct EngineConfig {
    string dataset;
    string lexicon;
    string forward;
    string inverted;
    string docmap;
    string barrels;
};


// SAVE CONFIG
void saveConfig(const EngineConfig& cfg) {
    json j;
    j["dataset"] = cfg.dataset;
    j["lexicon"] = cfg.lexicon;
    j["forward"] = cfg.forward;
    j["inverted"] = cfg.inverted;
    j["docmap"] = cfg.docmap;
    j["barrels"] = cfg.barrels;

    ofstream f("engine_config.json");
    f << j.dump(4);
}


// LOAD CONFIG
bool loadConfig(EngineConfig& cfg) {
    ifstream f("engine_config.json");
    if (!f.is_open()) return false;

    json j;
    f >> j;

    cfg.dataset = j["dataset"];
    cfg.lexicon = j["lexicon"];
    cfg.forward = j["forward"];
    cfg.inverted = j["inverted"];
    cfg.docmap = j["docmap"];
    cfg.barrels = j["barrels"];

    return true;
}


void setupEngine() {

    EngineConfig cfg;

    cout << "Dataset Path: ";
    getline(cin, cfg.dataset);

    cout << "Storage Folder: ";
    string base;
    getline(cin, base);

    fs::create_directories(base);

    cfg.lexicon  = base + "/Lexicon.txt";
    cfg.forward  = base + "/Forward.txt";
    cfg.inverted = base + "/Inverted.txt";
    cfg.docmap   = base + "/DocMap.csv";
    cfg.barrels  = base + "/Barrels";

    cout << "\n--- Building Lexicon ---\n";

    Lexicon lex(cfg.dataset, cfg.lexicon);
    lex.readfile_createmap();
    lex.createLexicon();


    cout << "\n--- Building Forward Index ---\n";

    ForwardIndex fwd(
        cfg.lexicon,
        cfg.dataset,
        cfg.forward
    );

    fwd.forwardIndex_creator();


    cout << "\n--- Building Inverted Index ---\n";

    InvertedIndex inv(
        cfg.lexicon,
        cfg.forward,
        cfg.inverted
    );

    inv.invertedIndex_writer();


    cout << "\n--- Building Barrels ---\n";

    BarrelGenerator barrels(cfg.barrels);
    barrels.createBarrels(cfg.inverted);


    cout << "\n--- Building DocMap ---\n";

    AUC auc(cfg.dataset, cfg.docmap);
    auc.createIndexFile();


    saveConfig(cfg);

    cout << "\nSetup Complete\n";
}


void startEngine() {

    EngineConfig cfg;

    if (!loadConfig(cfg)) {
        cout << "Run Setup First\n";
        return;
    }

    Autocomplete autocomplete;
    autocomplete.loadLexicon(cfg.lexicon);


    cout << "\n--- INITIALIZING SEARCH ENGINE ---" << endl;

    auto t3 = Clock1::now();

    SearchEngine engine;

    engine.loadLexicon(cfg.lexicon);
    engine.loadDocMap(cfg.docmap);
    engine.loadBarrels(cfg.barrels);
    engine.setDatasetPath(cfg.dataset);

    auto t4 = Clock1::now();

    cout << "[TIME] Engine initialization took "
         << chrono::duration_cast<chrono::milliseconds>(t4 - t3).count()
         << " ms\n";


    DynamicIndexer indexer(
        cfg.dataset,
        cfg.lexicon,
        cfg.forward,
        cfg.docmap,
        cfg.barrels
    );


    cout << "\n--- STARTING HTTP SERVER ---\n";


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



    // ADD DOC
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

            RESTART_SERVER.store(true);

            res.status = 200;
            res.set_content(R"({"status":"ok","restart":true})", "application/json");

            std::thread([&svr]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                svr.stop();
            }).detach();

        } catch (...) {
            res.status = 400;
            res.set_content(R"({"status":"invalid json"})", "application/json");
        }

    });


    // AUTOCOMPLETE
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
    cout << "GET  http://localhost:8080/search?q=test\n";
    cout << "POST http://localhost:8080/adddoc\n";


    svr.listen("0.0.0.0", 8080);


    if (RESTART_SERVER.load()) {
        cout << "[SERVER] Restarting...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        std::exit(0);
    }

}


int main() {

    try {
        std::locale::global(std::locale(""));
    } catch (...) {
        std::cerr << "Warning: Failed UTF8 locale\n";
    }

    cout << "\nStellarTrace Search Engine\n";
    cout << "1. Setup Engine\n";
    cout << "2. Start Engine\n";
    cout << "Choice: ";

    int choice;
    cin >> choice;
    cin.ignore();

    if (choice == 1)
        setupEngine();
    else
        startEngine();

    return 0;
}