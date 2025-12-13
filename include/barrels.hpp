#ifndef BARRELS_HPP
#define BARRELS_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

class BarrelGenerator {
private:
    int totalBarrels;
    std::string outputDir;

public:
    BarrelGenerator(int nBarrels = 100)
        : totalBarrels(nBarrels), outputDir("Barrels") {}

    void createBarrels(const std::string& inputPath) {
        std::cout << "[Barrels] Creating barrels...\n";

        if (!fs::exists(outputDir))
            fs::create_directory(outputDir);

        std::ifstream infile(inputPath);
        if (!infile.is_open()) {
            std::cerr << "[Barrels][ERROR] Cannot open inverted index\n";
            return;
        }

        std::vector<std::ofstream> barrelFiles(totalBarrels);
        std::vector<std::ofstream> indexFiles(totalBarrels);

        for (int i = 0; i < totalBarrels; ++i) {
            barrelFiles[i].open(outputDir + "/barrel_" + std::to_string(i) + ".txt");
            indexFiles[i].open(outputDir + "/barrel_" + std::to_string(i) + ".idx");
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

        std::cout << "[Barrels] Done. Total entries: " << count << "\n";
    }
};

#endif
