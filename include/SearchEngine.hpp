#ifndef SEARCH_ENGINE_HPP
#define SEARCH_ENGINE_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <json.hpp>

using json = nlohmann::json;

// --- Data Structures ---
struct DocEntry {
    std::string docId;
    int count;
    int mask;
};

struct InvertedList {
    double idf;
    std::vector<DocEntry> docs;
};

struct DocMetadata {
    std::string internalId;
    long long offset;
    long long length;
};

struct SearchResult {
    std::string docId;
    double score;
    DocMetadata meta;
    bool operator>(const SearchResult& other) const { return score > other.score; }
};

class SearchEngine {
private:
    std::unordered_map<std::string, int> lexicon;
    std::unordered_map<std::string, DocMetadata> docTable;
    std::string rawDatasetPath;

    const int TOTAL_BARRELS = 100;
    const std::string BARREL_DIR = "Barrels/";

    // --- Helpers ---
    int parseInteger(std::string str) {
        str.erase(std::remove(str.begin(), str.end(), ','), str.end());
        try { return std::stoi(str); } catch (...) { return 0; }
    }

    long long parseLong(std::string str) {
        str.erase(std::remove(str.begin(), str.end(), ','), str.end());
        try { return std::stoll(str); } catch (...) { return 0; }
    }

    double calculateScore(const DocEntry& entry, double idf) {
        double tf = static_cast<double>(entry.count);
        double tf_idf = tf * idf;
        double positionWeight = 1.0;
        if (entry.mask == 1) positionWeight = 10.0;
        else if (entry.mask == 2) positionWeight = 5.0;
        return tf_idf + positionWeight;
    }

    // --- ROBUST BARREL SCANNER (The Fix) ---
    InvertedList getPostingList(int wordID) {
        InvertedList result;
        result.idf = 0.0;

        // 1. Determine which barrel holds this word
        int barrelID = wordID % TOTAL_BARRELS;
        std::string filename = BARREL_DIR + "barrel_" + std::to_string(barrelID) + ".txt";

        std::ifstream file(filename);
        if (!file.is_open()) {
            // Uncomment for debugging:
            // std::cerr << "[Warning] Could not open barrel: " << filename << std::endl;
            return result;
        }

        std::string line;
        // 2. Scan the file line-by-line
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            // Check if this line starts with our WordID
            std::stringstream ss(line);
            uint32_t currentID;
            if (!(ss >> currentID)) continue;

            if (currentID == (uint32_t)wordID) {
                // FOUND IT! Parse the rest of the line.
                // Format: WordID IDF : DocID(TF,Mask) ...

                ss >> result.idf; // Read IDF

                std::string colon;
                ss >> colon; // Skip the ":" separator

                std::string entryStr;
                while (ss >> entryStr) {
                    // Parse "0704.0001(3,1)"
                    size_t p1 = entryStr.find('(');
                    size_t comma = entryStr.find(',');
                    size_t p2 = entryStr.find(')');

                    if (p1 != std::string::npos) {
                        try {
                            DocEntry entry;
                            entry.docId = entryStr.substr(0, p1);

                            std::string tfStr = entryStr.substr(p1 + 1, comma - p1 - 1);
                            std::string maskStr = entryStr.substr(comma + 1, p2 - comma - 1);

                            entry.count = parseInteger(tfStr);
                            entry.mask = parseInteger(maskStr);

                            result.docs.push_back(entry);
                        } catch (...) {}
                    }
                }
                return result; // Stop reading once found
            }
        }
        return result; // Word not found in this barrel
    }

public:
    void setDatasetPath(const std::string& path) { rawDatasetPath = path; }

    void loadLexicon(const std::string& filepath) {
        std::cout << "[Engine] Loading Lexicon..." << std::endl;
        std::ifstream file(filepath);
        if (!file.is_open()) return;
        std::string line, word, idStr;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            ss >> word >> idStr;
            lexicon[word] = parseInteger(idStr);
        }
        file.close();
    }

    void loadDocMap(const std::string& filepath) {
        std::cout << "[Engine] Loading Metadata..." << std::endl;
        std::ifstream file(filepath);
        if (!file.is_open()) return;
        std::string line;
        std::getline(file, line);
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string segment;
            std::vector<std::string> parts;
            while (std::getline(ss, segment, '|')) {
                parts.push_back(segment);
            }
            if (parts.size() >= 4) {
                DocMetadata meta;
                meta.internalId = parts[0];
                meta.offset = parseLong(parts[2]);
                meta.length = parseLong(parts[3]);
                docTable[parts[1]] = meta;
            }
        }
        file.close();
    }

    std::vector<json> search(std::string query) {
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
        std::stringstream ss(query);
        std::string term;
        std::unordered_map<std::string, double> docScores;

        std::cout << "[Search] Processing query: " << query << std::endl;

        while (ss >> term) {
            if (lexicon.find(term) == lexicon.end()) {
                std::cout << "  - Word not in lexicon: " << term << std::endl;
                continue;
            }

            int wordID = lexicon[term];
            std::cout << "  - Fetching WordID: " << wordID << " from Barrels..." << std::endl;

            // --- FETCH FROM DISK ---
            InvertedList list = getPostingList(wordID);

            std::cout << "    > Found " << list.docs.size() << " docs." << std::endl;

            for (const auto& entry : list.docs) {
                docScores[entry.docId] += calculateScore(entry, list.idf);
            }
        }

        std::vector<json> output;
        if (docScores.empty()) return output;

        std::vector<SearchResult> results;
        for (const auto& pair : docScores) {
            SearchResult res;
            res.docId = pair.first;
            res.score = pair.second;
            if (docTable.find(pair.first) != docTable.end()) {
                res.meta = docTable[pair.first];
            }
            results.push_back(res);
        }

        std::sort(results.begin(), results.end(), std::greater<SearchResult>());

        // Open binary mode for robust seeking in the raw JSON
        std::ifstream rawFile(rawDatasetPath, std::ios::binary);
        int count = 0;

        for (const auto& res : results) {
            if (count >= 10) break;

            rawFile.clear();
            rawFile.seekg(res.meta.offset);

            std::vector<char> buffer(res.meta.length);
            if (rawFile.read(buffer.data(), res.meta.length)) {
                std::string jsonStr(buffer.begin(), buffer.end());
                try {
                    json j = json::parse(jsonStr);
                    j["relevance_score"] = res.score;
                    output.push_back(j);
                } catch (...) {}
            }
            count++;
        }
        return output;
    }
};

#endif