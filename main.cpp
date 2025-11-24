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
namespace fs = std::filesystem;

int main() {
    try {
        std::locale::global(std::locale(""));
    } catch (...) {
        std::cerr << "Warning: Failed to set global UTF-8 locale.\n";
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
