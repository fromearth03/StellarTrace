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
#include <json.hpp>

using json = nlohmann::json;

// ===================== DATA STRUCTURES =====================

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

    std::unordered_map<std::string,int> lexicon;
    std::unordered_map<std::string,DocMetadata> docTable;
    std::unordered_map<int,long long> barrelIndex[TOTAL_BARRELS];

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
                int id;
                ss >> id;
                if (id == wordID) break;
                line.clear();
            }
        }

        if (line.empty()) return result;

        std::stringstream ss(line);
        int id;
        ss >> id >> result.idf;

        std::string colon;
        ss >> colon;

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

    // ===================== STRICT AND =====================

    std::vector<json> runStrictAND(std::vector<TermInfo>& terms) {
        std::unordered_map<std::string,double> scores;
        bool first = true;

        for (auto& t : terms) {
            size_t limit = std::min(t.list.docs.size(), MAX_DOCS_PER_TERM);
            std::unordered_map<std::string,const DocEntry*> lookup;

            for (size_t i = 0; i < limit; ++i)
                lookup[t.list.docs[i].docId] = &t.list.docs[i];

            if (first) {
                for (auto& [id,e] : lookup)
                    scores[id] = score(*e, t.list.idf);
                first = false;
            } else {
                for (auto it = scores.begin(); it != scores.end(); ) {
                    auto f = lookup.find(it->first);
                    if (f == lookup.end())
                        it = scores.erase(it);
                    else {
                        it->second += score(*f->second, t.list.idf);
                        ++it;
                    }
                }
            }

            if (scores.empty()) break;
        }

        return finalize(scores);
    }

    // ===================== FINALIZE =====================

    std::vector<json> finalize(std::unordered_map<std::string,double>& scores) {
        std::vector<SearchResult> results;

        for (auto& [doc, sc] : scores) {
            if (!docTable.count(doc)) continue;
            results.push_back({doc, sc, docTable.at(doc)});
        }

        if (results.empty()) return {};

        size_t k = std::min<size_t>(10, results.size());
        std::partial_sort(results.begin(), results.begin()+k,
                          results.end(), std::greater<>());

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
    // ===================== LOADERS =====================

    void setDatasetPath(const std::string& p) { rawDatasetPath = p; }

    void loadLexicon(const std::string& p) {
        std::ifstream f(p);
        std::string w; int id;
        while (f >> w >> id) lexicon[w] = id;
    }

    void loadDocMap(const std::string& p) {
        std::ifstream f(p);
        std::string line;
        std::getline(f, line);

        while (std::getline(f, line)) {
            std::stringstream ss(line);
            std::string seg;
            std::vector<std::string> v;
            while (std::getline(ss, seg, '|')) v.push_back(seg);

            if (v.size() >= 4) {
                docTable[v[1]] = {
                    v[0],
                    parseLong(v[2]),
                    parseLong(v[3])
                };
            }
        }
    }

    void loadBarrels() {
        for (int i = 0; i < TOTAL_BARRELS; ++i) {
            std::ifstream idx(BARREL_DIR + "barrel_" + std::to_string(i) + ".idx");
            int w; long long o;
            while (idx >> w >> o)
                barrelIndex[i][w] = o;
        }
    }

    // ===================== SEARCH (SOFT AND) =====================

    std::vector<json> search(const std::string& query) {
        std::stringstream qs(query);
        std::string term;

        std::vector<TermInfo> terms;
        std::vector<std::future<InvertedList>> futures;

        // ---- Collect terms + parallel fetch ----
        while (qs >> term) {
            std::transform(term.begin(), term.end(), term.begin(), ::tolower);
            if (STOPWORDS.count(term)) continue;
            if (!lexicon.count(term)) continue;

            int wid = lexicon[term];
            futures.push_back(std::async(
                std::launch::async,
                &SearchEngine::fetchPostingList,
                this, wid
            ));
            terms.push_back({term, wid, 0, {}});
        }

        if (terms.empty()) return {};

        // ---- Join futures ----
        for (size_t i = 0; i < terms.size(); ++i) {
            terms[i].list = futures[i].get();
            terms[i].docCount = terms[i].list.docs.size();
        }

        // ---- Sort rare → common ----
        std::sort(terms.begin(), terms.end(),
                  [](auto& a, auto& b){
                      return a.docCount < b.docCount;
                  });

        // ================= RELAXATION LOOP =================
        while (!terms.empty()) {
            auto results = runStrictAND(terms);
            if (!results.empty())
                return results;

            // 🔥 Drop the most common term
            terms.pop_back();
        }

        return {};
    }
};

#endif
