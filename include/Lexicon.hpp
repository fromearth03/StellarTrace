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
            if (std::isalpha(c, current_locale))
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
                    //----------------------------------------------------------------------------------
                    auto safe_get_string = [](const json& j) -> std::string {
                        if (!j.is_null()) {
                            // Check if it is a string (optional, but robust)
                            if (j.is_string()) {
                                return j.get<std::string>();
                            }
                        }
                        return ""; // Return empty string if null or missing/non-string
                    };
                    //====================================================================================
                    if (paper.contains("title")) content += safe_get_string(paper["title"]) + " ";
                    if (paper.contains("abstract")) content += safe_get_string(paper["abstract"]) + " ";

                    if (paper.contains("submitter")) {
                        content += safe_get_string(paper["submitter"]) + " ";
                    }
                    if (paper.contains("authors_parsed") && paper["authors_parsed"].is_array()) {
                        const json& authorsArray = paper["authors_parsed"];

                        for (const auto& authorEntry : authorsArray) {
                            if (authorEntry.is_array() && authorEntry.size() >= 2) {

                                // Use safe_get_string for individual author name parts as well
                                std::string lastName = safe_get_string(authorEntry.at(0));
                                std::string firstName = safe_get_string(authorEntry.at(1));

                                content += firstName + " " + lastName + " ";

                                // Include middle initial/suffix if available (Index 2)
                                if (authorEntry.size() >= 3 && !authorEntry.at(2).empty()) {
                                    content += safe_get_string(authorEntry.at(2)) + " ";
                                }
                            }
                        }
                    }
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

    // Getter for read-only access to the word map
    const std::unordered_map<std::string, unsigned int>& getWords() const {
        return words;
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
