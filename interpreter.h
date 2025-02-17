#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include "utility.h"
#include "JitContext.h"
#include "JitGenerator.h"
#include "tests.h"
#include "CompilerUtility.h"
#include <termios.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>


#define MAX_INPUT 1024
#define MAX_WORD_LENGTH 16

inline bool debug_enabled = false;  // Debug flag (default: off)

// Function to enable or disable debug mode
inline void set_debug_mode(bool enable) {
    debug_enabled = enable;
}


struct termios t;
// Enable raw mode

inline void enable_raw_mode(struct termios *orig_termios) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, orig_termios);
    raw = *orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
    raw.c_cc[VMIN] = 1; // Min characters to read
    raw.c_cc[VTIME] = 0; // No timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    if (debug_enabled) printf("DEBUG: Raw mode enabled\n");
}

// Restore original terminal settings
void disable_raw_mode(struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
    if (debug_enabled) printf("DEBUG: Raw mode disabled\n");
}




#define MAX_HISTORY 50  // Maximum history size

std::vector<std::string> history; // Command history buffer
int history_index = -1;           // Index for navigating history

inline void read_input_c(char *buffer, size_t max_length) {
    size_t pos = 0;             // Current cursor position
    size_t length = 0;          // Length of the input string in `buffer`
    char c;
    std::string current_input;  // Keeps track of the current input for history navigation

    while (true) {
        if (read(STDIN_FILENO, &c, 1) != 1) break; // Read a single character

        if (debug_enabled) printf("DEBUG: Read character: %c (0x%02X)\n", c, c);

        // Handle Enter (complete the line input)
        if (c == '\n' || c == '\r') {
            write(STDOUT_FILENO, "\n", 1);
            buffer[length] = '\0'; // Null-terminate the input

            if (length > 0) {
                history.push_back(std::string(buffer)); // Save input to history
                if (history.size() > MAX_HISTORY) history.erase(history.begin()); // Limit history
            }

            history_index = -1; // Reset history navigation
            break;
        }

        // Handle Backspace
        if (c == 127 || c == 8) { // Backspace or Ctrl-H
            if (pos > 0) {
                pos--;
                length--;
                memmove(buffer + pos, buffer + pos + 1, length - pos); // Remove the char at `pos`
                buffer[length] = '\0';

                // Update screen
                write(STDOUT_FILENO, "\b", 1); // Move cursor back
                write(STDOUT_FILENO, &buffer[pos], length - pos); // Redraw the rest of the line
                write(STDOUT_FILENO, " ", 1); // Erase extra character on screen
                write(STDOUT_FILENO, "\b", 1); // Move cursor back one space
                for (size_t i = pos; i < length; i++) write(STDOUT_FILENO, "\b", 1); // Place cursor correctly
            }
            continue;
        }

        // Handle Arrow Keys
        if (c == 27) { // Escape sequence
            char seq[2];
            if (read(STDIN_FILENO, seq, 2) == 2) {
                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': // Up - Previous history
                            if (!history.empty() && history_index + 1 < (int)history.size()) {
                                if (history_index == -1) current_input = std::string(buffer, length); // Save current input
                                history_index++;
                                strncpy(buffer, history[history.size() - 1 - history_index].c_str(), max_length - 1);
                                length = strlen(buffer);
                                pos = length;
                                buffer[length] = '\0';
                                write(STDOUT_FILENO, "\33[2K\r", 4); // Clear line
                                write(STDOUT_FILENO, buffer, length); // Display history item
                            }
                            continue;
                        case 'B': // Down - Next history
                            if (history_index > 0) {
                                history_index--;
                                strncpy(buffer, history[history.size() - 1 - history_index].c_str(), max_length - 1);
                            } else if (history_index == 0) {
                                history_index = -1;
                                strncpy(buffer, current_input.c_str(), max_length - 1);
                            }
                            length = strlen(buffer);
                            pos = length;
                            buffer[length] = '\0';
                            write(STDOUT_FILENO, "\33[2K\r", 4); // Clear line
                            write(STDOUT_FILENO, buffer, length); // Display history item
                            continue;
                        case 'D': // Left - Move cursor left
                            if (pos > 0) {
                                write(STDOUT_FILENO, "\033[D", 3); // Move cursor left
                                pos--;
                            }
                            continue;
                        case 'C': // Right - Move cursor right
                            if (pos < length) {
                                write(STDOUT_FILENO, "\033[C", 3); // Move cursor right
                                pos++;
                            }
                            continue;
                    }
                }
            }
            continue;
        }

        if (c == 1) { // Ctrl+A - Move to start of line
            while (pos > 0) {
                write(STDOUT_FILENO, "\033[D", 3); // Move left
                pos--;
            }
            continue;
        }

        if (c == 5) { // Ctrl+E - Move to end of line
            while (pos < length) {
                write(STDOUT_FILENO, "\033[C", 3); // Move right
                pos++;
            }
            continue;
        }

        // Handle Normal Character Input
        if (length < max_length - 1) {
            // Shift the buffer to the right starting from `pos`
            memmove(buffer + pos + 1, buffer + pos, length - pos);
            buffer[pos] = c;   // Insert character at cursor
            length++;
            pos++;

            // Redraw updated input to the screen
            write(STDOUT_FILENO, &buffer[pos - 1], length - pos + 1); // Write rest of the string
            for (size_t i = pos; i < length; i++) write(STDOUT_FILENO, "\b", 1); // Move cursor back to correct position
        }
    }
}

// Wrapper function to replace std::getline with read_input_c
inline std::istream &custom_getline(std::istream &input, std::string &line) {
    constexpr size_t MAX_INPUT_LENGTH = 1024;
    char buffer[MAX_INPUT_LENGTH];

    if (!input.good()) return input; // Handle stream errors

    // Use your custom terminal input function
    read_input_c(buffer, MAX_INPUT_LENGTH);

    // Convert buffer to std::string
    line = std::string(buffer);

    return input;
}


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
// use when loading forth from a file etc.
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
               // printf("Interpret text : [%s]\n", accumulated_input.c_str());
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
    struct termios orig_termios;
    enable_raw_mode(&orig_termios);
    std::string input;
    std::string accumulated_input;
    bool compiling = false;

    slurpIn("./start.f");

    // The infinite terminal loop
    while (true)
    {
        std::cout << (compiling ? "] " : "> ");
        std::cout.flush();
        custom_getline(std::cin, input); // Read a line of input from the terminal


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
            std::cout.flush();
        }
    }
    disable_raw_mode(&orig_termios);
}


#endif //INTERPRETER_H
