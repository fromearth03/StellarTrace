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
        for (size_t i = 0; i < values.size(); ++i)
            sum += values[i] * other.values[i];
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
    bool operator>(const SearchResult& o) const {
        return score > o.score;
    }
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
    static constexpr size_t MAX_RESULTS = 200;

    const std::string BARREL_DIR = "Bar_org/";

    const std::unordered_set<std::string> STOPWORDS = {
        "the","is","are","was","were","to","of","and","or",
        "a","an","in","on","for","with","by","as","at","from","their"
    };

    std::unordered_map<std::string, int> lexicon;
    std::unordered_map<std::string, DocMetadata> docTable;
    std::unordered_map<int, long long> barrelIndex[TOTAL_BARRELS];
    std::unordered_map<std::string, Vector> wordVectors;

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
        if (e.mask == 1) s += 5;
        else if (e.mask == 2) s += 10;
        return s;
    }

    // ===================== SPELLING =====================

    int editDistance(const std::string& a, const std::string& b) {
        int m = a.size(), n = b.size();
        std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
        for (int i = 0; i <= m; i++) dp[i][0] = i;
        for (int j = 0; j <= n; j++) dp[0][j] = j;
        for (int i = 1; i <= m; i++)
            for (int j = 1; j <= n; j++)
                dp[i][j] = (a[i - 1] == b[j - 1])
                    ? dp[i - 1][j - 1]
                    : 1 + std::min({ dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1] });
        return dp[m][n];
    }

    std::string findCorrection(const std::string& word) {
        std::string best;
        int bestDist = 2;
        for (auto& [w, _] : lexicon) {
            if (std::abs((int)w.size() - (int)word.size()) > bestDist) continue;
            int d = editDistance(word, w);
            if (d < bestDist) {
                bestDist = d;
                best = w;
            }
        }
        return best;
    }

    std::string findSemanticNeighbor(const std::string& word) {
        if (!wordVectors.count(word)) return "";
        float bestSim = 0.7f;
        std::string best;
        for (auto& [w, _] : lexicon) {
            if (!wordVectors.count(w)) continue;
            float sim = wordVectors[word].dot(wordVectors[w]) /
                        (wordVectors[word].magnitude() * wordVectors[w].magnitude());
            if (sim > bestSim) {
                bestSim = sim;
                best = w;
            }
        }
        return best;
    }

    // ===================== BARREL FETCH =====================

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
            result.docs.push_back({
                token.substr(0, p1),
                parseInt(token.substr(p1 + 1, p2 - p1 - 1)),
                parseInt(token.substr(p2 + 1, p3 - p2 - 1))
            });
        }
        return result;
    }

    // ===================== SOFT OR SCORING =====================

    std::vector<json> runSoftOR(std::vector<TermInfo>& terms) {
        std::unordered_map<std::string, double> scores;

        for (auto& t : terms) {
            size_t limit = std::min(t.list.docs.size(), MAX_DOCS_PER_TERM);
            for (size_t i = 0; i < limit; ++i) {
                const DocEntry& e = t.list.docs[i];
                scores[e.docId] += score(e, t.list.idf);
            }
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

        size_t k = std::min(MAX_RESULTS, results.size());
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
            if (v.size() >= 4)
                docTable[v[1]] = { v[0], parseLong(v[2]), parseLong(v[3]) };
        }
    }

    void loadBarrels() {
        for (int i = 0; i < TOTAL_BARRELS; ++i) {
            std::ifstream idx(BARREL_DIR + "barrel_" + std::to_string(i) + ".idx");
            int w; long long o;
            while (idx >> w >> o) barrelIndex[i][w] = o;
        }
    }

    // ===================== SEARCH =====================

    std::vector<json> search(const std::string& query) {
        std::stringstream qs(query);
        std::string term;
        std::vector<TermInfo> terms;
        std::vector<std::future<InvertedList>> futures;

        while (qs >> term) {
            std::transform(term.begin(), term.end(), term.begin(), ::tolower);
            if (STOPWORDS.count(term)) continue;

            std::string t;
            if (lexicon.count(term)) t = term;
            else {
                t = findCorrection(term);
                if (t.empty()) t = findSemanticNeighbor(term);
            }
            if (t.empty()) continue;

            int wid = lexicon[t];
            futures.push_back(std::async(std::launch::async,
                &SearchEngine::fetchPostingList, this, wid));
            terms.push_back({ t, wid, 0, {} });
        }

        if (terms.empty()) return {};

        for (size_t i = 0; i < terms.size(); ++i) {
            terms[i].list = futures[i].get();
            terms[i].docCount = terms[i].list.docs.size();
        }

        return runSoftOR(terms);
    }
};

#endif
