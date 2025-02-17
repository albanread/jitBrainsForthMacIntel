#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include "utility.h"
#include "StringInterner.h"
#include "JitContext.h"
#include "JitGenerator.h"
#include "tests.h"
#include "CompilerUtility.h"



// interpreter calls words, or pushes numbers.
inline void interpreter(const std::string& sourceCode)
{
    const auto words = splitAndLogWords(sourceCode);

    size_t i = 0;
    while (i < words.size())
    {
        const auto& word = words[i];
        if (logging) printf("Interpreter ... processing word: [%s]\n", word.c_str());

        if (word == ":")
        {
            handleCompileMode(i, words, sourceCode);
        }
        else
        {
            interpreterProcessWord(word, i, words);
        }
        ++i;
    }
}


inline bool startup_loaded = false;


// Function to interpret multiple statements and functions in the given text
inline void interpretText(const std::string& text)
{
    std::istringstream stream(text);
    std::string line;
    std::string accumulated_input;
    bool compiling = false;

    while (std::getline(stream, line))
    {
        if (line.empty())
        {
            continue;
        }

        accumulated_input += " " + line; // Accumulate input lines

        auto words = split(line);

        for (auto it = words.begin(); it != words.end(); ++it)
        {
            const auto& word = *it;

            if (word == ":")
            {
                compiling = true;
            }
            else if (word == ";")
            {
                compiling = false;
                //printf("Interpret text : [%s]\n", accumulated_input.c_str());
                interpreter(accumulated_input);
                accumulated_input.clear();
                break;
            }
        }

        if (!compiling)
        {
            interpreter(accumulated_input);
            accumulated_input.clear();
        }
    }

    // Ensure remaining accumulated input is processed after the loop
    if (!accumulated_input.empty())
    {
        interpreter(accumulated_input);
        accumulated_input.clear();
    }
}


// Function to load and interpret the start.f file
inline void slurpIn(const std::string& file_name = "./start.f")
{
    if (startup_loaded) return;
    startup_loaded = true;

    if (std::ifstream file(file_name); !file.is_open())
    {
        throw std::runtime_error("Could not open start.f file.");
    }

    try
    {
        std::ifstream file(file_name);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file.");
        }

        // Read the entire file content into a string
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();

        // Close the file
        file.close();
        interpretText(fileContent);
    }

    catch (const std::exception& e)
    {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        // Reset context and stack as required
    }
}

inline std::string handleSpecialCommands(const std::string& input)
{
    bool handled = false;

    if (input == "*MEM" || input == "*mem")
    {
        jc.reportMemoryUsage();
        handled = true;
    }
    else if (input == "*TESTS" || input == "*tests")
    {
        run_basic_tests();
        handled = true;
    }
    else if (input == "*STRINGS" || input == "*strings")
    {
        strIntern.display_list();
        handled = true;
    }
    else if (input == "*QUIT" || input == "*quit")
    {
        exit(0);
    }
    else if (input == "*LOGGINGON" || input == "*loggingon")
    {
        jc.loggingON();
        handled = true;
    }
    else if (input == "*LOGGINGOFF" || input == "*loggingoff")
    {
        jc.loggingOFF();
        handled = true;
    }

    if (handled)
    {
        // Remove the handled command from the input
        return "";
    }
    // If no command was handled, return the original input
    return input;
}

inline bool processTraceCommands(auto& it, const auto& words, std::string& accumulated_input)
{
    const auto& word = *it;
    if (word == "*TRON" || word == "*tron" || word == "*TROFF" || word == "*troff")
    {
        auto command = word;
        ++it; // Advance the iterator
        if (it != words.end())
        {
            const auto& nextWord = *it;
            if (command == "*TRON" || command == "*tron")
            {
                if (!nextWord.empty())
                    traceOn(nextWord);
            }
            else if (command == "*TROFF" || command == "*troff")
            {
                if (!nextWord.empty())
                    traceOff(nextWord);
            }
            // Remove `command` and `nextWord` from accumulated_input
            accumulated_input.erase(accumulated_input.find(command), command.length() + nextWord.length() + 2);
        }
        else
        {
            std::cerr << "Error: Expected name of word to trace after " << command << std::endl;
        }
        return true; // Processed trace command
    }
    return false; // Not a trace command
}

