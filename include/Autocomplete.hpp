#ifndef AUTOCOMPLETE_HPP
#define AUTOCOMPLETE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <cctype>

class Autocomplete {
private:
    std::unordered_map<std::string, std::vector<std::string>> prefixMap;

    static constexpr size_t MIN_LEN = 3;
    static constexpr size_t MAX_PREFIX = 8;
    static constexpr size_t MAX_SUGGESTIONS = 18;

    std::string normalize(const std::string& s) const {
        std::string r;
        for (char c : s) {
            if (std::isalpha(static_cast<unsigned char>(c)))
                r += std::tolower(c);
        }
        return r;
    }

public:
    // ================= LOAD LEXICON =================
    void loadLexicon(const std::string& lexiconPath) {
        std::ifstream f(lexiconPath);
        if (!f.is_open()) return;

        std::string word;
        unsigned int id;

        while (f >> word >> id) {
            std::string w = normalize(word);
            if (w.size() < MIN_LEN) continue;

            // 🔥 STORE ALL PREFIXES
            for (size_t i = MIN_LEN; i <= w.size() && i <= MAX_PREFIX; ++i) {
                prefixMap[w.substr(0, i)].push_back(w);
            }
        }

        // Cleanup
        for (auto& [p, list] : prefixMap) {
            std::sort(list.begin(), list.end());
            list.erase(std::unique(list.begin(), list.end()), list.end());
            if (list.size() > 100)
                list.resize(100);
        }
    }

    // ================= QUERY =================
    std::vector<std::string> suggest(const std::string& input) const {
        std::string q = normalize(input);
        if (q.size() < MIN_LEN)
            return {};

        auto it = prefixMap.find(q);
        if (it == prefixMap.end())
            return {};

        std::vector<std::string> result;
        for (const auto& w : it->second) {
            result.push_back(w);
            if (result.size() >= MAX_SUGGESTIONS)
                break;
        }

        return result;
    }
};

#endif
