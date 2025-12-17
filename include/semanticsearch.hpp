#ifndef SEMANTIC_SEARCH_HPP
#define SEMANTIC_SEARCH_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cmath>

class SemanticSearch {
private:
    const std::unordered_set<std::string>& lexicon;

public:
    explicit SemanticSearch(const std::unordered_set<std::string>& lex)
        : lexicon(lex) {}

    // ================= MAIN ENTRY =================
    std::string rewriteQuery(const std::string& query) const {
        std::stringstream ss(query);
        std::string token;
        std::string rewritten;

        while (ss >> token) {
            std::string resolved = resolveToken(token);
            if (!resolved.empty())
                rewritten += resolved + " ";
        }

        return rewritten;
    }

private:
    // ---------------- RESOLVE SINGLE TOKEN ----------------
    std::string resolveToken(const std::string& input) const {
        std::string w = normalize(input);
        if (w.empty()) return "";

        // 1️⃣ Exact match
        if (lexicon.count(w))
            return w;

        // 2️⃣ Morphological normalization (Porter stem)
        std::string stem = porterStem(w);
        if (!stem.empty() && lexicon.count(stem))
            return stem;

        // 3️⃣ Typo correction (edit distance 1)
        for (const auto& term : lexicon) {
            if (editDistanceOne(w, term))
                return term;
        }

        return "";
    }

    // ---------------- NORMALIZE ----------------
    std::string normalize(const std::string& s) const {
        std::string r;
        for (char c : s) {
            if (std::isalpha(static_cast<unsigned char>(c)))
                r += std::tolower(c);
        }
        return r;
    }

    // ---------------- PORTER STEM (MINIMAL) ----------------
    std::string porterStem(std::string w) const {
        if (w.size() < 4) return w;

        auto ends = [&](const std::string& s) {
            return w.size() >= s.size() &&
                   w.compare(w.size() - s.size(), s.size(), s) == 0;
        };

        if (ends("sses")) w.erase(w.size() - 2);
        else if (ends("ies")) w.erase(w.size() - 2);
        else if (ends("s") && !ends("ss")) w.pop_back();

        if (ends("ing")) {
            std::string base = w.substr(0, w.size() - 3);
            if (hasVowel(base)) w = base;
        } else if (ends("ed")) {
            std::string base = w.substr(0, w.size() - 2);
            if (hasVowel(base)) w = base;
        }

        if (ends("y") && hasVowel(w.substr(0, w.size() - 1)))
            w[w.size() - 1] = 'i';

        return w;
    }

    bool hasVowel(const std::string& s) const {
        for (char c : s)
            if (c=='a'||c=='e'||c=='i'||c=='o'||c=='u')
                return true;
        return false;
    }

    // ---------------- EDIT DISTANCE = 1 ----------------
    bool editDistanceOne(const std::string& a,
                         const std::string& b) const {
        int la = a.size(), lb = b.size();
        if (std::abs(la - lb) > 1) return false;

        int i = 0, j = 0, edits = 0;
        while (i < la && j < lb) {
            if (a[i] == b[j]) {
                i++; j++;
            } else {
                if (++edits > 1) return false;
                if (la > lb) i++;
                else if (lb > la) j++;
                else { i++; j++; }
            }
        }
        return true;
    }
};

#endif
