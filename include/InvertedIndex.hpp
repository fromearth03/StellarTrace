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
                            i_index[wid].push_back(std::make_tuple(docID, count, mask));
                        }
                    } catch (...) {
                        continue;
                    }
                }
            }
            f_index.emplace(docID, ids);
        }

        file.close();
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
        idf_calculate();

        std::ofstream out("InvertedIndex.txt");
        if (!out.is_open()) return;

        for (auto &entry : i_index) {
            unsigned int wid = entry.first;

            double idfVal = (idf.find(wid) != idf.end()) ? idf[wid] : 0.0;

            out << wid << " " << idfVal << " : ";
            for (auto &t : entry.second) {
                std::string docID;
                unsigned int count;
                int mask;
                std::tie(docID, count, mask) = t;
                out << docID << "(" << count << "," << mask << ") ";
            }
            out << "\n";
        }

        out.close();
    }
};

#endif