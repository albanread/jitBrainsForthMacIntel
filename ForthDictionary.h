/// @file ForthDictionary.h
/// @brief This file contains the definitions and declarations for managing a dictionary in a Forth interpreter.
///
/// The Forth dictionary is a central component of the Forth language, encapsulating the words
/// and their associated executable code or data. This implementation provides the functionality
/// to add, manage, and query words in the dictionary, supporting typical Forth operations such
/// as defining words, constants, and variables.
///
/// The ForthDictionary class is implemented as a singleton, ensuring that only one instance
/// of the dictionary exists during the lifecycle of the program. The class manages the memory
/// allocation for the dictionary, supports adding new words, and provides mechanisms to find
/// and manipulate existing words. Auxiliary classes and types are defined to support these
/// operations, such as ForthWord, ForthWordType, ForthWordState, and ForthFunction.
///
/// Additionally, the file includes utility functions for converting word types and states to strings,
/// assisting in debugging and displaying the dictionary contents.
///
/// Dependencies:
/// - <iostream>
/// - <cstring>
/// - <vector>
/// - <cstdint>
/// - <unordered_map>
/// - "utility.h"
///
/// Example Usage:
/// @code
/// ForthDictionary& dict = ForthDictionary::getInstance(1024 * 1024);
/// dict.addWord("example", myGeneratorFunc, myCompiledFunc, myImmediateFunc, myInterpFunc);
/// ForthWord* foundWord = dict.findWord("example");
/// if (foundWord) {
///     // Perform operations with the found word
/// }
/// @endcode

#ifndef FORTH_DICTIONARY_H
#define FORTH_DICTIONARY_H

#include <iostream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "utility.h"
#include <variant>

// Function pointer type for Forth functions
typedef void (*ForthFunction)();

// Enum representing various states a Forth word can have
enum ForthWordState
{
    NORMAL = 0,
    IMMEDIATE = 1 << 0,
    COMPILE_ONLY = 1 << 1,
    INTERPRET_ONLY = 1 << 2,
    COMPILE_ONLY_IMMEDIATE = COMPILE_ONLY | IMMEDIATE,
    INTERPRET_ONLY_IMMEDIATE = INTERPRET_ONLY | IMMEDIATE,
};

// Convert ForthWordState to a string for debugging
inline std::string ForthWordStateToString(const ForthWordState state)
{
    switch (state)
    {
    case NORMAL: return "NORMAL";
    case IMMEDIATE: return "IMMEDIATE";
    case COMPILE_ONLY: return "COMPILE_ONLY";
    case INTERPRET_ONLY: return "INTERPRET_ONLY";
    case COMPILE_ONLY_IMMEDIATE: return "COMPILE_ONLY_IMMEDIATE";
    case INTERPRET_ONLY_IMMEDIATE: return "INTERPRET_ONLY_IMMEDIATE";
    default: return "UNKNOWN";
    }
}

// Enum representing various types a Forth word can have
enum ForthWordType {
    // Base type categories
    WORD         = 1 << 0,  // General word
    CONSTANT     = 1 << 1,  // Constant value
    VARIABLE     = 1 << 2,  // Mutable variable
    VALUE        = 1 << 3,  // Like VARIABLE, but with immediate assignment
    RECORD       = 1 << 4,  // User-defined record type

    // Data type categories
    INTEGER      = 1 << 5,  // Default number type
    FLOAT        = 1 << 6,  // Floating-point number
    STRING       = 1 << 7,  // String type

    // Array modifiers (can be combined with data types)
    ARRAY        = 1 << 8,  // One-dimensional array
    TWODIM       = 1 << 9,  // Two-dimensional array



    // Combined types using logical hierarchy
    FLOATCONSTANT = CONSTANT | FLOAT,  // Floating-point constant
    FLOATVALUE    = VALUE | FLOAT ,     // Float assigned to a VALUE
    FLOATVARIABLE = VARIABLE | FLOAT,   // Floating-point variable
    INTEGERVALUE  = VALUE | INTEGER,   // Integer assigned to a VALUE
    INTEGERARRAY  = ARRAY | INTEGER,   // Array of integers
    STRINGVALUE   = VALUE | STRING,    // String assigned to a VALUE
    FLOATARRAY    = ARRAY | FLOAT,     // Array of floating-point values
    STRINGARRAY   = ARRAY | STRING,    // Array of strings




};
inline std::string ForthWordTypeToString(ForthWordType type) {
    std::vector<std::string> components;
    if (type & WORD) components.push_back("WORD");
    // Base type category
    if (type & CONSTANT) components.push_back("CONSTANT");
    if (type & VARIABLE) components.push_back("VARIABLE");
    if (type & VALUE) components.push_back("VALUE");
    if (type & RECORD) components.push_back("RECORD");

    // Data type category
    if (type & INTEGER) components.push_back("INTEGER");
    if (type & FLOAT) components.push_back("FLOAT");
    if (type & STRING) components.push_back("STRING");

    // Array type
    if (type & ARRAY) components.push_back("ARRAY");
    if (type & TWODIM) components.push_back("2D ARRAY");

    // If no flags matched, return UNKNOWN
    if (components.empty()) return "UNKNOWN";

    // Convert vector to a space-separated string
    std::ostringstream oss;
    for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0) oss << " ";
        oss << components[i];
    }

    return oss.str();
}


// Tokenizer class to split Forth source code into words
class ForthTokenizer {
public:
    explicit ForthTokenizer(const std::string& source) {
        tokenize(source);
        currentIndex = 0;
    }

    // Move to the next word in the source code
    void next() {
        if (currentIndex < words.size()) {
            currentIndex++;
        }
    }

