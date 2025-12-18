#ifndef BARRELS_HPP
#define BARRELS_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

// BarrelGenerator handles splitting an inverted index into multiple barrel files.
// Each barrel contains a subset of terms determined by wordID modulo totalBarrels.
// An accompanying index file stores byte offsets for fast lookup.

class BarrelGenerator {
private:
    int totalBarrels;
    std::string outputDir;

public:
    BarrelGenerator(int nBarrels = 1)
        : totalBarrels(nBarrels), outputDir("bartest") {}

    // Creates barrels and index files from the input inverted index file
    // Input: path to inverted index file
    // Output: barrel files and corresponding index files in outputDir

    void createBarrels(const std::string& inputPath) {
        std::cout << "[Barrels] Creating barrels...\n";

        if (!fs::exists(outputDir))
            fs::create_directory(outputDir);

        std::ifstream infile(inputPath);
        if (!infile.is_open()) {
        std::cerr << "[Barrels][ERROR] Cannot open inverted index: " << inputPath << "\n";
        return;
        }

        if (infile.peek() == std::ifstream::traits_type::eof()) {
        std::cerr << "[Barrels][WARNING] Inverted index is empty: " << inputPath << "\n";
        }

        std::vector<std::ofstream> barrelFiles(totalBarrels);
        std::vector<std::ofstream> indexFiles(totalBarrels);

        for (int i = 0; i < totalBarrels; ++i) {
        barrelFiles[i].open(outputDir + "/barrel_" + std::to_string(i) + ".txt");
        indexFiles[i].open(outputDir + "/barrel_" + std::to_string(i) + ".idx");

        if (!barrelFiles[i].is_open() || !indexFiles[i].is_open()) {
        std::cerr << "[Barrels][ERROR] Failed to open barrel or index file for barrel " << i << "\n";
        return;
        }
}

        std::string line;
        long long count = 0;

        while (std::getline(infile, line)) {
            if (line.empty()) continue;

            std::stringstream ss(line);
            int wordID;
            ss >> wordID;

            int bID = wordID % totalBarrels;

            long long offset = barrelFiles[bID].tellp();
            indexFiles[bID] << wordID << " " << offset << "\n";
            barrelFiles[bID] << line << "\n";

            if (++count % 500000 == 0)
                std::cout << "[Barrels] Processed " << count << " entries...\n";
        }

        // Print Total Count
        std::cout << "[Barrels] Done. Total entries: " << count << "\n";
    }
};

#endif