inline bool processLoopCheckCommands(auto& it, const auto& words, std::string& accumulated_input)
{
    const auto& word = *it;
    if (word == "*LOOPCHECK" || word == "*loopcheck")
    {
        // get next word
        ++it;
        if (it != words.end())
        {
            const auto& nextWord = *it;
            if (nextWord == "ON" || nextWord == "on")
            {
                // display loop checking on
                std::cout << "Loop checking ON" << std::endl;
                jc.loopCheckON();
            }
            else if (nextWord == "OFF" || nextWord == "off")
            {
                // display loop checking off
                std::cout << "Loop checking OFF" << std::endl;
                jc.loopCheckOFF();
            }
            else
            {
                std::cerr << "Error: Expected argument (on,off) after " << word << std::endl;
            }
            // Remove `command` and `nextWord` from accumulated_input
            accumulated_input.erase(accumulated_input.find(word), word.length() + nextWord.length() + 2);
        }
        return true; // Processed loop check command
    }
    return false; // Not a loop check command
}

inline bool processLoggingCommands(auto& it, const auto& words, std::string& accumulated_input)
{
    const auto& word = *it;
    if (word == "*LOGGING" || word == "*logging")
    {
        // get next word
        ++it;
        if (it != words.end())
        {
            const auto& nextWord = *it;
            if (nextWord == "ON" || nextWord == "on")
            {
                // display loop checking on
                std::cout << "logging ON" << std::endl;
                jc.loggingON();
            }
            else if (nextWord == "OFF" || nextWord == "off")
            {
                // display loop checking off
                std::cout << "logging OFF" << std::endl;
                jc.loggingOFF();
            }
            else
            {
                std::cerr << "Error: Expected argument (on,off) after " << word << std::endl;
            }
            // Remove `command` and `nextWord` from accumulated_input
            accumulated_input.erase(accumulated_input.find(word), word.length() + nextWord.length() + 2);
        }
        return true; // Processed loop check command
    }
    return false; // Not a loop check command
}



inline bool processDumpCommands(auto& it, const auto& words, std::string& accumulated_input)
{
    const auto& word = *it;
    if (word == "*dump" || word == "*DUMP")
    {
        // Get the next word
        ++it;
        if (it != words.end())
        {
            const auto& addrStr = *it;
            uintptr_t address;

            try
            {
                if (addrStr.find("0x") != std::string::npos || addrStr.find("0X") != std::string::npos)
                {
                    // Address is in hexadecimal
                    address = std::stoull(addrStr, nullptr, 16);
                }
                else
                {
                    // Address is in decimal
                    address = std::stoull(addrStr, nullptr, 10);
                }

                // Cast the address to a void pointer and call dump
                dump(reinterpret_cast<void*>(address));
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Error: Invalid address format" << std::endl;
            } catch (const std::out_of_range& e)
            {
                std::cerr << "Error: Address out of range" << std::endl;
            }

            // Remove `command` and `nextWord` from accumulated_input
            accumulated_input.erase(accumulated_input.find(word), word.length() + addrStr.length() + 2);
        }
        else
        {
            std::cerr << "Error: Expected address after " << word << std::endl;
        }
        return true; // Processed dump command
    }
    return false; // Not a dump command
}

inline void interactive_terminal()
{
    std::string input;
    std::string accumulated_input;
    bool compiling = false;
    slurpIn("./start.f");

    // The infinite terminal loop
    while (true)
    {
        std::cout << (compiling ? "] " : "> ");
        std::getline(std::cin, input); // Read a line of input from the terminal

        if (input == "QUIT" || input == "quit")
        {
            sm.resetDS();
            break; // Exit the loop if the user enters QUIT
        }

        input = handleSpecialCommands(input);

        if (input.empty())
        {
            continue;
        }

        accumulated_input += " " + input; // Accumulate input lines

        auto words = split(input);

        // Check if compiling is required based on the input
        for (auto it = words.begin(); it != words.end(); ++it)
        {
            const auto& word = *it;

            if (word == "QUIT" || word == "quit")
            {
                break;
            }

            if (processLoggingCommands(it, words, accumulated_input))
            {
                continue;
            }

            if (processTraceCommands(it, words, accumulated_input))
            {
                continue;
            }

            if (processLoopCheckCommands(it, words, accumulated_input))
            {
                continue;
            }

            if (processDumpCommands(it, words, accumulated_input))
            {
                continue;
            }

            if (word == ":")
            {
                compiling = true;
            }
            else if (word == ";")
            {
                compiling = false;
                interpreter(accumulated_input);
                accumulated_input.clear();
                break;
            }
        }

        if (!compiling)
        {
            interpreter(accumulated_input); // Process the accumulated input using the outer_interpreter
            accumulated_input.clear();
            std::cout << " Ok" << std::endl;
        }
    }
}


#endif //INTERPRETER_H
