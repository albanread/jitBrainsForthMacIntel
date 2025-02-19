#ifndef COMPILERUTILITY_H
#define COMPILERUTILITY_H
#include <iostream>
#include <unordered_set>
#include "ForthDictionary.h"
#include "JitGenerator.h"


// Declaration of compileWord function
void compileWord(const std::string& wordName, const std::string& compileText, const std::string& sourceCode);

// Traced words set
inline std::unordered_set<std::string> tracedWords;

inline void traceOn(const std::string& word)
{
    tracedWords.insert(word);
    std::cout << "Tracing enabled for: " << word << std::endl;
}

inline void traceOff(const std::string& word)
{
    tracedWords.erase(word);
    std::cout << "Tracing disabled for: " << word << std::endl;
}

inline void clearR15()
{
    asm volatile (
        "xor %%r15, %%r15"
        :
        :
        : "r15"
    );
}

inline void exec(ForthFunction f)
{
    clearR15();
    f();
}

inline double parseFloat(const std::string& word) {
    if (word.empty()) {
        throw std::invalid_argument("Empty string is not a valid number");
    }

    size_t startIndex = 0;
    bool isNegative = false;

    // Check for an optional leading minus sign for decimal numbers
    if (word[0] == '-') {
        isNegative = true;
        startIndex = 1;
    }

    // Default to floating-point parsing
    double number = std::stod(word.substr(startIndex)); // Convert to double

    // Apply the negative sign if necessary
    if (isNegative) {
        number = -number;
    }

    return number;
}


inline int64_t parseNumber(const std::string& word) {
    if (word.empty()) {
        throw std::invalid_argument("Empty string is not a valid number");
    }

    size_t startIndex = 0;
    bool isNegative = false;

    // Check for an optional leading minus sign for decimal numbers
    if (word[0] == '-') {
        isNegative = true;
        startIndex = 1;
    }

    // Handle hexadecimal and binary prefixes
    if (startIndex + 2 < word.length() && word[startIndex] == '0') {
        if (word[startIndex + 1] == 'x' || word[startIndex + 1] == 'X') {
            return std::stoull(word.substr(startIndex), nullptr, 16); // Hexadecimal (unsigned)
        } else if (word[startIndex + 1] == 'b' || word[startIndex + 1] == 'B') {
            return std::stoull(word.substr(startIndex + 2), nullptr, 2); // Binary (unsigned)
        }
    }

    // Default to decimal
    int64_t number = std::stoll(word.substr(startIndex), nullptr, 10); // Decimal

    // Apply the negative sign if necessary
    if (isNegative) {
        number = -number;
    }

    return number;
}

// also removes comments between ( and )
inline std::string scanForLiterals(const std::string& compileText)
{
    // Regex to match literals (s" ..." or .")
    std::regex literalRegex(R"((\w*")\s(.*?[^\\])\")");
    // Regex to match ( ... ) comments
    std::regex commentRegex(R"(\(.*?\))");

    std::string result;
    std::smatch match;
    std::string tmpText = compileText;


    // Remove comments from the input text
    tmpText = std::regex_replace(tmpText, commentRegex, "");

    // Process literals
    while (std::regex_search(tmpText, match, literalRegex))
    {
        std::string literalStart = match[1].str(); // s" or ."
        std::string literalString = match[2].str(); // The literal content after the first space
        printf("Literal found: [%s] [%s]\n", literalStart.c_str(), literalString.c_str());
        tmpText = "@1";

    }
    result += tmpText;

    return result;
}

inline std::vector<std::string> splitAndLogWords(const std::string& sourceCode)
{
    std::string newCompileText = scanForLiterals(sourceCode);
    const std::vector<std::string> words = split(newCompileText);

    if (logging)
    {
        printf("Split words: ");
        for (const auto& word : words)
        {
            printf("%s ", word.c_str());
        }
        printf("\n");
    }

    return words;
}



// Function to handle compile mode (defining new words)
// inline void handleCompileMode(size_t& i, const std::vector<std::string>& words, const std::string& sourceCode)
// {
//     if (logging) printf("Entering compile mode to define a new word.\n");
//     ++i;
//     std::string wordName;
//
//     if (i < words.size())
//     {
//         wordName = words[i];
//         if (logging) printf("New word definition: [%s]\n", wordName.c_str());
//     }
//     else
//     {
//         if (logging) printf("Error: No word name provided after ':'\n");
//         throw std::runtime_error("Interpreter Error: No word name provided after ':'");
//     }
//
//     std::string compileText;
//
//     ++i;
//     while (i < words.size() && words[i] != ";")
//     {
//         compileText += words[i] + " ";
//         ++i;
//     }
//
//     if (i >= words.size())
//     {
//         if (logging) printf("Error: No ending ';' found for word definition.\n");
//         throw std::runtime_error("Interpreter Error: No ending ';' found for word definition.");
//     }
//
//
//     compileWord(wordName, compileText, sourceCode);
//
//     ++i;
// }


#endif //COMPILERUTILITY_H
