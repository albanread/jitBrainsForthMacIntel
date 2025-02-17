#ifndef COMPILER_H
#define COMPILER_H
#include <sstream>
#include <string>
#include "ForthDictionary.h"
#include <unordered_set>
#include "StringInterner.h"
#include "JitContext.h"
#include "JitGenerator.h"
#include "CompilerUtility.h"


inline void compileWord(const std::string& wordName, const std::string& compileText, const std::string& sourceCode)
{
    logging = jc.logging;

    bool logging = tracedWords.find(wordName) != tracedWords.end();

    if (logging)
    {
        printf("\nCompiling word: [%s]\n", wordName.c_str());
    }

    JitContext& jc = JitContext::getInstance();
    jc.resetContext();

    // Start compiling the new word
    JitGenerator::genPrologue();

    const auto words = split(compileText);

    if (logging) printf("Split words: ");
    for (const auto& word : words)
    {
        if (logging) printf("%s ", word.c_str());
    }
    if (logging) printf("\n");

    size_t i = 0;
    while (i < words.size())
    {
        const auto& word = words[i];
        if (logging) printf("Compiler ... processing word: [%s]\n", word.c_str());

        auto* fword = d.findWord(word.c_str());

        if (fword)
        {
            if (fword->generatorFunc)
            {
                if (logging) printf("Generating code for word: %s\n", word.c_str());
                exec(fword->generatorFunc);
            }
            else if (fword->compiledFunc)
            {
                if (logging) printf("Generating call for compiled function of word: %s\n", word.c_str());
                // Assuming JitGenerator::genCall is the method to generate call for compiled function
                JitGenerator::genCall(fword->compiledFunc);
            }
            else if (fword->immediateFunc)
            {
                if (logging) printf("Running immediate function of word: %s\n", word.c_str());
                jc.pos_next_word = i;
                jc.pos_last_word = 0;
                jc.words = &words;
                exec(fword->immediateFunc);
                if (jc.pos_last_word != 0)
                {
                    i = jc.pos_last_word;
                }
            }
            else
            {
                if (logging) printf("Error: Unknown behavior for word: %s\n", word.c_str());
                jc.resetContext();
                return;
            }
        }
        else if (int o = JitGenerator::findLocal(word) != INVALID_OFFSET)
        {
            if (logging) printf(" local variable: %s at %d\n", word.c_str(), o);
            JitGenerator::genPushLocal(jc.offset);
        }
        else if (is_float(word))
        {
            try
            {
                double floatingNumber = parseFloat(word);
                jc.double_A = floatingNumber;
                JitGenerator::genPushDouble();
                if (logging) printf("Generated code for float: %s\n", word.c_str());
            }
            catch (const std::invalid_argument& e)
            {
                if (logging) std::cout << "Error: Invalid float: " << word << std::endl;
                jc.resetContext();
                return;
            }
            catch (const std::out_of_range& e)
            {
                if (logging) std::cout << "Error: Float out of range: " << word << std::endl;
                jc.resetContext();
                return;
            }
        }
        else if (is_number(word))
        {
            try
            {
                uint64_t number = parseNumber(word);
                jc.uint64_A = number;
                JitGenerator::genPushLong();
                if (logging) printf("Generated code for number: %s\n", word.c_str());
            }
            catch (const std::invalid_argument& e)
            {
                if (logging) std::cout << "Error: Invalid number: " << word << std::endl;
                jc.resetContext();
                return;
            }
            catch (const std::out_of_range& e)
            {
                if (logging) std::cout << "Error: Number out of range: " << word << std::endl;
                jc.resetContext();
                return;
            }
        }
        else
        {
            if (logging) std::cout << "Error: Unknown or uncompilable word: [" << word << "]" << std::endl;
            jc.resetContext();
            throw std::runtime_error("Compiler Error: Unknown or uncompilable word: " + word);
        }
        ++i;
    }

    // Check if the word already exists in the dictionary
    if (d.findWord(wordName.c_str()) != nullptr)
    {
        if (logging) printf("Compiler: word already exists: %s\n", wordName.c_str());
        jc.resetContext();
        return;
    }

    // Finalize compiled word
    JitGenerator::genEpilogue();
    const ForthFunction f = JitGenerator::endGeneration();
    d.addWord(wordName.c_str(),
              nullptr,
              f,
              nullptr,
              nullptr, sourceCode);


    if (logging)
    {
        printf("Compiler: successfully compiled word: %s\n", wordName.c_str());

        jc.reportMemoryUsage();
    }
}


#endif //COMPILER_H
