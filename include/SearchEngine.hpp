#ifndef SEARCH_ENGINE_HPP
#define SEARCH_ENGINE_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <future>
#include <cmath>
#include <json.hpp>

using json = nlohmann::json;

// ===================== DATA STRUCTURES =====================

struct Vector {
    std::vector<float> values;
    float dot(const Vector& other) const {
        float sum = 0;
        for (size_t i = 0; i < values.size(); ++i) sum += values[i] * other.values[i];
        return sum;
    }
    float magnitude() const {
        return std::sqrt(dot(*this));
    }
};

struct DocEntry {
    std::string docId;
    int tf;
    int mask;
};

struct InvertedList {
    double idf = 0.0;
    std::vector<DocEntry> docs;
};

struct DocMetadata {
    std::string internalId;
    long long offset = 0;
    long long length = 0;
};

struct SearchResult {
    std::string docId;
    double score;
    DocMetadata meta;
    bool operator>(const SearchResult& o) const { return score > o.score; }
};

struct TermInfo {
    std::string term;
    int wordID;
    size_t docCount;
    InvertedList list;
};

// ===================== SEARCH ENGINE =====================

class SearchEngine {
private:
    static constexpr int TOTAL_BARRELS = 100;
    static constexpr size_t MAX_DOCS_PER_TERM = 200000;

    const std::string BARREL_DIR = "Barrels/";

    const std::unordered_set<std::string> STOPWORDS = {
        "the","is","are","was","were","to","of","and","or",
        "a","an","in","on","for","with","by","as","at","from","their"
    };

    std::unordered_map<std::string, int> lexicon;
    std::unordered_map<std::string, DocMetadata> docTable;
    std::unordered_map<int, long long> barrelIndex[TOTAL_BARRELS];
    std::unordered_map<std::string, Vector> wordVectors; // For Semantic Search

    std::string rawDatasetPath;

    // ===================== HELPERS =====================

    int parseInt(std::string s) {
        s.erase(std::remove(s.begin(), s.end(), ','), s.end());
        try { return std::stoi(s); } catch (...) { return 0; }
    }

    long long parseLong(std::string s) {
        s.erase(std::remove(s.begin(), s.end(), ','), s.end());
        try { return std::stoll(s); } catch (...) { return 0; }
    }

    double score(const DocEntry& e, double idf) {
        double s = e.tf * idf;
        if (e.mask == 1) s += 10;
        else if (e.mask == 2) s += 5;
        return s;
    }

    // --- Spelling Correction Logic (Edit Distance) ---
    int editDistance(const std::string& s1, const std::string& s2) {
        int m = s1.length(), n = s2.length();
        std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
        for (int i = 0; i <= m; i++) dp[i][0] = i;
        for (int j = 0; j <= n; j++) dp[0][j] = j;
        for (int i = 1; i <= m; i++) {
            for (int j = 1; j <= n; j++) {
                if (s1[i - 1] == s2[j - 1]) dp[i][j] = dp[i - 1][j - 1];
                else dp[i][j] = 1 + std::min({ dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1] });
            }
        }
        return dp[m][n];
    }

    std::string findCorrection(const std::string& word) {
        std::string bestMatch = "";
        int minDistance = 2; // Threshold for typo tolerance
        for (auto const& [lexWord, id] : lexicon) {
            if (std::abs((int)word.length() - (int)lexWord.length()) > minDistance) continue;
            int dist = editDistance(word, lexWord);
            if (dist < minDistance) {
                minDistance = dist;
                bestMatch = lexWord;
            }
        }
        return bestMatch;
    }

    // --- Semantic Logic (Nearest Neighbor in Lexicon) [cite: 65, 67] ---
    std::string findSemanticNeighbor(const std::string& word) {
        if (wordVectors.find(word) == wordVectors.end()) return "";
        std::string bestMatch = "";
        float maxSim = -1.0f;
        for (auto const& [lexWord, id] : lexicon) {
            if (wordVectors.count(lexWord)) {
                float sim = wordVectors[word].dot(wordVectors[lexWord]) /
                            (wordVectors[word].magnitude() * wordVectors[lexWord].magnitude());
                if (sim > maxSim && sim > 0.7) { // 0.7 Similarity Threshold
                    maxSim = sim;
                    bestMatch = lexWord;
                }
            }
        }
        return bestMatch;
    }

    // ===================== THREAD-SAFE POSTING FETCH =====================

    InvertedList fetchPostingList(int wordID) {
        InvertedList result;
        int bID = wordID % TOTAL_BARRELS;
        std::ifstream file(BARREL_DIR + "barrel_" + std::to_string(bID) + ".txt");
        if (!file.is_open()) return result;
        std::string line;
        auto it = barrelIndex[bID].find(wordID);
        if (it != barrelIndex[bID].end()) {
            file.seekg(it->second);
            std::getline(file, line);
        }
        if (line.empty() || line.rfind(std::to_string(wordID), 0) != 0) {
            file.seekg(0);
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                int id; ss >> id;
                if (id == wordID) break;
                line.clear();
            }
        }
        if (line.empty()) return result;
        std::stringstream ss(line);
        int id; ss >> id >> result.idf;
        std::string colon; ss >> colon;
        std::string token;
        while (ss >> token) {
            size_t p1 = token.find('(');
            size_t p2 = token.find(',');
            size_t p3 = token.find(')');
            if (p1 == std::string::npos) continue;
            DocEntry e;
            e.docId = token.substr(0, p1);
            e.tf = parseInt(token.substr(p1 + 1, p2 - p1 - 1));
            e.mask = parseInt(token.substr(p2 + 1, p3 - p2 - 1));
            result.docs.push_back(e);
        }
        return result;
    }

    std::vector<json> runStrictAND(std::vector<TermInfo>& terms) {
        std::unordered_map<std::string, double> scores;
        bool first = true;
        for (auto& t : terms) {
            size_t limit = std::min(t.list.docs.size(), MAX_DOCS_PER_TERM);
            std::unordered_map<std::string, const DocEntry*> lookup;
            for (size_t i = 0; i < limit; ++i)
                lookup[t.list.docs[i].docId] = &t.list.docs[i];
            if (first) {
                for (auto& [id, e] : lookup) scores[id] = score(*e, t.list.idf);
                first = false;
            } else {
                for (auto it = scores.begin(); it != scores.end(); ) {
                    auto f = lookup.find(it->first);
                    if (f == lookup.end()) it = scores.erase(it);
                    else { it->second += score(*f->second, t.list.idf); ++it; }
                }
            }
            if (scores.empty()) break;
        }
        return finalize(scores);
    }

    std::vector<json> finalize(std::unordered_map<std::string, double>& scores) {
        std::vector<SearchResult> results;
        for (auto& [doc, sc] : scores) {
            if (!docTable.count(doc)) continue;
            results.push_back({ doc, sc, docTable.at(doc) });
        }
        if (results.empty()) return {};
        size_t k = std::min<size_t>(10, results.size());
        std::partial_sort(results.begin(), results.begin() + k, results.end(), std::greater<>());
        std::ifstream raw(rawDatasetPath, std::ios::binary);
        std::vector<json> out;
        for (size_t i = 0; i < k; ++i) {
            raw.seekg(results[i].meta.offset);
            std::vector<char> buf(results[i].meta.length);
            raw.read(buf.data(), buf.size());
            try {
                json j = json::parse(std::string(buf.begin(), buf.end()));
                j["relevance_score"] = results[i].score;
                out.push_back(j);
            } catch (...) {}
        }
        return out;
    }

