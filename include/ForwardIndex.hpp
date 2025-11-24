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
    std::unordered_map<std::string, std::unordered_map<unsigned int, unsigned int>> f_index;

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
        std::string content;

        try {
            json paper = json::parse(line);
            auto get = [](const json& j){ return j.is_string() ? j.get<std::string>() : ""; };

            if (paper.contains("id"))
                id = get(paper["id"]);
            else
                id = "Unassigned" + std::to_string(++uncounted);

            if (paper.contains("title"))
                content += get(paper["title"]) + " ";

            if (paper.contains("abstract"))
                content += get(paper["abstract"]) + " ";

            if (paper.contains("submitter"))
                content += get(paper["submitter"]) + " ";

            if (paper.contains("authors_parsed") && paper["authors_parsed"].is_array()) {
                for (const auto& a : paper["authors_parsed"]) {
                    if (a.is_array() && a.size() >= 2) {
                        content += get(a[1]) + " ";
                        content += get(a[0]) + " ";
                        if (a.size() >= 3)
                            content += get(a[2]) + " ";
                    }
                }
            }

        } catch (...) {
            continue;
        }

        for (char& c : content)
            if (!std::isalnum(c, current_locale))
                c = ' ';

        // count of each word in doc will be used in creation of inverted index
        std::unordered_map<unsigned int, unsigned int> freq;


        std::istringstream iss(content);
        std::string w;

        while (iss >> w) {
            std::string cleaned = cleanWord(w);

            if (cleaned.empty()) continue;
            if (stopWords.count(cleaned)) continue;

            auto it = words.find(cleaned);
            if (it != words.end()) {
                unsigned int wid = it->second;
                freq[wid]++;
            }
        }

        f_index.emplace(id, freq);    // store the map instead of list
    }

    // Write output
    std::ofstream out("ForwardIndex.txt");
    for (const auto& entry : f_index) {
        out << entry.first << " : ";

        for (const auto& p : entry.second) {
            out << p.first << "(" << p.second << ") ";   // wid(count)
        }

        out << "\n";
    }
    out.close();
}

};

#endif
