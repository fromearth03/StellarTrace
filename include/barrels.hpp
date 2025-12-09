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
    // MUST MATCH SearchEngine.hpp
    BarrelGenerator(int nBarrels = 100) : totalBarrels(nBarrels), outputDir("Barrels") {}

    void createBarrels(const std::string& inputPath) {
        std::cout << "[Barrels] Creating directory..." << std::endl;
        if (!fs::exists(outputDir)) {
            fs::create_directory(outputDir);
        }

        std::ifstream infile(inputPath);
        if (!infile.is_open()) {
            std::cerr << "Error: Cannot open " << inputPath << std::endl;
            return;
        }

        // Open 100 Barrel Files
        std::vector<std::ofstream*> barrelFiles(totalBarrels);
        for (int i = 0; i < totalBarrels; ++i) {
            std::string fname = outputDir + "/barrel_" + std::to_string(i) + ".txt";
            barrelFiles[i] = new std::ofstream(fname); // Text mode is fine for scanning
        }

        std::string line;
        int processed = 0;

        std::cout << "[Barrels] Splitting inverted index..." << std::endl;

        while (std::getline(infile, line)) {
            if (line.empty()) continue;

            std::stringstream ss(line);
            uint32_t wordID;

            // Peek at the first number (WordID) to decide destination
            if (!(ss >> wordID)) continue;

            int bID = wordID % totalBarrels;

            // Write the FULL line to the correct barrel
            *barrelFiles[bID] << line << "\n";

            processed++;
            if (processed % 10000 == 0) std::cout << "\rProcessed " << processed << " lines..." << std::flush;
        }

        // Close all
        for (auto f : barrelFiles) {
            f->close();
            delete f;
        }
        std::cout << "\n[Barrels] Success! 100 Barrels created in '" << outputDir << "'." << std::endl;
    }
};

#endif