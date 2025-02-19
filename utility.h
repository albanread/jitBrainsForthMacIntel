#ifndef UTILITY_H
#define UTILITY_H
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cctype>
#include <bitset>
#include <iomanip>
#include <cstdint>

extern "C" {
inline void putchars(const char *s) {
    fputs(s, stdout);
}


inline void printDecimal(int64_t number) {
    //printf("number is %ld", number);
    std::cout << std::to_string(number);
}

inline void printUnsignedDecimal(uint64_t number) {
    std::cout << std::to_string(number);
}

inline void printUnsignedHex(uint64_t number) {
    std::ostringstream oss;
    oss << "0x" << std::hex << number;
    std::cout << oss.str() << std::endl;
}

inline void printHex(int64_t number) {
    std::ostringstream oss;
    oss << "0x" << std::hex << number;
    std::cout << oss.str() << std::endl;
}

inline void printBinary(int64_t number) {
    std::cout << "0b" << std::bitset<64>(number);
}

inline void printUnsignedBinary(uint64_t number) {
    std::cout << "0b" << std::bitset<64>(number);
}
}

inline std::string trim(const std::string &str) {
    const size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos)
        return ""; // no content

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

inline bool is_float(const std::string &s) {
    if (s.empty()) return false;
    size_t startIndex = 0;
    bool hasDecimalPoint = false;

    // Check for an optional leading minus
    if (s[0] == '-') {
        if (s.length() == 1) return false; // only '-' is not a valid number
        startIndex = 1;
    }

    // Check each character in the string
    for (size_t i = startIndex; i < s.length(); ++i) {
        if (s[i] == '.') {
            if (hasDecimalPoint) return false; // More than one decimal point is not valid
            hasDecimalPoint = true;
        } else if (!std::isdigit(s[i])) {
            return false; // Non-digit characters other than a single decimal point are not valid
        }
    }

    // Ensure there's at least one digit if it has a leading minus
    return hasDecimalPoint && s.length() > startIndex + 1;
}


inline bool is_number(const std::string &s) {
    if (s.empty()) return false;
    size_t startIndex = 0;

    // Check for an optional leading minus
    if (s[0] == '-') {
        if (s.length() == 1) return false; // only '-' is not a valid number
        startIndex = 1;
    }

    if (startIndex + 2 < s.length() && s[startIndex] == '0') {
        if (s[startIndex + 1] == 'x' || s[startIndex + 1] == 'X') // Hexadecimal check
        {
            for (size_t i = startIndex + 2; i < s.length(); ++i) {
                if (!std::isxdigit(s[i])) return false;
            }
            return true;
        } else if (s[startIndex + 1] == 'b' || s[startIndex + 1] == 'B') // Binary check
        {
            for (size_t i = startIndex + 2; i < s.length(); ++i) {
                if (s[i] != '0' && s[i] != '1') return false;
            }
            return true;
        }
    }

    // Decimal check
    for (size_t i = startIndex; i < s.length(); ++i) {
        if (!std::isdigit(s[i])) return false;
    }

    return true;
}

inline std::vector<std::string> split(const std::string &str) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    for (std::string word; iss >> word;)
        result.push_back(word);
    return result;
}

inline std::string to_lower(const std::string &s) {
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

inline void dump(const void *address) {
    const unsigned char *addr = static_cast<const unsigned char *>(address);
    size_t length = 32;

    for (size_t i = 0; i < length; i += 16) {
        // Print the memory address
        std::cout << std::setw(8) << std::setfill('0') << std::hex << reinterpret_cast<uintptr_t>(addr + i) << ": ";

        // Print the hexadecimal representation
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < length) {
                std::cout << std::setw(2) << static_cast<int>(addr[i + j]) << " ";
            } else {
                std::cout << "   ";
            }
        }

        // Print the ASCII representation
        std::cout << " ";
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < length) {
                char ch = static_cast<char>(addr[i + j]);
                if (std::isprint(ch)) {
                    std::cout << ch;
                } else {
                    std::cout << ".";
                }
            }
        }

        std::cout << std::endl;
    }
}


inline void printFloat(double number) {
    std::cout << number;
}


#endif //UTILITY_H
