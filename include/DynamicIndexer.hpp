#ifndef DYNAMIC_INDEXER_HPP
#define DYNAMIC_INDEXER_HPP

#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

class DynamicIndexer {
private:
    static constexpr int TOTAL_BARRELS = 100;

    std::string datasetPath;
    std::string lexiconPath;
    std::string forwardPath;
    std::string docMapPath;
    std::string barrelDir;

    unsigned int nextWordID = 0;
    unsigned int nextInternalDocID = 0;
    unsigned int nextNewID = 0;

    std::unordered_map<std::string, unsigned int> lexicon;
    std::vector<std::pair<std::string, unsigned int>> newlyAddedWords;

    std::unordered_set<std::string> stopWords{
        "the","and","is","in","at","of","on","for","to","a","an","that","it"
    };

    // ---------------- HELPERS ----------------

    std::string clean(const std::string& w) {
        std::string r;
        for (char c : w)
            if (std::isalpha(static_cast<unsigned char>(c)))
                r += std::tolower(c);
        return r;
    }

    void loadLexicon() {
        std::ifstream f(lexiconPath);
        std::string w;
        unsigned int id;
        while (f >> w >> id) {
            lexicon[w] = id;
            nextWordID = std::max(nextWordID, id);
        }
    }

    void loadDocCounters() {
        std::ifstream f(docMapPath);
        std::string line;
        std::getline(f, line);
        while (std::getline(f, line)) {
            nextInternalDocID++;
            if (line.find("new") != std::string::npos)
                nextNewID++;
        }
    }

    void appendLexicon() {
        if (newlyAddedWords.empty()) return;
        std::ofstream out(lexiconPath, std::ios::app);
        for (auto& [w, id] : newlyAddedWords)
            out << w << " " << id << "\n";
        newlyAddedWords.clear();
    }

public:
    DynamicIndexer(
        const std::string& dataset,
        const std::string& lexiconFile,
        const std::string& forward,
        const std::string& docmap,
        const std::string& barrels
    )
        : datasetPath(dataset),
          lexiconPath(lexiconFile),
          forwardPath(forward),
          docMapPath(docmap),
          barrelDir(barrels)
    {
        fs::create_directories(barrelDir);
        loadLexicon();
        loadDocCounters();
    }

    
    // MAIN ENTRY
    bool addDocument(json doc) {
        // ---------- ASSIGN ID ----------
        std::string docID = "new" + std::to_string(++nextNewID);
        doc["id"] = docID;

        // ---------- DATASET ----------
        std::ofstream raw(datasetPath, std::ios::app | std::ios::binary);
        long long offset = raw.tellp();
        std::string dump = doc.dump();
        raw << dump << "\n";
        long long length = dump.size();
        raw.close();

        // DOC MAP
        std::ofstream map(docMapPath, std::ios::app);
        map << ++nextInternalDocID << "|"
            << docID << "|"
            << offset << "|"
            << length << "\n";
        map.close();

        // TOKEN COLLECTION
        std::unordered_map<unsigned int, unsigned int> freq;
        std::unordered_map<unsigned int, int> mask;

        auto get = [](const json& j){
            return j.is_string() ? j.get<std::string>() : "";
        };

        auto processField = [&](const std::string& src, int fieldMask) {
            std::string t = src;
            std::replace_if(t.begin(), t.end(),
                [](char c){ return !std::isalnum(static_cast<unsigned char>(c)); }, ' ');

            std::stringstream ss(t);
            std::string w;

            while (ss >> w) {
                w = clean(w);
                if (w.empty() || stopWords.count(w)) continue;

                if (!lexicon.count(w)) {
                    unsigned int id = ++nextWordID;
                    lexicon[w] = id;
                    newlyAddedWords.emplace_back(w, id);
                }

                unsigned int wid = lexicon[w];
                freq[wid]++;

                if (!mask.count(wid) || fieldMask > mask[wid])
                    mask[wid] = fieldMask;
            }
        };

        // FIELD PROCESSING
        if (doc.contains("abstract"))
            processField(get(doc["abstract"]), 0);

        if (doc.contains("title"))
            processField(get(doc["title"]), 1);

        if (doc.contains("submitter"))
            processField(get(doc["submitter"]), 2);

        if (doc.contains("authors_parsed")) {
            for (auto& a : doc["authors_parsed"]) {
                if (a.is_array() && a.size() >= 2) {
                    processField(get(a[0]), 2);
                    processField(get(a[1]), 2);
                }
            }
        }

        // WRITE NEW WORDS
        appendLexicon();

        // FORWARD INDEX
        std::ofstream fwd(forwardPath, std::ios::app);
        fwd << docID << " : ";
        for (auto& [wid, c] : freq)
            fwd << wid << "(" << c << "," << mask[wid] << ") ";
        fwd << "\n";
        fwd.close();

        // BARRELS + IDX
        for (auto& [wid, c] : freq) {
            int b = wid % TOTAL_BARRELS;

            std::string txtFile = barrelDir + "/barrel_" + std::to_string(b) + ".txt";
            std::string idxFile = barrelDir + "/barrel_" + std::to_string(b) + ".idx";

            std::ofstream txt(txtFile, std::ios::app);
            if (!txt.is_open()) continue;

            long long pos = txt.tellp();

            txt << wid << " 0 : "
                << docID << "(" << c << "," << mask[wid] << ")\n";
            txt.close();

            std::ofstream idx(idxFile, std::ios::app);
            idx << wid << " " << pos << "\n";
            idx.close();
        }

        return true;
    }
};

#endif
