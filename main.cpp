#include <locale>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include "include/Lexicon.hpp"
#include "include/ForwardIndex.hpp"
#include "include/astronomicalunitc.hpp"
#include "include/InvertedIndex.hpp"
#include "include/SearchEngine.hpp"
#include "include/external/httplib.h"
namespace fs = std::filesystem;
using namespace httplib;


int main() {
    try {
        std::locale::global(std::locale(""));
    } catch (...) {
        std::cerr << "Warning: Failed to set global UTF-8 locale.\n";
    }

    SearchEngine engine;

    std::cout << "Loading Engine..." << std::endl;
    // Ensure these match your actual file names!
    engine.loadLexicon("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/Lexicon/Lexicon (arxiv-metadata).txt");
    engine.loadInvertedIndex("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/inverted_index.txt");
    engine.loadDocMap("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/AUC.csv");
    engine.setDatasetPath("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/Dataset/arxiv-metadata.json");

    std::cout << "Engine Ready on http://localhost:8080" << std::endl;

    Server svr;

    svr.Get("/search", [&](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*"); // Allow React to connect

        if (req.has_param("q")) {
            std::string query = req.get_param_value("q");

            // Get the list of full JSON objects
            std::vector<json> results = engine.search(query);

            // Convert directly to string and send
            json response_json = results; // Implicit conversion to JSON array
            res.set_content(response_json.dump(), "application/json");
        } else {
            res.set_content("[]", "application/json");
        }
    });

    svr.listen("0.0.0.0", 8080);
    /*Search Engine Logic
    SearchEngine engine;

    std::cout << "Loading Engine..." << std::endl;
    // Ensure these match your actual file names!
    engine.loadLexicon("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/Lexicon/Lexicon (test).txt");
    engine.loadInvertedIndex("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/InvertedIndextest.txt");
    engine.loadDocMap("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/test.csv");
    engine.setDatasetPath("/home/aliakbar/CLionProjects/StellarTrace/cmake-build-debug/test.json");

    std::cout << "Engine Ready on http://localhost:8080" << std::endl;

    Server svr;

    svr.Get("/search", [&](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*"); // Allow React to connect

        if (req.has_param("q")) {
            std::string query = req.get_param_value("q");

            // Get the list of full JSON objects
            std::vector<json> results = engine.search(query);

            // Convert directly to string and send
            json response_json = results; // Implicit conversion to JSON array
            res.set_content(response_json.dump(), "application/json");
        } else {
            res.set_content("[]", "application/json");
        }
    });

    svr.listen("0.0.0.0", 8080);
    */
    //use following code to create inverted index
    //InvertedIndex I ("Lexicon/Lexicon (test).txt", "asdf","ForwardIndex.txt");
    //I.invertedIndex_writer();


    //Following code is to read data faster from file with doc id starting byte and length
    /*
    AUC a("test.json" , "test.csv");
    a.createIndexFile();
    */
    // Use following to create forward index first comment is path to lexicon file and second to data file
    /*
    ForwardIndex f("Lexicon/Lexicon (test).txt", "test.json");
    f.forwardIndex_creator();
    */
    //Use following syntax to create lexicon if the file is json make the secon argument true other wise it is false
    /*
     #include "include/Lexicon.hpp"
    Lexicon lexicon("CC-MAIN-20251005114239-20251005144239-00004.warc.wet", false);
    lexicon.readfile_createmap();
    lexicon.createLexicon();

    //========================================================================
    //For combined lexicon of all the files use this
    #include "include/Lexiconfolder.hpp"
    LexiconFolder lex("Lexicon"); // folder containing your .txt lexicon files
    lex.mergeLexicons();          // merge all files into CombinedLexicon.txt

    */
    return 0;
}
