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
namespace fs = std::filesystem;

int main() {
    try {
        std::locale::global(std::locale(""));
    } catch (...) {
        std::cerr << "Warning: Failed to set global UTF-8 locale.\n";
    }
    SearchEngine engine;

    std::cout << "========================================\n";
    std::cout << "      StellarTrace Search Engine        \n";
    std::cout << "========================================\n";

    // 2. Load the Data Files
    // CRITICAL: Ensure these filenames match exactly what you have on your disk

    std::cout << "[*] Loading Lexicon..." << std::endl;
    // Format: word id
    engine.loadLexicon("Lexicon/Lexicon (test).txt");

    std::cout << "[*] Loading Inverted Index..." << std::endl;
    // Format: WordID idf : docID(count,mask) ...
    engine.loadInvertedIndex("InvertedIndextest1.txt");

    std::cout << "[*] Loading Document Metadata..." << std::endl;
    // Format: internal_id|original_id|offset|length
    engine.loadDocMap("test.csv");
    engine.setDatasetPath("test.json");

    std::cout << "[*] System Ready.\n" << std::endl;

    // 3. Interactive Search Loop
    std::string query;
    while (true) {
        std::cout << "Enter search query (or 'exit'): ";
        if (!std::getline(std::cin, query)) break; // Handle EOF

        // Exit condition
        if (query == "exit" || query == "quit") {
            std::cout << "Shutting down..." << std::endl;
            break;
        }

        // Skip empty searches
        if (query.empty()) continue;

        // 4. Call the search function
        engine.search(query);
    }
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
