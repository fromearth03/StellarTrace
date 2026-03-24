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
#include <limits>
#include <json.hpp>

using json = nlohmann::json;

// ===================== DATA STRUCTURES =====================

struct Vector {
    std::vector<float> values;
    float dot(const Vector& o) const {
        float s = 0;
        for (size_t i = 0; i < values.size(); ++i)
            s += values[i] * o.values[i];
        return s;
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
    static constexpr size_t MAX_RESULTS = 200;

    std::string BARREL_DIR;
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

    bool isDynamicDoc(const std::string& id) const {
        return id.rfind("new", 0) == 0;
    }

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
            float sim =
                wordVectors[word].dot(wordVectors[w]) /
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
        InvertedList r;
        int b = wordID % TOTAL_BARRELS;

        auto it = barrelIndex[b].find(wordID);
        if (it == barrelIndex[b].end()) return r;

        std::ifstream f(BARREL_DIR + "/barrel_" + std::to_string(b) + ".txt");
        if (!f.is_open()) return r;

        f.seekg(it->second);
        std::string line;
        std::getline(f, line);
        if (line.empty()) return r;

        std::stringstream ss(line);
        int id; ss >> id >> r.idf;
        std::string colon; ss >> colon;

        std::string token;
        while (ss >> token) {
            size_t p1 = token.find('(');
            size_t p2 = token.find(',');
            size_t p3 = token.find(')');
            if (p1 == std::string::npos) continue;
            r.docs.push_back({
                token.substr(0, p1),
                parseInt(token.substr(p1 + 1, p2 - p1 - 1)),
                parseInt(token.substr(p2 + 1, p3 - p2 - 1))
            });
        }
        return r;
    }

    // ===================== FINALIZE =====================

    std::vector<json> finalize(std::unordered_map<std::string,double>& scores) {
        std::vector<SearchResult> res;
        for (auto& [d,s] : scores)
            if (docTable.count(d))
                res.push_back({ d, s, docTable[d] });

        if (res.empty()) return {};

        size_t k = std::min(MAX_RESULTS, res.size());
        std::partial_sort(res.begin(), res.begin()+k, res.end(), std::greater<>());

        std::ifstream raw(rawDatasetPath, std::ios::binary);
        std::vector<json> out;

        for (size_t i = 0; i < k; ++i) {
            raw.seekg(res[i].meta.offset);
            std::vector<char> buf(res[i].meta.length);
            raw.read(buf.data(), buf.size());
            try {
                json j = json::parse(std::string(buf.begin(), buf.end()));
                j["relevance_score"] = res[i].score;
                out.push_back(j);
            } catch (...) {}
        }
        return out;
    }

public:
    // ===================== LOADERS =====================
    void setBarrelDir(const std::string& path) {
        BARREL_DIR = path;
    }
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
    void loadBarrels(const std::string& dir) {
        BARREL_DIR = dir;

        for (int i = 0; i < TOTAL_BARRELS; ++i) {
            std::ifstream idx(BARREL_DIR + "/barrel_" + std::to_string(i) + ".idx");
            int w; long long o;
            while (idx >> w >> o) barrelIndex[i][w] = o;
        }
    }

    // ===================== SEARCH =====================

    std::vector<json> search(const std::string& query) {

        // 🔥 NRT DYNAMIC DOC BOOST (NO EARLY RETURN)
        std::unordered_map<std::string,double> dynamicScores;

        std::ifstream raw(rawDatasetPath, std::ios::binary);
        std::string q = query;
        std::transform(q.begin(), q.end(), q.begin(), ::tolower);

        for (auto& [id, meta] : docTable) {

            if (!isDynamicDoc(id)) continue;

            raw.seekg(meta.offset);

            std::vector<char> buf(meta.length);
            raw.read(buf.data(), buf.size());

            try {
                json j = json::parse(std::string(buf.begin(), buf.end()));

                std::string text = j.dump();
                std::transform(text.begin(), text.end(), text.begin(), ::tolower);

                if (text.find(q) != std::string::npos) {
                    dynamicScores[id] += 15.0;  // boost new docs
                }

            } catch (...) {}
        }

        // 🔹 FULL SEMANTIC + SPELLING PIPELINE
        std::stringstream qs(query);
        std::string term;
        std::vector<TermInfo> terms;
        std::vector<std::future<InvertedList>> futures;

        while (qs >> term) {
            std::transform(term.begin(), term.end(), term.begin(), ::tolower);
            if (STOPWORDS.count(term)) continue;

            std::string t = lexicon.count(term) ? term : findCorrection(term);
            if (t.empty()) t = findSemanticNeighbor(term);
            if (t.empty()) continue;

            int wid = lexicon[t];
            futures.push_back(std::async(std::launch::async,
                &SearchEngine::fetchPostingList, this, wid));
            terms.push_back({ t, wid, 0, {} });
        }

        if (terms.empty() && dynamicScores.empty())
            return {};

        // MERGE SCORES
        std::unordered_map<std::string,double> scores = dynamicScores;

        for (size_t i = 0; i < terms.size(); ++i) {
            terms[i].list = futures[i].get();
            for (auto& e : terms[i].list.docs)
                scores[e.docId] += score(e, terms[i].list.idf);
        }

        return finalize(scores);
    }
};

#endif
