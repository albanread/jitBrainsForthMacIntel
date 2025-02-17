#include "ForthDictionary.h"
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include "JitGenerator.h"
#include <string>
#include <set>


// Static method to get the singleton instance
ForthDictionary& ForthDictionary::getInstance(size_t size)
{
    static ForthDictionary instance(size);
    return instance;
}

// Private constructor to prevent instantiation
ForthDictionary::ForthDictionary(size_t size) : memory(size), currentPos(0), latestWord(nullptr)
{
}

// Add a new word to the dictionary
void ForthDictionary::addWord(const char* name,
                              ForthFunction generatorFunc,
                              ForthFunction compiledFunc,
                              ForthFunction immediateFunc,
                              ForthFunction immTerpFunc,
                              const std::string& sourceCode = "")
{
    std::string lower_name = to_lower(name);

    if (currentPos + sizeof(ForthWord) > memory.size())
    {
        throw std::runtime_error("Dictionary memory overflow");
    }

    void* wordMemory = &memory[currentPos];
    auto* newWord = new(wordMemory) ForthWord(lower_name.c_str(),
                                              generatorFunc,
                                              compiledFunc,
                                              immediateFunc,
                                              immTerpFunc,
                                              latestWord);

    // Correctly set the latest word to the new word
    latestWord = newWord;

    // Store the source code in the map
    sourceCodeMap[lower_name] = sourceCode;

    currentPos += sizeof(ForthWord);
    allot(16); // Increase current position for potentially more data
}

void ForthDictionary::addWord(const char* name,
                              ForthFunction generatorFunc,
                              ForthFunction compiledFunc,
                              ForthFunction immediateFunc,
                              ForthFunction immTerpFunc)
{
    addWord(name, generatorFunc, compiledFunc, immediateFunc, immTerpFunc, "");
}

void ForthDictionary::addConstant(const char* name,
                                  ForthFunction generatorFunc,
                                  ForthFunction compiledFunc,
                                  ForthFunction immediateFunc,
                                  ForthFunction immTerpFunc)
{
    addWord(name, generatorFunc, compiledFunc, immediateFunc, immTerpFunc, "");
    latestWord->type = ForthWordType::CONSTANT;
}

void ForthDictionary::addCompileOnlyImmediate(const char* name,
                                              ForthFunction generatorFunc,
                                              ForthFunction compiledFunc,
                                              ForthFunction immediateFunc,
                                              ForthFunction immTerpFunc)
{
    addWord(name, generatorFunc, compiledFunc, immediateFunc, immTerpFunc, "");
    latestWord->type = ForthWordType::WORD;
    latestWord->state = ForthWordState::COMPILE_ONLY_IMMEDIATE;
}

void ForthDictionary::addInterpretOnlyImmediate(const char* name,
                                                ForthFunction generatorFunc,
                                                ForthFunction compiledFunc,
                                                ForthFunction immediateFunc,
                                                ForthFunction immTerpFunc)
{
    addWord(name, generatorFunc, compiledFunc, immediateFunc, immTerpFunc, "");
    latestWord->type = ForthWordType::WORD;
    latestWord->state = ForthWordState::INTERPRET_ONLY_IMMEDIATE;
}


// Find a word in the dictionary
ForthWord* ForthDictionary::findWord(const char* name) const
{
    std::string lower_name = to_lower(name);
    ForthWord* word = latestWord;
    while (word != nullptr)
    {
        if (std::strcmp(word->name, lower_name.c_str()) == 0)
        {
            return word;
        }
        word = word->link;
    }
    return nullptr;
}

// Allot space in the dictionary
void ForthDictionary::allot(size_t bytes)
{
    if (currentPos + bytes > memory.size())
    {
        throw std::runtime_error("Dictionary memory overflow");
    }
    currentPos += bytes;
}

// Store data in the dictionary
void ForthDictionary::storeData(const void* data, size_t dataSize)
{
    if (currentPos + dataSize > memory.size())
    {
        throw std::runtime_error("Dictionary memory overflow");
    }
    std::memcpy(&memory[currentPos], data, dataSize);
    currentPos += dataSize;
}

// Get the latest added word
ForthWord* ForthDictionary::getLatestWord() const
{
    return latestWord;
}

uint64_t ForthDictionary::getCurrentPos() const
{
    return (uint64_t)currentPos;
}

uint64_t ForthDictionary::getCurrentLocation() const
{
    return reinterpret_cast<uint64_t>(&memory[currentPos]);
}

// Add base words to the dictionary
void ForthDictionary::add_base_words()
{
    // Add base words, placeholder for actual implementation
}

void ForthDictionary::forgetLastWord()
{
    if (latestWord == nullptr)
    {
        throw std::runtime_error("No words to forget");
    }

    std::cout << "Forgetting word " << latestWord->name << std::endl;

    // Remove source code entry
    sourceCodeMap.erase(latestWord->name);

    // Size of the word (this depends on your actual implementation details. Adjust as needed).
    size_t wordSize = sizeof(ForthWord) + 16; // include the extra allotted space
    currentPos -= wordSize;

    // Update the latest word pointer
    ForthWord* previousWord = latestWord->link;
    latestWord = previousWord;

    // Additional check to update the linking properly in dictionary
    if (latestWord != nullptr)
    {
        std::cout << "New latest word is " << latestWord->name << std::endl;
    }
    else
    {
        std::cout << "No more words in dictionary" << std::endl;
    }
}

// set the data field
void ForthDictionary::setData(uint64_t data) const
{
    latestWord->data = data;
}


void ForthDictionary::setDataDouble(const double data) const
{
    if (latestWord == nullptr)
    {
        throw std::runtime_error("No latest word available to set data");
    }
    latestWord->setData(data);
}

