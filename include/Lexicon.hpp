#ifndef LEXICON_HPP
#define LEXICON_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <locale>
#include <json.hpp>

using json = nlohmann::json;

class Lexicon {
private:
    std::string path;                 // Path to input file
    unsigned int wordID;              // Counter for unique words
    bool isJson;                      // Flag: true for JSON, false for plain text (.wet)

    std::unordered_set<std::string> stopWords = {
        "the", "and", "is", "in", "at", "of", "on", "for", "to", "a", "an", "that", "it"
    }; // Common words to ignore

    std::unordered_map<std::string, unsigned int> words; // Stores word -> ID mapping
    std::locale current_locale;                           // Locale for proper Unicode handling

    // Clean a word: keep only letters/numbers, lowercase it
    std::string cleanWord(const std::string& w) {
        std::string result;
        for (char c : w) {
            if (std::isalnum(c, current_locale))
                result += std::tolower(c, current_locale);
        }
        return result;
    }

public:
    // Constructor: second parameter indicates JSON vs plain text
    Lexicon(const std::string& filename, bool jsonFile = true)
        : path(filename), wordID(0), isJson(jsonFile)
    {
        try {
            current_locale = std::locale(""); // Use system UTF-8 locale
        } catch (...) {
            std::cout << "Warning: Unable to use UTF-8 locale. Using classic locale.\n";
            current_locale = std::locale::classic();
        }
    }

    // Read file and build the lexicon map
    void readfile_createmap() {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cout << "CRITICAL ERROR: Cannot open file: " << path << "\n";
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::string content;

            if (isJson) {
                // Process JSON lines: extract "title" and "abstract"
                try {
                    json paper = json::parse(line);
                    if (paper.contains("title")) content += paper["title"].get<std::string>() + " ";
                    if (paper.contains("abstract")) content += paper["abstract"].get<std::string>();
                } catch (json::parse_error&) { continue; }
                  catch (json::out_of_range&) { continue; }
            } else {
                // Plain text (.wet) files: take the entire line
                content = line;
            }

            // Replace all punctuation/non-alphanumeric chars with space
            std::replace_if(
                content.begin(), content.end(),
                [](char c) { return !std::isalnum(c, std::locale()); },
                ' '
            );

            // Split line into words
            std::istringstream iss(content);
            std::string word;
            while (iss >> word) {
                std::string cleaned = cleanWord(word);

                // Add unique, non-stopword words to map
                if (!cleaned.empty() &&
                    stopWords.find(cleaned) == stopWords.end() &&
                    words.find(cleaned) == words.end()) {
                    words.emplace(cleaned, ++wordID);
                }
            }
        }

        std::cout << "Processed " << words.size() << " unique words.\n";
        file.close();
    }

    // Save lexicon to a text file
    void createLexicon() {
        std::filesystem::create_directories("Lexicon"); // ensure folder exists

        // Extract just the input file name without directories
        std::filesystem::path inputPath(path);
        std::string filename = inputPath.stem().string(); // "arxiv-metadata" for example

        // Safe output path in Lexicon folder
        std::string outputfile = "Lexicon/Lexicon (" + filename + ").txt";

        std::ofstream out(outputfile);
        if (!out.is_open()) {
            std::cout << "CRITICAL ERROR: Cannot open output file.\n";
            return;
        }

        for (const auto& p : words)
            out << p.first << " " << p.second << "\n";

        out.close();
        std::cout << "Lexicon written to " << outputfile
                  << ". Total unique words: " << wordID << "\n";
    }

};

#endif // LEXICON_HPP
