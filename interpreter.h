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

inline bool debug_enabled = false; // Debug flag (default: off)

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
int history_index = -1; // Index for navigating history

inline void read_input_c(char *buffer, size_t max_length) {
    size_t pos = 0; // Current cursor position
    size_t length = 0; // Length of the input string in `buffer`
    char c;
    std::string current_input; // Keeps track of the current input for history navigation

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
        if (c == 127 || c == 8) {
            // Backspace or Ctrl-H
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
        if (c == 27) {
            // Escape sequence
            char seq[2];
            if (read(STDIN_FILENO, seq, 2) == 2) {
                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': // Up - Previous history
                            if (!history.empty() && history_index + 1 < (int) history.size()) {
                                if (history_index == -1) current_input = std::string(buffer, length);
                                // Save current input
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

        if (c == 1) {
            // Ctrl+A - Move to start of line
            while (pos > 0) {
                write(STDOUT_FILENO, "\033[D", 3); // Move left
                pos--;
            }
            continue;
        }

        if (c == 5) {
            // Ctrl+E - Move to end of line
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
            buffer[pos] = c; // Insert character at cursor
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

void skip_whitespace(const char **input) {
    while (**input && isspace(**input)) {
        (*input)++;
    }
}


int is_float(const char *str) {
    int has_dot = 0, has_exp = 0;
    int seen_digit_before_exp = 0, seen_digit_after_exp = 0;

    // Handle optional leading sign
    if (*str == '-' || *str == '+') {
        str++;
    }

    while (*str) {
        if (*str == '.') {
            if (has_dot || has_exp) {
                return 0; // Invalid: more than one dot or dot after exponent
            }
            has_dot = 1;
        } else if (*str == 'e' || *str == 'E') {
            if (has_exp || !seen_digit_before_exp) {
                return 0; // Invalid: more than one exponent or no digits before exponent
            }
            has_exp = 1;
        } else if (*str == '-' || *str == '+') {
            if (*(str - 1) != 'e' && *(str - 1) != 'E') {
                return 0; // Invalid: sign not immediately after exponent
            }
        } else if (isdigit(*str)) {
            if (has_exp) {
                seen_digit_after_exp = 1; // Keeping track of digits after 'e/E'
            } else {
                seen_digit_before_exp = 1; // Keeping track of digits before 'e/E'
            }
        } else {
            return 0; // Invalid character
        }
        str++;
    }
    return (has_dot || has_exp) && seen_digit_before_exp && (!has_exp || seen_digit_after_exp);
}

Token get_next_token(const char **input) {
    Token token = {TOKEN_UNKNOWN, {0}};
    skip_whitespace(input);

    if (**input == '\0') {
        token.type = TOKEN_END;
        return token;
    }

    int i = 0;
    char temp[MAX_TOKEN_LENGTH] = {0};
    while (**input && !isspace(**input) && i < MAX_TOKEN_LENGTH - 1) {
        temp[i++] = *(*input)++;
    }
    temp[i] = '\0';

    // Check for compilation/interpreting tokens
    if (strcmp(temp, ":") == 0 || strcmp(temp, "]") == 0) {
        token.type = TOKEN_COMPILING;
    } else if (strcmp(temp, ";") == 0 || strcmp(temp, "[") == 0) {
        token.type = TOKEN_INTERPRETING;
    }
    // Handle words ending in quotes
    else if (temp[i - 1] == '"') {
        temp[i] = '\0'; // Remove trailing " from word
        token.type = TOKEN_WORD;
        strncpy(token.value, temp, MAX_TOKEN_LENGTH - 1);
        (*input)--; // Backtrack so the " starts a new string literal
    }
    // Detect floating numbers
    else if (is_float(temp)) {
        token.type = TOKEN_FLOAT;
        token.float_value = atof(temp);
    }
    // Detect integer numbers
    else if (is_number(temp)) {
        token.type = TOKEN_NUMBER;
        token.int_value = parseNumber(temp);
    }
    // Handle regular words
    else {
        token.type = TOKEN_WORD;
        strncpy(token.value, temp, MAX_TOKEN_LENGTH - 1);
    }
    return token;
}


Token get_string_token(const char **input) {
    Token token = {TOKEN_STRING, {0}};
    (*input)++; // Consume opening "
    if (**input == ' ') {
        (*input)++; // Consume that one space
    }
    int i = 0;
    while (**input && **input != '"' && i < MAX_TOKEN_LENGTH - 1) {
        token.value[i++] = *(*input)++;
    }
    if (**input == '"') {
        (*input)++; // Consume closing quote
    }
    token.value[i] = '\0';
    return token;
}

inline int tokenize_forth(const char *input, Token tokens[MAX_TOKENS]) {
    const char *cursor = input;
    int token_count = 0;
    Token token;

    // reset all tokens
    for (int i = 0; i < MAX_TOKENS; i++) {
        tokens[i].type = TOKEN_END;
        tokens[i].value[0] = '\0';
        tokens[i].int_value = 0;
        tokens[i].float_value = 0;
    }

    while ((token = get_next_token(&cursor)).type != TOKEN_END && token_count < MAX_TOKENS) {
        if (token.type == TOKEN_WORD && *cursor == '"') {
            tokens[token_count++] = token;
            token = get_string_token(&cursor);
        }
        tokens[token_count++] = token;
    }

    tokens[token_count].type = TOKEN_END; // Mark end of tokens
    return token_count;
}

 inline void handleCompilerTokenizedWord(int &index, Token (*tokens)[MAX_TOKENS]) {

    // on entry we should have TOKEN_COMPILING at index
    if ( (*tokens)[index].type != TOKEN_COMPILING) {
        throw std::runtime_error("Compiler Error: invalid token: " + std::to_string((*tokens)[index].type));
    }

    // skip to new word name
    index++;
    std::string wordName = (*tokens)[index].value;
    logging = jc.logging; // Use a single consistent logging variable
    printf("\nCompiling word: [%s]\n", wordName.c_str());

    // Prevent recompiling an existing word
    if (d.findWord(wordName.c_str()) != nullptr) {
        if (logging) printf("Compiler: word already exists: %s\n", wordName.c_str());
        jc.resetContext();
        throw std::runtime_error("Compiler Error: word already exists: " + wordName);
    }

    // Check if this word is being traced
    bool wordLogging = (tracedWords.find(wordName) != tracedWords.end());
    if (wordLogging) {
        printf("\nCompiling word: [%s] with tracing enabled.\n", wordName.c_str());
    }

    // Get singleton JIT context and reset it for new word compilation
    JitContext &jc = JitContext::getInstance();
    jc.resetContext();

    // Start compiling the new word
    JitGenerator::genPrologue();

    // Process tokens until end or exit condition (TOKEN_END or TOKEN_COMPILING)
    index++;

    while (index < MAX_TOKENS) {
        auto &token = (*tokens)[index];
        auto type = token.type;

        if (type == TOKEN_COMPILING || type == TOKEN_END) {
            break;
        }

        if (logging) {
            printf("Processing token at index %d: [%s] (type=%d)\n", index, token.value, type);
        }

        switch (type) {
            case TOKEN_WORD: {
                std::string word = token.value;

                if (logging) printf("Processing WORD: %s\n", word.c_str());

                auto *fword = d.findWord(word.c_str());


                if (fword) {
                    // Handle word types based on functions defined within the word
                    if (fword->generatorFunc) {
                        if (logging) printf("Generating code for word: %s\n", word.c_str());
                        exec(fword->generatorFunc);
                    } else if (fword->compiledFunc) {
                        if (logging) printf("Generating call for compiled function of word: %s\n", word.c_str());
                        JitGenerator::genCall(fword->compiledFunc);
                    } else if (fword->immediateFunc) {
                        if (logging) printf("Running immediate function of word: %s\n", word.c_str());

                        // Handle immediate functions
                        jc.pos_next_word = index;
                        jc.pos_last_word = 0;
                        jc.next_token = (*(tokens))[index + 1];
                        exec(fword->immediateFunc);

                        // Handle modified index from immediate function
                        if (jc.pos_last_word != 0) {
                            index = jc.pos_last_word;
                        }
                    } else {
                        if (logging) printf("Error: Unknown behavior for word: %s\n", word.c_str());
                        jc.resetContext();
                        throw std::runtime_error("Unknown word behavior: " + word);
                    }
                } else {
                    // Handle case where word is a local variable
                    int o = JitGenerator::findLocal(word);
                    if (o != INVALID_OFFSET) {
                        if (logging) printf("Local variable: %s at offset %d\n", word.c_str(), o);
                        JitGenerator::genPushLocal(o);
                    } else {
                        if (logging) printf("Error: Unknown or uncompilable word: %s\n", word.c_str());
                        jc.resetContext();
                        throw std::runtime_error("Unknown word: " + word);
                    }
                }
                break;
            }
            case TOKEN_NUMBER: {
                uint64_t number = token.int_value;
                jc.uint64_A = number;
                JitGenerator::genPushLong();
                if (logging) printf("Generated code for number: %d\n", token.int_value);
                break;
            }
            case TOKEN_FLOAT: {
                jc.double_A = token.float_value;
                JitGenerator::genPushDouble();
                if (logging) printf("Generated code for float: %f\n", token.float_value);
                break;
            }
            case TOKEN_STRING:
                // Assume strings have been handled by immediate functions
                break;
            case TOKEN_UNKNOWN: {
                if (logging) printf("Processing UNKNOWN token.\n");
                jc.resetContext();
                throw std::runtime_error("Unknown token encountered: " + std::string(token.value));
            }
            case TOKEN_INTERPRETING:
            case TOKEN_COMPILING:
                // Nothing specific needed here, break out of loop if encountered
                if (logging) printf("Unexpected COMPILING/INTERPRETING token: [%s]\n", token.value);
                break;
            default:
                if (logging) printf("Unhandled token type: [%d]\n", type);
                break;
        }

        // Increment index for the next token
        index++;
    }

    // Finalize compiled word
    JitGenerator::genEpilogue();
    const ForthFunction f = JitGenerator::endGeneration();

    d.addWord(wordName.c_str(), nullptr, f, nullptr, nullptr, "");

    if (logging || wordLogging) {
        printf("Compiler: Successfully compiled word: %s\n", wordName.c_str());
        jc.reportMemoryUsage();
    }
}


inline void interpreterProcessWordTokenized(const Token &token, int &index, Token (*tokens)[MAX_TOKENS]) {
    ForthWord *fword = nullptr;

    switch (token.type) {
        case TOKEN_WORD:
            // Handle word tokens, `value` is used for string/word storage
            if (debug_enabled) printf("Processing WORD: %s\n", token.value);
            fword = d.findWord(token.value);
            if (fword) {
                if (fword->compiledFunc) {
                    if (debug_enabled) printf("Calling word: %s\n", token.value);
                    exec(fword->compiledFunc);
                } else if (fword->terpFunc) // immediate word
                {
                    if (debug_enabled) printf("Running interpreter immediate word: %s\n", token.value);
                    jc.pos_next_word = index;
                    jc.next_token = (*(tokens))[index + 1];
                    exec(fword->terpFunc);
                } else {
                    if (debug_enabled)
                        std::cout << "Error: Word [" << token.value <<
                                "] found but cannot be executed.\n";
                    d.displayWord(token.value);
                    throw std::runtime_error("Cannot execute word");
                }
            } else {
                if (debug_enabled)
                    std::cout << "Error: Unknown or uncompilable word: [" << token.value << "]" <<
                            std::endl;
                throw std::runtime_error("Unknown word: " + std::string(token.value));
            }
            break;

        case TOKEN_COMPILING:
            printf("Processing COMPILING: %s\n", token.value);
            handleCompilerTokenizedWord(index, tokens);


        case TOKEN_NUMBER:
            // Handle integers
            //printf("Processing NUMBER: %d\n", token.int_value);
            // Perform actions with the integer value
            sm.pushDS(token.int_value);
            break;

        case TOKEN_FLOAT:
            // Handle floating-point numbers
            //printf("Processing FLOAT: %f\n", token.float_value);
            sm.pushDSDouble(token.float_value);
            break;

        case TOKEN_STRING:
            // Handle string literal tokens
            if (debug_enabled) printf("Processing STRING: \"%s\"\n", token.value);

            break;

        case TOKEN_UNKNOWN:
            // Handle unknown tokens if necessary
            if (debug_enabled) printf("Processing UNKNOWN token.\n");
            if (logging) std::cout << "Error: Unknown or uncompilable word: [" << token.value << "]" << std::endl;
            throw std::runtime_error("Unknown word: " + std::string(token.value));
            break;

        case TOKEN_END:
            // Encounter end of tokens; typically no action needed here
            if (debug_enabled) printf("End of tokens.\n");
            break;

        default:
            if (debug_enabled) printf("Invalid token type encountered.\n");
            break;
    }
}


inline void interpreter(const std::string &sourceCode) {
    int count = tokenize_forth(sourceCode.c_str(), tokens);
    // print_token_list(tokens, count);
    int i = 0;
    while (i < count) {
        // Pass the token, index, and full token array to the processing function
        interpreterProcessWordTokenized(tokens[i], i, &tokens);
        i++;
    }
}


inline bool startup_loaded = false;


// Function to interpret multiple statements and functions in the given text
// use when loading forth from a file etc.
inline void interpretText(const std::string &text) {
    std::istringstream stream(text);
    std::string line;
    std::string accumulated_input;
    bool compiling = false;

    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        accumulated_input += " " + line; // Accumulate input lines

        auto words = split(line);

        for (auto it = words.begin(); it != words.end(); ++it) {
            const auto &word = *it;

            if (word == ":") {
                compiling = true;
            } else if (word == ";") {
                compiling = false;
                // printf("Interpret text : [%s]\n", accumulated_input.c_str());
                interpreter(accumulated_input);
                accumulated_input.clear();
                break;
            }
        }

        if (!compiling) {
            interpreter(accumulated_input);
            accumulated_input.clear();
        }
    }

    // Ensure remaining accumulated input is processed after the loop
    if (!accumulated_input.empty()) {
        interpreter(accumulated_input);
        accumulated_input.clear();
    }
}


// Function to load and interpret the start.f file
inline void slurpIn(const std::string &file_name = "./start.f") {
    if (startup_loaded) return;
    startup_loaded = true;

    if (std::ifstream file(file_name); !file.is_open()) {
        throw std::runtime_error("Could not open start.f file.");
    }

    try {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file.");
        }

        // Read the entire file content into a string
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();

        // Close the file
        file.close();

        interpretText(fileContent);
    } catch (const std::exception &e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        // Reset context and stack as required
    }
}

inline std::string handleSpecialCommands(const std::string &input) {
    bool handled = false;

    if (input == "*MEM" || input == "*mem") {
        jc.reportMemoryUsage();
        handled = true;
    } else if (input == "*TESTS" || input == "*tests") {
        run_basic_tests();
        handled = true;
    } else if (input == "*STRINGS" || input == "*strings") {
        handled = true;
    } else if (input == "*QUIT" || input == "*quit") {
        exit(0);
    } else if (input == "*LOGGINGON" || input == "*loggingon") {
        jc.loggingON();
        handled = true;
    } else if (input == "*LOGGINGOFF" || input == "*loggingoff") {
        jc.loggingOFF();
        handled = true;
    }

    if (handled) {
        // Remove the handled command from the input
        return "";
    }
    // If no command was handled, return the original input
    return input;
}

inline bool processTraceCommands(auto &it, const auto &words, std::string &accumulated_input) {
    const auto &word = *it;
    if (word == "*TRON" || word == "*tron" || word == "*TROFF" || word == "*troff") {
        auto command = word;
        ++it; // Advance the iterator
        if (it != words.end()) {
            const auto &nextWord = *it;
            if (command == "*TRON" || command == "*tron") {
                if (!nextWord.empty())
                    traceOn(nextWord);
            } else if (command == "*TROFF" || command == "*troff") {
                if (!nextWord.empty())
                    traceOff(nextWord);
            }
            // Remove `command` and `nextWord` from accumulated_input
            accumulated_input.erase(accumulated_input.find(command), command.length() + nextWord.length() + 2);
        } else {
            std::cerr << "Error: Expected name of word to trace after " << command << std::endl;
        }
        return true; // Processed trace command
    }
    return false; // Not a trace command
}

inline bool processLoopCheckCommands(auto &it, const auto &words, std::string &accumulated_input) {
    const auto &word = *it;
    if (word == "*LOOPCHECK" || word == "*loopcheck") {
        // get next word
        ++it;
        if (it != words.end()) {
            const auto &nextWord = *it;
            if (nextWord == "ON" || nextWord == "on") {
                // display loop checking on
                std::cout << "Loop checking ON" << std::endl;
                jc.loopCheckON();
            } else if (nextWord == "OFF" || nextWord == "off") {
                // display loop checking off
                std::cout << "Loop checking OFF" << std::endl;
                jc.loopCheckOFF();
            } else {
                std::cerr << "Error: Expected argument (on,off) after " << word << std::endl;
            }
            // Remove `command` and `nextWord` from accumulated_input
            accumulated_input.erase(accumulated_input.find(word), word.length() + nextWord.length() + 2);
        }
        return true; // Processed loop check command
    }
    return false; // Not a loop check command
}

inline bool processLoggingCommands(auto &it, const auto &words, std::string &accumulated_input) {
    const auto &word = *it;
    if (word == "*LOGGING" || word == "*logging") {
        // get next word
        ++it;
        if (it != words.end()) {
            const auto &nextWord = *it;
            if (nextWord == "ON" || nextWord == "on") {
                // display loop checking on
                std::cout << "logging ON" << std::endl;
                jc.loggingON();
            } else if (nextWord == "OFF" || nextWord == "off") {
                // display loop checking off
                std::cout << "logging OFF" << std::endl;
                jc.loggingOFF();
            } else {
                std::cerr << "Error: Expected argument (on,off) after " << word << std::endl;
            }
            // Remove `command` and `nextWord` from accumulated_input
            accumulated_input.erase(accumulated_input.find(word), word.length() + nextWord.length() + 2);
        }
        return true; // Processed loop check command
    }
    return false; // Not a loop check command
}


inline bool processDumpCommands(auto &it, const auto &words, std::string &accumulated_input) {
    const auto &word = *it;
    if (word == "*dump" || word == "*DUMP") {
        // Get the next word
        ++it;
        if (it != words.end()) {
            const auto &addrStr = *it;
            uintptr_t address;

            try {
                if (addrStr.find("0x") != std::string::npos || addrStr.find("0X") != std::string::npos) {
                    // Address is in hexadecimal
                    address = std::stoull(addrStr, nullptr, 16);
                } else {
                    // Address is in decimal
                    address = std::stoull(addrStr, nullptr, 10);
                }

                // Cast the address to a void pointer and call dump
                dump(reinterpret_cast<void *>(address));
            } catch (const std::invalid_argument &e) {
                std::cerr << "Error: Invalid address format" << std::endl;
            } catch (const std::out_of_range &e) {
                std::cerr << "Error: Address out of range" << std::endl;
            }

            // Remove `command` and `nextWord` from accumulated_input
            accumulated_input.erase(accumulated_input.find(word), word.length() + addrStr.length() + 2);
        } else {
            std::cerr << "Error: Expected address after " << word << std::endl;
        }
        return true; // Processed dump command
    }
    return false; // Not a dump command
}

inline void interactive_terminal() {
    struct termios orig_termios;
    enable_raw_mode(&orig_termios);
    std::string input;
    std::string accumulated_input;
    bool compiling = false;

    //slurpIn("./start.f");

    // The infinite terminal loop
    while (true) {
        std::cout << (compiling ? "] " : "> ");
        std::cout.flush();
        custom_getline(std::cin, input); // Read a line of input from the terminal


        if (input == "QUIT" || input == "quit") {
            sm.resetDS();
            break; // Exit the loop if the user enters QUIT
        }

        input = handleSpecialCommands(input);

        if (input.empty()) {
            continue;
        }

        accumulated_input += " " + input; // Accumulate input lines

        auto words = split(input);

        // Check if compiling is required based on the input
        for (auto it = words.begin(); it != words.end(); ++it) {
            const auto &word = *it;

            if (word == "QUIT" || word == "quit") {
                break;
            }

            if (processLoggingCommands(it, words, accumulated_input)) {
                continue;
            }

            if (processTraceCommands(it, words, accumulated_input)) {
                continue;
            }

            if (processLoopCheckCommands(it, words, accumulated_input)) {
                continue;
            }

            if (processDumpCommands(it, words, accumulated_input)) {
                continue;
            }

            if (word == ":") {
                compiling = true;
            } else if (word == ";") {
                compiling = false;
                interpreter(accumulated_input);
                accumulated_input.clear();
                break;
            }
        }

        if (!compiling) {
            interpreter(accumulated_input); // Process the accumulated input using the outer_interpreter
            accumulated_input.clear();
            std::cout << " Ok" << std::endl;
            std::cout.flush();
        }
    }
    disable_raw_mode(&orig_termios);
}


#endif //INTERPRETER_H