void ForthDictionary::setData(void* data) const
{
    if (latestWord == nullptr)
    {
        throw std::runtime_error("No latest word available to set data");
    }
    latestWord->setData(data);
}

void ForthDictionary::setCompiledFunction(ForthFunction func) const
{
    latestWord->compiledFunc = func;
}

void ForthDictionary::setImmediateFunction(ForthFunction func) const
{
    latestWord->immediateFunc = func;
}

void ForthDictionary::setGeneratorFunction(ForthFunction func) const
{
    latestWord->generatorFunc = func;
}

void ForthDictionary::setTerpFunction(ForthFunction func) const
{
    latestWord->terpFunc = func;
}

void ForthDictionary::setState(const ForthWordState i) const
{
    latestWord->state = i;
}

ForthWordState ForthDictionary::getState() const
{
    return latestWord->state;
}

void ForthDictionary::setName(std::string name)
{
    std::strncpy(latestWord->name, name.c_str(), sizeof(latestWord->name));
    latestWord->name[sizeof(latestWord->name) - 1] = '\0'; // Ensure null-termination
}

void ForthDictionary::setData(uint64_t d)
{
    latestWord->data = d;
}

 uint64_t ForthDictionary::getData() const
{
    if (latestWord == nullptr)
    {
        throw std::runtime_error("No latest word available to get data");
    }
    return latestWord->getUint64();
}

double ForthDictionary::getDataAsDouble() const
{
    if (latestWord == nullptr)
    {
        throw std::runtime_error("No latest word available to get data");
    }
    return latestWord->getDouble();
}

void* ForthDictionary::getDataAsPointer() const
{
    if (latestWord == nullptr)
    {
        throw std::runtime_error("No latest word available to get data");
    }
    return latestWord->getPointer();
}


// get and set type
ForthWordType ForthDictionary::getType() const
{
    return latestWord->type;
}

void ForthDictionary::setType(ForthWordType type) const
{
    latestWord->type = type;
}


// get pointer to data
void* ForthDictionary::get_data_ptr() const
{
    return &latestWord->data;
}


inline std::set<std::string> IndentSet;

std::string spaces(int n)
{
    return std::string(n, ' ');
}

std::string prettyPrintSourceCode(const std::string& source)
{
    size_t indent = 2;
    ForthTokenizer tokenizer(source);
    std::ostringstream oss;
    // Example usage
    while (true)
    {
        if (tokenizer.current() == ":")
        {
            tokenizer.next();
            indent += 2;
            oss << " " << tokenizer.current() << " ";
            tokenizer.next();
        }
        if (tokenizer.current() == "if" || tokenizer.current() == "do" ||
            tokenizer.current() == "begin"   )
        {
            oss << std::endl  << spaces(indent) << tokenizer.current();
            indent += 2;
            oss << std::endl << spaces(indent);
        } else if (tokenizer.current() == "then" || tokenizer.current() == "again"
            || tokenizer.current() == "repeat"  || tokenizer.current() == "loop"
            || tokenizer.current() == "+loop"   || tokenizer.current() == "recurse"
            || tokenizer.current() == ";"
            )
        {
            indent -= 2;
            oss << std::endl << spaces(indent) << tokenizer.current();
            oss << std::endl << spaces(indent);
        } else
        {
            oss << tokenizer.current() << " ";
        }
        if (!tokenizer.hasNext())
        {
            break;
        }
        tokenizer.next();
    }


    std::string result = oss.str();
    return result;
}


void ForthDictionary::displayWord(std::string name)
{
    std::cout << "Displaying word " << name << std::endl;
    auto* word = findWord(name.c_str());
    if (word == nullptr)
    {
        std::cout << "Word not found" << std::endl;
        return;
    }
    std::cout << "Name: " << word->name << std::endl;

    // Display function addresses in hex
    std::cout << "Compiled  : " << std::hex << reinterpret_cast<uintptr_t>(word->compiledFunc) << std::endl;
    std::cout << "Immediate : " << std::hex << reinterpret_cast<uintptr_t>(word->immediateFunc) << std::endl;
    std::cout << "Generator : " << std::hex << reinterpret_cast<uintptr_t>(word->generatorFunc) << std::endl;
    std::cout << "Interp    : " << std::hex << reinterpret_cast<uintptr_t>(word->terpFunc) << std::endl;
    std::cout << "State: " << ForthWordStateToString(word->state) << std::endl;
    std::cout << "Type: " << ForthWordTypeToString(word->type) << std::endl;

    if (std::holds_alternative<uint64_t>(word->data)) {
        std::cout << "Data contains uint64_t: " << std::get<uint64_t>(word->data) << std::endl;
    } else if (std::holds_alternative<double>(word->data)) {
        std::cout << "Data contains double: " << std::get<double>(word->data) << std::endl;
    } else if (std::holds_alternative<void*>(word->data)) {
        std::cout << "Data contains void*: " << std::get<void*>(word->data) << std::endl;
    } else {
        std::cerr << "Data contains unknown type" << std::endl;
    }
    std::cout << "Link: " << word->link << std::endl;

    // Check if the source code entry exists and is not empty
    auto it = sourceCodeMap.find(word->name);
    if (it != sourceCodeMap.end() && !it->second.empty())
    {
        std::cout << "Source Code:\n" << prettyPrintSourceCode(it->second) << std::endl;
    }
    else
    {
        std::cout << "Source Code: N/A" << std::endl << std::endl;
    }
}

// List all words in the dictionary
void ForthDictionary::list_words() const
{
    ForthWord* word = latestWord;
    while (word != nullptr)
    {
        std::cout << word->name << " ";
        word = word->link;
    }
    std::cout << std::endl;
}