    // Get the current word
    std::string current() const {
        return words[currentIndex];
    }

    // Check if the tokenizer has more words
    bool hasNext() const {
        return currentIndex < words.size() - 1;
    }

private:
    std::vector<std::string> words;
    size_t currentIndex;

    // Split the source code into words
    void tokenize(const std::string& source) {
        std::istringstream iss(source);
        std::string word;

        while (iss >> word) {
            std::ranges::transform(word, word.begin(), ::tolower);
            words.push_back(word);
        }
    }
};

using DataVariant = std::variant<uint64_t, double, void*>;
// Structure to represent a word in the dictionary
struct ForthWord
{
    char name[32]{}; // Name of the word (fixed length for simplicity)
    ForthFunction compiledFunc; // Compiled Forth function pointer
    ForthFunction generatorFunc; // Used to generate 'inline' code
    ForthFunction immediateFunc; // Immediate function pointer
    ForthFunction terpFunc; // Function pointer for the interpreter
    ForthWord* link; // Pointer to the previous word in the dictionary
    ForthWordState state; // State of the word
    uint8_t reserved; // Reserved for future use
    ForthWordType type; // Type of the word
    DataVariant data; // Holds uint64_t, double or void*

    // Constructor to initialize a word
    ForthWord(const char* wordName,
              ForthFunction genny = nullptr,
              ForthFunction func = nullptr,
              ForthFunction immFunc = nullptr,
              ForthFunction terpFunc = nullptr,
              ForthWord* prev = nullptr)
        : generatorFunc(genny), compiledFunc(func),
          immediateFunc(immFunc), terpFunc(terpFunc), type(WORD),
          link(prev), state(ForthWordState::NORMAL), data(uint64_t(0)) // Default initialize to uint64_t(0)
    {
        std::strncpy(name, wordName, sizeof(name));
        name[sizeof(name) - 1] = '\0'; // Ensure null-termination
    }


    // Accessor methods for data
    void setData(uint64_t value) { data = value; }
    void setData(double value) { data = value; }
    void setData(void* value) { data = value; }

    uint64_t getUint64() const
    {
        if (std::holds_alternative<uint64_t>(data))
        {
            return std::get<uint64_t>(data);
        }
        throw std::runtime_error("Data does not hold a uint64_t");
    }

    double getDouble() const
    {
        if (std::holds_alternative<double>(data))
        {
            return std::get<double>(data);
        }
        throw std::runtime_error("Data does not hold a double");
    }

    void* getPointer() const
    {
        if (std::holds_alternative<void*>(data))
        {
            return std::get<void*>(data);
        }
        throw std::runtime_error("Data does not hold a void*");
    }
};



// Class to manage the Forth dictionary
class ForthDictionary
{
public:
    // Static method to get the singleton instance
    static ForthDictionary& getInstance(size_t size = 1024 * 1024 * 8);

    // Delete copy constructor and assignment operator to prevent copies
    ForthDictionary(const ForthDictionary&) = delete;
    ForthDictionary& operator=(const ForthDictionary&) = delete;

    // Add a new word to the dictionary
    void addWord(const char* name, ForthFunction generatorFunc, ForthFunction compiledFunc, ForthFunction immediateFunc,
                 ForthFunction immTerpFunc, const std::string& sourceCode);
    void addWord(const char* name, ForthFunction generatorFunc, ForthFunction compiledFunc, ForthFunction immediateFunc,
                 ForthFunction immTerpFunc);
    void addConstant(const char* name, ForthFunction generatorFunc, ForthFunction compiledFunc,
                     ForthFunction immediateFunc,
                     ForthFunction immTerpFunc);
    void addCompileOnlyImmediate(const char* name, ForthFunction generatorFunc, ForthFunction compiledFunc,
                                 ForthFunction immediateFunc, ForthFunction immTerpFunc);
    void addInterpretOnlyImmediate(const char *name,
                                   ForthFunction immTerpFunc);

    // Find a word in the dictionary
    ForthWord* findWord(const char* name) const;

    // Allot space in the dictionary
    void allot(size_t bytes);

    // Store data in the dictionary
    void storeData(const void* data, size_t dataSize);

    // Get the latest added word
    [[nodiscard]] ForthWord* getLatestWord() const;
    [[nodiscard]] uint64_t getCurrentPos() const;
    [[nodiscard]] uint64_t getCurrentLocation() const;

    // Add base words to the dictionary
    static void add_base_words();
    void forgetLastWord();
    void setData(uint64_t data) const;
    void setDataDouble(double data) const;
    void setData(double data) const;
    void setData(void* data) const;
    void setCompiledFunction(ForthFunction func) const;
    void setImmediateFunction(ForthFunction func) const;
    void setGeneratorFunction(ForthFunction func) const;
    void setTerpFunction(ForthFunction func) const;
    void setState(ForthWordState i) const;
    [[nodiscard]] ForthWordState getState() const;
    void setName(std::string name);
    void setData(uint64_t d);
    uint64_t getData() const;
    double getDataAsDouble() const;
    void* getDataAsPointer() const;
    ForthWordType getType() const;
    void setType(ForthWordType type) const;
    void* get_data_ptr() const;
    void displayWord(std::string name);
    void SetState(uint8_t i);

    // List all words in the dictionary
    void list_words() const;

private:
    // Private constructor to prevent instantiation
    explicit ForthDictionary(size_t size);

    std::vector<char> memory; // Memory buffer for the dictionary
    size_t currentPos; // Current position in the memory buffer
    ForthWord* latestWord; // Pointer to the latest added word

    // Map to store the source code associated with each word
    std::unordered_map<std::string, std::string> sourceCodeMap;
};

#endif // FORTH_DICTIONARY_H