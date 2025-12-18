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
#include <iostream>
#include <vector>

class InvertedIndex {
    std::string path_lexicon;
    std::string path_forward;

    std::unordered_map<std::string, unsigned int> words;

    // In-Memory Inverted Index
    // Key: WordID (Sorted automatically by map)
    // Value: List of (DocID, Count, Mask)
    std::map<unsigned int, std::vector<std::tuple<std::string, unsigned int, int>>> i_index;

public:
    InvertedIndex(std::string p_Lexicon, std::string p_Forward) :
        path_lexicon(p_Lexicon), path_forward(p_Forward) {}

    void lexiconCreater() {
        std::ifstream ifs(path_lexicon);
        if (!ifs.is_open()) return;

        std::string word;
        std::string idStr;
        while (ifs >> word >> idStr) {
            idStr.erase(std::remove(idStr.begin(), idStr.end(), ','), idStr.end());
            words.emplace(word, std::stoul(idStr));
        }
        ifs.close();
        std::cout << "Lexicon Loaded. Total words: " << words.size() << std::endl;
    }

    void forward_inverted_Index_creator() {
        std::cout << "Reading Forward Index into Memory (One Pass)..." << std::endl;

        std::ifstream file(path_forward);
        if (!file.is_open()) {
            std::cout << "Error: Cannot open Forward Index!" << std::endl;
            return;
        }

        std::string line;
        unsigned int totalDocsN = 0; // Count N on the fly

        // STEP 1: READ FILE ONCE (O(N) Complexity)
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;

            // Extract DocID
            std::string docID = line.substr(0, colonPos);
            while (!docID.empty() && docID.back() == ' ') docID.pop_back();

            totalDocsN++; // Increment document count

            // Parse the rest of the line
            std::string wordIdsStr = line.substr(colonPos + 1);
            std::istringstream iss(wordIdsStr);
            std::string token;

            while (iss >> token) {
                // Token
                size_t p1 = token.find('(');
                size_t p3 = token.find(')');

                if (p1 != std::string::npos && p3 != std::string::npos) {
                    try {
                        // Extract Word ID
                        std::string idPart = token.substr(0, p1);
                        // Remove commas if present in ID
                        idPart.erase(std::remove(idPart.begin(), idPart.end(), ','), idPart.end());
                        unsigned int wid = std::stoul(idPart);

                        // Extract Count and Mask
                        size_t p2 = token.find(',', p1);
                        if (p2 != std::string::npos && p2 < p3) {
                            unsigned int count = std::stoul(token.substr(p1 + 1, p2 - p1 - 1));
                            int mask = std::stoi(token.substr(p2 + 1, p3 - p2 - 1));

                            // INSERT INTO MEMORY IMMEDIATELY
                            // This flips the index from Doc->Word to Word->Doc
                            i_index[wid].push_back(std::make_tuple(docID, count, mask));
                        }
                    } catch (...) {
                        continue;
                    }
                }
            }

            // Progress indicator for loading
            if (totalDocsN % 1000 == 0) {
                std::cout << "\r[Loading] Processed " << totalDocsN << " documents..." << std::flush;
            }
        }
        file.close();
        std::cout << "\nIndex Loaded in Memory. Total Documents: " << totalDocsN << std::endl;

        // STEP 2: WRITE TO FILE
        std::cout << "Writing Inverted Index to Disk..." << std::endl;

        std::ofstream out("inverted_index_tst.txt");
        if (!out.is_open()) return;

        size_t writeCount = 0;
        size_t totalWords = i_index.size();

        // Iterate through the map
        for (auto const& pair : i_index) {
            unsigned int wid = pair.first;
            auto const& postings = pair.second;

            unsigned int df = postings.size();
            // Calculate IDF
            double idfVal = (df > 0) ? std::log(static_cast<double>(totalDocsN) / df) : 0.0;

            // Write Header: "WordID IDF :"
            out << wid << " " << idfVal << " : ";

            // Write Postings: "DocID(Count,Mask) ..."
            for (const auto& entry : postings) {
                out << std::get<0>(entry) << "("
                    << std::get<1>(entry) << ","
                    << std::get<2>(entry) << ") ";
            }
            out << "\n";

            // Progress Update
            writeCount++;
            if (writeCount % 1000 == 0) {
                std::cout << "\r[Writing] Saved " << writeCount << " / " << totalWords << " words..." << std::flush;
            }
        }

        out.close();
        std::cout << "\nDone! 'inverted_index.txt' created successfully." << std::endl;
    }

    void invertedIndex_writer() {
        lexiconCreater();
        forward_inverted_Index_creator();
    }
};

#endif