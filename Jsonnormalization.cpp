// #include <iostream>
// #include <fstream>
// #include <string>
// #include <cctype>
// #include <unordered_set>
// #include <unordered_map>
// #include <sstream>
// #include <algorithm>
// #include <locale> // Required for std::locale functions
//
// // Assuming nlohmann/json is available
// #include <json.hpp>
//
// using json = nlohmann::json;
// using namespace std;
//
// class Lexicon {
//     private:
//     string path;
//     unsigned int wordID;
//     unordered_set<string> stopWords = {
//         "the", "and", "is", "in", "at", "of", "on", "for", "to", "a", "an", "that", "it"
//     };
//     unordered_map<string, unsigned int> words;
//
//     // Use a specific locale object for correct character handling
//     const locale& current_locale = locale("");
//
//     string cleanWord(const string& w) {
//         string result;
//         for (char c : w) {
//             // FIX: Use std::isalnum and std::tolower with the locale object
//             // This is the C++ standard way to attempt to handle non-ASCII/UTF-8 characters correctly.
//             if (std::isalnum(c, current_locale)) {
//                 result += std::tolower(c, current_locale);
//             }
//             // All non-alphanumeric characters (including punctuation, math symbols) are ignored/removed
//         }
//         return result;
//     }
//
// public:
//     Lexicon(string filename) {
//         path = filename;
//         wordID = 0;
//         // NOTE: For best results, main() should call std::locale::global(std::locale("")); or similar.
//     }
//
//     void readfile_createmap() {
//         std::ifstream file(path);
//         if (!file.is_open()) {
//             std::cout << "CRITICAL ERROR: Cannot open input file: " << path << std::endl;
//             return;
//         }
//
//         std::string line;
//         while (std::getline(file, line)) {
//             if (line.empty()) continue;
//
//             try {
//                 json paper = json::parse(line);
//
//                 std::string content;
//                 if (paper.contains("title")) content += paper["title"].get<std::string>() + " ";
//                 if (paper.contains("abstract")) content += paper["abstract"].get<std::string>();
//
//                 // --- FIX: Robust Delimiter Replacement (CRITICAL for concatenated words) ---
//                 std::replace_if(content.begin(), content.end(),
//                     [](char c) {
//                         // Target common non-alphanumeric delimiters like newlines, dashes, slashes, etc.
//                         return c == '\n' || c == '\r' || c == '-' || c == '/' || c == '.' || c == ',' || c == '(' || c == ')' || c == ':' || c == '$' || c == '\\' || c == '{' || c == '}';
//                     },
//                     ' ');
//
//                 // ---------------------------------------------------------------------
//
//                 std::istringstream iss(content);
//                 std::string word;
//
//                 while (iss >> word) {
//                     std::string cleaned = cleanWord(word);
//
//                     if (!cleaned.empty()) {
//
//                         if (stopWords.find(cleaned) == stopWords.end()) {
//
//                             if (words.find(cleaned) == words.end()) {
//
//                                 words.emplace(cleaned, ++wordID);
//                             }
//                         }
//                     }
//                 }
//
//             } catch (json::parse_error& e) {
//                 continue;
//             } catch (const json::out_of_range& e) {
//                 continue;
//             }
//         }
//         std::cout << "Successfully processed " << words.size() << " unique words." << std::endl;
//         file.close();
//     }
//
//     void createLexicon() {
//         ofstream out("Lexicon(temporary).txt");
//
//         if (!out.is_open()) {
//             cout << "CRITICAL ERROR: Cannot open output file Lexicon.txt!\n";
//             return;
//         }
//
//         for (const auto &p : words) {
//             out << p.first << " " << p.second << "\n";
//         }
//
//         out.close();
//         cout << "Lexicon.txt written. Total unique words saved: " << wordID << "\n";
//     }
// };
//
// int main() {
//     // Note: Setting the global locale helps the standard library functions (like isalnum)
//     // recognize non-ASCII characters if the underlying OS supports it.
//     try {
//         std::locale::global(std::locale(""));
//     } catch (...) {
//         // Fallback if locale setting fails
//         std::cerr << "Warning: Failed to set global locale for full UTF-8 support." << std::endl;
//     }
//
//     Lexicon lexicon("arxiv-metadata.json");
//     lexicon.readfile_createmap();
//     lexicon.createLexicon();
//     return 0;
// }