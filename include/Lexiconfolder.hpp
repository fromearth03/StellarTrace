#ifndef LEXICONFOLDER_HPP
#define LEXICONFOLDER_HPP

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class LexiconFolder {
private:
    std::string folderPath;
    std::unordered_map<std::string, unsigned int> words;
    unsigned int wordID = 0;

public:
    // Constructor: provide folder path containing lexicon .txt files
    LexiconFolder(const std::string& path) : folderPath(path) {}

    // Read all .txt files in folder and merge into a single lexicon
    void mergeLexicons() {
        std::vector<std::string> files;

        // Step 1: Collect all .txt files in the folder
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                files.push_back(entry.path().string());
            }
        }

        // Step 2: Read each file and add words to the map
        for (const auto& file : files) {
            std::ifstream in(file);
            if (!in.is_open()) {
                std::cerr << "Failed to open file: " << file << "\n";
                continue;
            }

            std::string line;
            while (std::getline(in, line)) {
                std::istringstream iss(line);
                std::string word;
                unsigned int id;

                if (!(iss >> word >> id)) continue; // skip malformed lines

                // Only insert if word is not already in map
                if (words.find(word) == words.end()) {
                    words[word] = ++wordID; // assign new unique ID
                }
            }

            in.close();
        }

        // Step 3: Write combined lexicon to a single output file
        std::string outputFile = "CombinedLexicon.txt";
        std::ofstream out(outputFile);
        if (!out.is_open()) {
            std::cerr << "Failed to open output file: " << outputFile << "\n";
            return;
        }

        for (const auto& p : words) {
            out << p.first << " " << p.second << "\n";
        }

        out.close();
        std::cout << "Combined lexicon written to " << outputFile
                  << " with " << words.size() << " unique words.\n";
    }
};

#endif // LEXICONFOLDER_HPP
