#ifndef INVERTED_INDEX_HPP
#define INVERTED_INDEX_HPP

#include <list>
#include <string>
#include <unordered_map>
#include <map>
#include <tuple>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

class InvertedIndex {
    std::string path_lexicon;
    std::string path_dataset;
    std::string path_forward;

    std::unordered_map<std::string, unsigned int> words;
    std::unordered_map<std::string, std::list<unsigned int>> f_index;

    // std::map ensures output is sorted by WordID
    std::map<unsigned int, std::list<std::tuple<std::string, unsigned int, int>>> i_index;

    std::unordered_map<unsigned int, double> idf;

public:
    InvertedIndex(std::string p_Lexicon, std::string p_Dataset, std::string p_Forward) :
        path_lexicon(p_Lexicon), path_dataset(p_Dataset), path_forward(p_Forward) {}

    void lexiconCreater() {
        std::ifstream ifs(path_lexicon);
        if (!ifs.is_open()) return;

        std::string word;
        std::string idStr;
        while (ifs >> word >> idStr) {
            // Clean commas from lexicon IDs if they exist there too
            idStr.erase(std::remove(idStr.begin(), idStr.end(), ','), idStr.end());
            words.emplace(word, std::stoul(idStr));
        }

        ifs.close();
    }

    void forward_inverted_Index_creator() {
        // First, scan the file to build f_index for number_docs
        std::ifstream file(path_forward);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;

            std::string docID = line.substr(0, colonPos);
            while (!docID.empty() && docID.back() == ' ') docID.pop_back();

            std::string wordIdsStr = line.substr(colonPos + 1);

            std::list<unsigned int> ids;
            std::istringstream iss(wordIdsStr);
            std::string token;
            while (iss >> token) {
                // Token format: "3,150(1,2)" or "93(1,2)"
                size_t p1 = token.find('(');
                size_t p3 = token.find(')');

                if (p1 != std::string::npos && p3 != std::string::npos) {
                    try {
                        // 1. Handle Word ID (remove commas strictly from the ID part)
                        std::string idPart = token.substr(0, p1);
                        idPart.erase(std::remove(idPart.begin(), idPart.end(), ','), idPart.end());
                        unsigned int wid = std::stoul(idPart);

                        // 2. Handle (Count, Mask) - Find comma AFTER the opening parenthesis
                        size_t p2 = token.find(',', p1);

                        if (p2 != std::string::npos && p2 < p3) {
                            unsigned int count = std::stoul(token.substr(p1 + 1, p2 - p1 - 1));
                            int mask = std::stoi(token.substr(p2 + 1, p3 - p2 - 1));

                            ids.push_back(wid);
                            // Removed i_index building here to avoid building full inverted index
                        }
                    } catch (...) {
                        continue;
                    }
                }
            }
            f_index.emplace(docID, ids);
        }

        file.close();

        // Now, open output file
        std::ofstream out("InvertedIndextest1.txt");
        if (!out.is_open()) return;

        // For each word in lexicon, scan the file again to collect postings for that word, compute IDF, write, and free memory
        for (auto& [word, wid] : words) {
            std::list<std::tuple<std::string, unsigned int, int>> postings;
            unsigned int df = 0;

            std::ifstream file2(path_forward);
            std::string line2;
            while (std::getline(file2, line2)) {
                if (line2.empty()) continue;

                size_t colonPos2 = line2.find(':');
                if (colonPos2 == std::string::npos) continue;

                std::string docID2 = line2.substr(0, colonPos2);
                while (!docID2.empty() && docID2.back() == ' ') docID2.pop_back();

                std::string wordIdsStr2 = line2.substr(colonPos2 + 1);

                std::istringstream iss2(wordIdsStr2);
                std::string token2;
                while (iss2 >> token2) {
                    size_t p1 = token2.find('(');
                    size_t p3 = token2.find(')');

                    if (p1 != std::string::npos && p3 != std::string::npos) {
                        try {
                            std::string idPart = token2.substr(0, p1);
                            idPart.erase(std::remove(idPart.begin(), idPart.end(), ','), idPart.end());
                            unsigned int w = std::stoul(idPart);

                            if (w == wid) {
                                size_t p2 = token2.find(',', p1);

                                if (p2 != std::string::npos && p2 < p3) {
                                    unsigned int count = std::stoul(token2.substr(p1 + 1, p2 - p1 - 1));
                                    int mask = std::stoi(token2.substr(p2 + 1, p3 - p2 - 1));

                                    postings.push_back(std::make_tuple(docID2, count, mask));
                                    ++df;
                                }
                            }
                        } catch (...) {
                            continue;
                        }
                    }
                }
            }
            file2.close();

            unsigned int N = f_index.size();
            double idfVal = (df > 0) ? std::log(static_cast<double>(N) / df) : 0.0;

            out << wid << " " << idfVal << " : ";
            for (auto &t : postings) {
                std::string docID;
                unsigned int count;
                int mask;
                std::tie(docID, count, mask) = t;
                out << docID << "(" << count << "," << mask << ") ";
            }
            out << "\n";

            // postings is automatically cleared when going out of scope, freeing memory for next word
        }

        out.close();
    }

    unsigned int number_docs() {
        return f_index.size();
    }

    void idf_calculate() {
        unsigned int N = number_docs();
        for (auto const& [wid, entry] : i_index) {
            unsigned int df = entry.size();
            if (df > 0)
                idf[wid] = std::log(static_cast<double>(N) / df);
            else
                idf[wid] = 0.0;
        }
    }

    void invertedIndex_writer() {
        lexiconCreater();
        forward_inverted_Index_creator();
        // Removed idf_calculate and writing since it's now done inside forward_inverted_Index_creator for memory efficiency
    }
};

#endif
