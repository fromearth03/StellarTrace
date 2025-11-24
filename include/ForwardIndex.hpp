#ifndef Forwardindex_HPP
#define Forwardindex_HPP

#include <json.hpp>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <sstream>
#include <locale>
#include <algorithm>

using json = nlohmann::json;

class ForwardIndex {
    std::string path_lexicon;
    std::string path_dataset;
    std::locale current_locale;

    std::unordered_map<std::string, unsigned int> words;
    std::unordered_map<std::string, std::list<unsigned int>> f_index;

    std::unordered_set<std::string> stopWords = {
        "the","and","is","in","at","of","on","for","to","a","an","that","it"
    };

public:
    ForwardIndex(std::string p_lexicon, std::string p_dataset)
        : path_lexicon(p_lexicon), path_dataset(p_dataset) {
        try { current_locale = std::locale(""); }
        catch (...) { current_locale = std::locale::classic(); }
    }

    void lexiconCreater() {
        std::ifstream ifs(path_lexicon);
        if (!ifs.is_open()) return;

        std::string word;
        unsigned int id;
        while (ifs >> word >> id)
            words.emplace(word, id);
    }

    std::string cleanWord(const std::string& w) {
        std::string r;
        for (char c : w)
            if (std::isalpha(c, current_locale))
                r += std::tolower(c, current_locale);
        return r;
    }

   void forwardIndex_creator() {
    lexiconCreater();

    std::ifstream file(path_dataset);
    if (!file.is_open()) return;

    std::string line;
    unsigned int uncounted = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::string id = "";
        std::string title_s, abstract_s, authors_s;

        try {
            json paper = json::parse(line);

            // Lambda function to safely get string from json
            // Explanation: This is a small function defined inline.
            // It takes a json object and returns the string if it exists, otherwise empty string.
            auto get = [](const json& j){ return j.is_string() ? j.get<std::string>() : ""; };

            if (paper.contains("id"))
                id = get(paper["id"]); // use lambda to get id string
            else
                id = "Unassigned" + std::to_string(++uncounted);

            if (paper.contains("title"))
                title_s = get(paper["title"]) + " ";

            if (paper.contains("abstract"))
                abstract_s = get(paper["abstract"]) + " ";

            if (paper.contains("submitter"))
                authors_s += get(paper["submitter"]) + " ";

            if (paper.contains("authors_parsed") && paper["authors_parsed"].is_array()) {
                for (const auto& a : paper["authors_parsed"]) {
                    if (a.is_array() && a.size() >= 2) {
                        authors_s += get(a[1]) + " ";
                        authors_s += get(a[0]) + " ";
                        if (a.size() >= 3)
                            authors_s += get(a[2]) + " ";
                    }
                }
            }

        } catch (...) {
            continue;
        }

        // Maps for frequency and mask
        std::unordered_map<unsigned int, unsigned int> freq;
        std::unordered_map<unsigned int, int> mask;

        // Lambda function to process text fields (abstract, title, authors)
        // Explanation: This is an inline function taking text and a mask value.
        // It cleans the text, splits into words, checks stop words, and updates frequency and mask.
        auto process_field = [&](const std::string& text, int fieldMask) {
            std::string cleanedText = text;
            for (char& c : cleanedText)
                if (!std::isalnum(c, current_locale))
                    c = ' ';

            std::istringstream iss(cleanedText);
            std::string w;
            while (iss >> w) {
                std::string cleaned = cleanWord(w);
                if (cleaned.empty()) continue;
                if (stopWords.count(cleaned)) continue;

                auto it = words.find(cleaned);
                if (it != words.end()) {
                    unsigned int wid = it->second;
                    freq[wid]++;           // count
                    mask[wid] = fieldMask; // field mask
                }
            }
        };

        // process fields: abstract=0, title=1, author=2
        process_field(abstract_s, 0);
        process_field(title_s,    1);
        process_field(authors_s,  2);

        // store only unique word IDs in f_index
        std::list<unsigned int> ids;
        for (auto& kv : freq)
            ids.push_back(kv.first);
        f_index.emplace(id, ids);

        // write to file in required format
        std::ofstream out("ForwardIndex.txt", std::ios::app);
        out << id << " : ";
        for (auto& kv : freq) {
            unsigned int wid = kv.first;
            unsigned int count = kv.second;
            int m = mask[wid];
            out << wid << "(" << count << "," << m << ") ";
        }
        out << "\n";
        out.close();
    }
}


};

#endif
