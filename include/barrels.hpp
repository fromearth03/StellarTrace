#ifndef BARRELS_HPP
#define BARRELS_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <chrono>
#include "SearchEngine.hpp" // for DocMetadata
#include "json.hpp"

using json = nlohmann::json;

class Barrels {
private:
    int barrelSize; // How many wordIDs per barrel
    std::string outputFolder;

public:
    // Constructor: default 50,000 wordIDs per barrel
    Barrels(int size = 800, const std::string& folder = "Barrels")
        : barrelSize(size), outputFolder(folder) 
    {
        std::filesystem::create_directories(outputFolder);
    }

    // Helper function to parse wordID with commas
    int parseWordID(const std::string& wordIDStr) {
        std::string cleaned = wordIDStr;
        cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ','), cleaned.end());
        return std::stoi(cleaned);
    }

    // Create barrels from a full inverted index file
    void makeBarrels(const std::string& invertedIndexFile) {
        std::ifstream inFile(invertedIndexFile);
        if (!inFile.is_open()) {
            std::cerr << "ERROR: Cannot open " << invertedIndexFile << "\n";
            return;
        }

        std::unordered_map<int, std::ofstream*> barrelStreams;
        std::unordered_map<int, int> barrelCounts;  // Track entries per barrel
        std::string line;
        int totalWords = 0;
        int maxWordID = 0;
        
        while (std::getline(inFile, line)) {
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string wordIDStr;
            ss >> wordIDStr;
            
            int wordID = parseWordID(wordIDStr);
            totalWords++;
            if (wordID > maxWordID) maxWordID = wordID;

            int barrelNumber = (wordID - 1) / barrelSize;
            barrelCounts[barrelNumber]++;

            if (barrelStreams.find(barrelNumber) == barrelStreams.end()) {
                std::string filename = outputFolder + "/barrel_" + std::to_string(barrelNumber) + ".txt";
                barrelStreams[barrelNumber] = new std::ofstream(filename, std::ios::out);
                if (!barrelStreams[barrelNumber]->is_open()) {
                    std::cerr << "ERROR: Cannot open " << filename << " for writing\n";
                    continue;
                }
            }

            *(barrelStreams[barrelNumber]) << line << "\n";
        }

        for (auto& pair : barrelStreams) {
            pair.second->close();
            delete pair.second;
        }

        inFile.close();
        
        inFile.close();
        
        // Print statistics
        std::cout << "Barrels created successfully in folder: " << outputFolder << "\n";
        std::cout << "Total words processed: " << totalWords << "\n";
        std::cout << "Maximum wordID: " << maxWordID << "\n";
        std::cout << "Number of barrels created: " << barrelStreams.size() << "\n";
        
        for (const auto& pair : barrelCounts) {
            int minID = pair.first * barrelSize + 1;
            int maxID = std::min((pair.first + 1) * barrelSize, maxWordID);
            std::cout << "  Barrel " << pair.first << ": " << pair.second 
                      << " entries (wordIDs " << minID << "-" << maxID << ")\n";
        }
    }

    // Search function: only loads relevant barrels
    std::vector<json> search(
        const std::string& query,
        const std::unordered_map<std::string, int>& lexicon,
        const std::unordered_map<std::string, DocMetadata>& docTable,
        const std::string& datasetPath,
        double* queryTimeMs = nullptr  // Optional: return query time in milliseconds
    ) {
        // Start timing
        auto start = std::chrono::high_resolution_clock::now();
        
        std::unordered_map<std::string, double> docScores;
        std::stringstream ss(query);
        std::string term;

        while (ss >> term) {
            std::transform(term.begin(), term.end(), term.begin(), ::tolower);

            if (lexicon.find(term) == lexicon.end()) continue;

            int wordID = lexicon.at(term);
            int barrelNumber = (wordID - 1) / barrelSize;
            std::string barrelFile = outputFolder + "/barrel_" + std::to_string(barrelNumber) + ".txt";

            std::ifstream inFile(barrelFile);
            if (!inFile.is_open()) continue;

            std::string line;
            while (std::getline(inFile, line)) {
                if (line.empty()) continue;

                std::stringstream lineSS(line);
                std::string idStr;
                double idf;
                lineSS >> idStr >> idf;
                int id = parseWordID(idStr);
                if (id != wordID) continue;

                std::string body = line.substr(line.find(':') + 1);
                std::stringstream bodySS(body);
                std::string entryStr;

                while (bodySS >> entryStr) {
                    size_t openParen = entryStr.find('(');
                    size_t comma = entryStr.find(',');
                    size_t closeParen = entryStr.find(')');

                    if (openParen != std::string::npos && comma != std::string::npos && closeParen != std::string::npos) {
                        std::string docId = entryStr.substr(0, openParen);
                        int count = std::stoi(entryStr.substr(openParen + 1, comma - openParen - 1));
                        int mask = std::stoi(entryStr.substr(comma + 1, closeParen - comma - 1));

                        double tf = static_cast<double>(count);
                        double tf_idf = tf * idf;
                        double positionWeight = 1.0;
                        if (mask == 1) positionWeight = 10.0;
                        else if (mask == 2) positionWeight = 5.0;

                        docScores[docId] += tf_idf + positionWeight;
                    }
                }
            }
        }

        // Sort results
        std::vector<std::pair<std::string, double>> sortedResults(docScores.begin(), docScores.end());
        std::sort(sortedResults.begin(), sortedResults.end(), [](auto& a, auto& b) {
            return a.second > b.second;
        });

        // Fetch JSON objects from dataset
        std::ifstream rawFile(datasetPath);
        std::vector<json> output;
        int count = 0;
        for (auto& pair : sortedResults) {
            if (count >= 10) break; // top 10 results
            if (docTable.find(pair.first) == docTable.end()) continue;

            const DocMetadata& meta = docTable.at(pair.first);

            rawFile.clear();
            rawFile.seekg(meta.offset);
            std::vector<char> buffer(meta.length);

            if (rawFile.read(buffer.data(), meta.length)) {
                std::string jsonStr(buffer.begin(), buffer.end());
                try {
                    json j = json::parse(jsonStr);
                    j["relevance_score"] = pair.second;
                    output.push_back(j);
                    count++;
                } catch (...) {}
            }
        }

        // End timing and calculate duration
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double timeMs = duration.count() / 1000.0;
        
        if (queryTimeMs != nullptr) {
            *queryTimeMs = timeMs;
        }

        return output;
    }
};

#endif // BARRELS_HPP
