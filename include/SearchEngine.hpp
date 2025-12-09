#ifndef SEARCHENGINE_HPP
#define SEARCHENGINE_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

struct DocMetadata {
    uint64_t offset;
    uint64_t length;
};

class SearchEngine {
private:
    std::unordered_map<std::string, int> lexicon;
    std::unordered_map<std::string, DocMetadata> docTable;
    std::string datasetPath;

public:
    SearchEngine() = default;

    void loadLexicon(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open lexicon file: " << path << "\n";
            return;
        }

        std::string word;
        std::string idStr;
        while (file >> word >> idStr) {
            // Remove any commas from the ID
            idStr.erase(std::remove(idStr.begin(), idStr.end(), ','), idStr.end());
            lexicon[word] = std::stoi(idStr);
        }
        file.close();
    }

    void loadDocMap(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open doc map file: " << path << "\n";
            return;
        }

        std::string line;
        // Skip header line
        std::getline(file, line);

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> tokens;

            // Parse pipe-delimited format: internal_id|original_id|offset|length
            while (std::getline(iss, token, '|')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 4) {
                std::string original_id = tokens[1];
                uint64_t offset = std::stoull(tokens[2]);
                uint64_t length = std::stoull(tokens[3]);

                docTable[original_id] = {offset, length};
            }
        }
        file.close();
    }

    void setDatasetPath(const std::string& path) { datasetPath = path; }

    const std::unordered_map<std::string, int>& getLexicon() const { return lexicon; }
    const std::unordered_map<std::string, DocMetadata>& getDocTable() const { return docTable; }
    const std::string& getDatasetPath() const { return datasetPath; }
};

#endif
