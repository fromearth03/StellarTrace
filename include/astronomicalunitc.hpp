#ifndef ASTRONOMICALUNITC_HPP
#define ASTRONOMICALUNITC_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <json.hpp>

using json = nlohmann::json;

class AUC {
private:
    std::string jsonFilePath;
    std::string indexFilePath;

public:
    // Constructor
    AUC(const std::string& jsonPath, const std::string& indexPath)
        : jsonFilePath(jsonPath), indexFilePath(indexPath) {}

    // Generate index file
    bool createIndexFile() const {
        std::ifstream inFile(jsonFilePath, std::ios::binary);
        std::ofstream indexFile(indexFilePath);

        if (!inFile.is_open()) {
            std::cerr << "Error: Cannot open JSON file: " << jsonFilePath << "\n";
            return false;
        }
        if (!indexFile.is_open()) {
            std::cerr << "Error: Cannot open index file: " << indexFilePath << "\n";
            return false;
        }

        // Write CSV header
        indexFile << "internal_doc_id,original_doc_id,start_offset,length\n";

        std::string line;
        uint64_t offset = 0;
        unsigned int internal_id = 1;

        while (std::getline(inFile, line)) {
            uint64_t length = line.size();  // length in bytes
            json obj = json::parse(line);
            std::string original_id = obj["id"];

            indexFile << internal_id << "|" << original_id << "|" << offset << "|" << length << "\n";

            offset += length + 1; // +1 for newline
            internal_id++;
        }

        inFile.close();
        indexFile.close();

        return true;
    }
};

#endif