public:
    void setDatasetPath(const std::string& p) { rawDatasetPath = p; }
    void loadLexicon(const std::string& p) {
        std::ifstream f(p);
        std::string w; int id;
        while (f >> w >> id) lexicon[w] = id;
    }
    void loadDocMap(const std::string& p) {
        std::ifstream f(p);
        std::string line; std::getline(f, line);
        while (std::getline(f, line)) {
            std::stringstream ss(line);
            std::string seg; std::vector<std::string> v;
            while (std::getline(ss, seg, '|')) v.push_back(seg);
            if (v.size() >= 4) docTable[v[1]] = { v[0], parseLong(v[2]), parseLong(v[3]) };
        }
    }
    void loadBarrels() {
        for (int i = 0; i < TOTAL_BARRELS; ++i) {
            std::ifstream idx(BARREL_DIR + "barrel_" + std::to_string(i) + ".idx");
            int w; long long o;
            while (idx >> w >> o) barrelIndex[i][w] = o;
        }
    }

    // ===================== SEARCH WITH SEMANTIC/SPELLING =====================

    std::vector<json> search(const std::string& query) {
        std::stringstream qs(query);
        std::string term;
        std::vector<TermInfo> terms;
        std::vector<std::future<InvertedList>> futures;

        while (qs >> term) {
            std::transform(term.begin(), term.end(), term.begin(), ::tolower);
            if (STOPWORDS.count(term)) continue;

            std::string processedTerm = "";

            // 1. Check Lexicon [cite: 52]
            if (lexicon.count(term)) {
                processedTerm = term;
            }
            // 2. Try Spelling Correction if not in Lexicon [cite: 66]
            else {
                processedTerm = findCorrection(term);
                // 3. Try Semantic Fallback if no spelling match [cite: 65, 67]
                if (processedTerm.empty()) {
                    processedTerm = findSemanticNeighbor(term);
                }
            }

            // 4. Drop word if all methods fail
            if (processedTerm.empty()) continue;

            int wid = lexicon[processedTerm];
            futures.push_back(std::async(std::launch::async, &SearchEngine::fetchPostingList, this, wid));
            terms.push_back({ processedTerm, wid, 0, {} });
        }

        if (terms.empty()) return {};

        for (size_t i = 0; i < terms.size(); ++i) {
            terms[i].list = futures[i].get();
            terms[i].docCount = terms[i].list.docs.size();
        }

        std::sort(terms.begin(), terms.end(), [](auto& a, auto& b) { return a.docCount < b.docCount; });

        while (!terms.empty()) {
            auto results = runStrictAND(terms);
            if (!results.empty()) return results;
            terms.pop_back(); // Relaxation Loop [cite: 60, 61]
        }
        return {};
    }
};

#endif