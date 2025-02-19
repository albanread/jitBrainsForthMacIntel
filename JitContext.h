#ifndef JITCONTEXT_H
#define JITCONTEXT_H

#include <iostream>
#include "asmjit/asmjit.h"
#include <string>
#include <vector>

#define MAX_INPUT 1024
#define MAX_WORD_LENGTH 16
#define MAX_TOKEN_LENGTH 1024
#define MAX_TOKENS 1024


typedef enum {
    TOKEN_WORD,
    TOKEN_NUMBER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_UNKNOWN,
    TOKEN_COMPILING,
    TOKEN_INTERPRETING,
    TOKEN_END
} TokenType;

typedef struct {
    TokenType type;

    union {
        int int_value;
        double float_value;
        char value[1024];
    };
} Token;

inline Token tokens[MAX_TOKENS];


inline void print_token(const Token *token) {
    switch (token->type) {
        case TOKEN_WORD:
            printf("WORD: %s\n", token->value);
            break;
        case TOKEN_NUMBER:
            printf("NUMBER: %d\n", token->int_value);
            break;
        case TOKEN_FLOAT:
            printf("FLOAT: %f\n", token->float_value);
            break;
        case TOKEN_STRING:
            printf("STRING: \"%s\"\n", token->value);
            break;
        case TOKEN_COMPILING:
            printf("COMPILING\n");
            break;
        case TOKEN_INTERPRETING:
            printf("INTERPRETING\n");
            break;

        default:
            printf("UNKNOWN\n");
            break;
    }
}

inline void print_token_list(const Token tokens[MAX_TOKENS]) {
  int i = 0;
  while (tokens[i].type != TOKEN_END && i < MAX_TOKENS) {
    print_token(&tokens[i]);
    i++;
  }
}


static bool logging = true;

class JitContext {
public:
    // Static method to get the singleton instance
    static JitContext &getInstance() {
        static JitContext instance;
        return instance;
    }

    // Delete copy constructor and assignment operator to prevent copies
    JitContext(const JitContext &) = delete;

    JitContext &operator=(const JitContext &) = delete;

    // Method to get the assembler reference
    [[nodiscard]] asmjit::x86::Assembler &getAssembler() const {
        return *assembler;
    }

    void resetContext() {
        if (auto_reset) {
            // Reset and reinitialize the code holder
            code.reset();
            code.init(rt.environment());

            asmjit::Section *dataSection;
            code.newSection(&dataSection, ".data", SIZE_MAX, asmjit::SectionFlags::kNone, 8);
            // Attach the assembler to the CodeHolder
            assembler = new asmjit::x86::Assembler(&code);

            //
            // // Recreate the assembler with the new code holder
            // //delete assembler;
            assembler = new asmjit::x86::Assembler(&code);
            if (logging) {
                code.setLogger(&logger);
                logger.addFlags(asmjit::FormatFlags::kMachineCode);
            }
        }
    }


    void reportMemoryUsage() const {
        auto sectionCount = code.sectionCount(); // Get the number of sections
        for (size_t i = 0; i < sectionCount; ++i) {
            const asmjit::Section *section = code.sectionById(i);
            if (!section) continue; // Safety check, should not be null

            const asmjit::CodeBuffer &buffer = section->buffer();
            std::cout << "Section " << i << ": " << section->name() << std::endl;
            std::cout << "  Buffer size    : " << buffer.size() << " bytes" << std::endl;
            std::cout << "  Buffer capacity: " << buffer.capacity() << " bytes" << std::endl;

            // Descriptions for known sections (you can expand this if you use more sections)
            switch (i) {
                case 0:
                    std::cout << "  Description    : Primary code section (default)" << std::endl;
                    break;
                case 1:
                    std::cout << "  Description    : Secondary section (if used)" << std::endl;
                    break;
                default:
                    std::cout << "  Description    : Additional section" << std::endl;
                    break;
            }
        }
    }

    // Example method
    static void someJitFunction() {
        // Implementation of a method
        std::cout << "Executing some JIT function..." << std::endl;
    }

    void loggingON() {
        logging = true;
        code.setLogger(&logger);
    }

    void loggingOFF() {
        logging = false;
        code.setLogger(nullptr); // Disable logging
    }

    void resetON() {
        auto_reset = true;
    }

    void resetOFF() {
        auto_reset = false;
    }

    void loopCheckON() {
        optLoopCheck = true;
    }

    void loopCheckOFF() {
        optLoopCheck = false;
    }

    void overflowCheckON() {
        optOverflowCheck = true;
    }

    void overflowCheckOFF() {
        optOverflowCheck = false;
    }

private:
    // Private constructor to prevent instantiation
    JitContext() : rt(),
                   assembler(nullptr),

                   logger(stdout),
                   uint64_A(0),
                   uint64_B(0),
                   uint32_A(0),
                   uint32_B(0),
                   uint16(0),
                   uint8(0),
                   int64_A(0),
                   int64_B(0),
                   int32(0),
                   int16(0),
                   int8(0),
                   offset(0),
                   f(0.0f),
                   d(0.0),
                   ptr_A(nullptr),
                   ptr_B(nullptr),
                   logging(false),
                   pos_next_word(0),
                   pos_last_word(0),
                   words(nullptr),
                   word(),
                   next_token()
{
        // Initialization code
        code.reset();
        code.init(rt.environment());
        assembler = new asmjit::x86::Assembler(&code);
        if (logging) {
            code.setLogger(&logger);
        }
    }

    // Private destructor
    ~JitContext() {
        // Cleanup code
        delete assembler;
    }

public:
    asmjit::FileLogger logger; // Logs to the standard output
    asmjit::JitRuntime rt;
    asmjit::CodeHolder code;
    asmjit::x86::Assembler *assembler;
    asmjit::Label epilogueLabel;

    // Used to pass arguments to the code generators
    uint64_t uint64_A;
    uint64_t uint64_B;
    uint32_t uint32_A;
    uint32_t uint32_B;
    uint16_t uint16;
    uint8_t uint8;
    int64_t int64_A;
    int64_t int64_B;
    int32_t int32;
    int16_t int16;
    int8_t int8;
    int offset;
    float f;
    double d;
    void *ptr_A;
    void *ptr_B;
    bool logging = false;
    bool auto_reset = true;
    // these are for immediate words that read the input stream
    size_t pos_next_word;
    size_t pos_last_word;
    const std::vector<std::string> *words;;
    std::string word;
    // these are compiler options

    bool optLoopCheck = false;
    bool optOverflowCheck = false;
    double double_A;

    // next token in stream
    Token next_token;
};

#endif // JITCONTEXT_H
