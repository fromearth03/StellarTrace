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
#include <json.hpp> // Make sure this is installed

using json = nlohmann::json;

// ... (Keep DocEntry, InvertedList, DocMetadata structs exactly as before) ...
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

    bool operator>(const SearchResult& other) const {
        return score > other.score;
    }
};

class SearchEngine {
private:
    std::unordered_map<std::string, int> lexicon;
    std::unordered_map<int, InvertedList> invertedIndex;
    std::unordered_map<std::string, DocMetadata> docTable;
    std::string rawDatasetPath;

    // ... (Keep parseInteger, parseLong, calculateScore exactly as before) ...
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

public:
    // ... (Keep loadLexicon, loadDocMap, loadInvertedIndex, setDatasetPath exactly as before) ...
    void setDatasetPath(const std::string& path) { rawDatasetPath = path; }

    void loadLexicon(const std::string& filepath) {
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

    void loadInvertedIndex(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;
            std::string header = line.substr(0, colonPos);
            std::string body = line.substr(colonPos + 1);
            std::stringstream headerStream(header);
            std::string idStr;
            double idf;
            headerStream >> idStr >> idf;
            int wordID = parseInteger(idStr);
            InvertedList invList;
            invList.idf = idf;
            std::stringstream bodyStream(body);
            std::string entryStr;
            while (bodyStream >> entryStr) {
                size_t openParen = entryStr.find('(');
                size_t comma = entryStr.find(',');
                size_t closeParen = entryStr.find(')');
                if (openParen != std::string::npos && comma != std::string::npos && closeParen != std::string::npos) {
                    DocEntry entry;
                    entry.docId = entryStr.substr(0, openParen);
                    std::string countStr = entryStr.substr(openParen + 1, comma - (openParen + 1));
                    std::string maskStr = entryStr.substr(comma + 1, closeParen - (comma + 1));
                    entry.count = parseInteger(countStr);
                    entry.mask = parseInteger(maskStr);
                    invList.docs.push_back(entry);
                }
            }
            invertedIndex[wordID] = invList;
        }
        file.close();
    }

    // --- UPDATED SEARCH FUNCTION ---
    // Returns a vector of JSON objects containing ALL fields
    std::vector<json> search(std::string query) {
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
        std::stringstream ss(query);
        std::string term;
        std::unordered_map<std::string, double> docScores;

        while (ss >> term) {
            if (lexicon.find(term) == lexicon.end()) continue;
            int wordID = lexicon[term];
            if (invertedIndex.find(wordID) == invertedIndex.end()) continue;

            InvertedList& list = invertedIndex[wordID];
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

        // Sort by score
        std::sort(results.begin(), results.end(), std::greater<SearchResult>());

        // Fetch Data
        std::ifstream rawFile(rawDatasetPath);
        int count = 0;

        for (const auto& res : results) {
            if (count >= 10) break; // Return top 10

            rawFile.clear();
            rawFile.seekg(res.meta.offset);

            std::vector<char> buffer(res.meta.length);
            if (rawFile.read(buffer.data(), res.meta.length)) {
                std::string jsonStr(buffer.begin(), buffer.end());
                try {
                    // Parse the FULL JSON object from the file
                    json j = json::parse(jsonStr);

                    // Inject the calculated score into the object
                    j["relevance_score"] = res.score;

                    // Add to output list
                    output.push_back(j);
                } catch (...) {
                    // Ignore parsing errors
                }
            }
            count++;
        }
        return output;
    }
};

#endif
