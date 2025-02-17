#ifndef JITGENERATOR_H
#define JITGENERATOR_H

#include <functional>
#include <iostream>
#include <stdexcept>
#include "asmjit/asmjit.h"
#include "JitContext.h"
#include "ForthDictionary.h"
#include <stack>
#include "StackManager.h"
#include <variant>
#include "StringInterner.h"
#include "quit.h"
#include <cmath>
#include "jitLabels.h"

const int INVALID_OFFSET = -9999;
static const double EPSILON = 1e-9; // Epsilon for floating-point comparison
static const uint64_t maskAbs = 0x7FFFFFFFFFFFFFFF;


// support locals
static int arguments_to_local_count;
static int locals_count;
static int returned_arguments_count;

struct VariableInfo {
    std::string name;
    int offset;
};

static std::unordered_map<std::string, VariableInfo> arguments;
static std::unordered_map<std::string, VariableInfo> locals;
static std::unordered_map<std::string, VariableInfo> returnValues;
static std::unordered_map<int, std::string> argumentsByOffset;
static std::unordered_map<int, std::string> localsByOffset;
static std::unordered_map<int, std::string> returnValuesByOffset;


inline JitContext &jc = JitContext::getInstance();
inline ForthDictionary &d = ForthDictionary::getInstance();
inline StackManager &sm = StackManager::getInstance();

inline extern void prim_emit(const uint64_t a) {
    char c = static_cast<char>(a & 0xFF);
    std::cout << c;
}

inline extern void prints(const char *str) {
    std::cout << str;
}

inline extern void throw_with_string(const char *str) {
    throw std::runtime_error(str);
}


// using ExecFunc = void (*)(ForthFunction);
// ExecFunc exec;

class JitGenerator {
public:
    void accessStackManager(StackManager &sm);


    // Static method to get the singleton instance
    static JitGenerator &getInstance() {
        static JitGenerator instance;
        return instance;
    }


    // Delete copy constructor and assignment operator to prevent copies
    JitGenerator(const JitGenerator &) = delete;

    JitGenerator &operator=(const JitGenerator &) = delete;


    static void commentWithWord(const std::string &baseComment) {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        const std::string comment = baseComment + " [" + jc.word + "]";
        a.comment(comment.c_str());
    }


    static void entryFunction() {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- entryFunction");
        a.nop();
    }

    static void exitFunction() {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
    }


    // preserve stack pointers
    static void preserveStackPointers() {
    }


    static void restoreStackPointers() {
    }


    // AsmJit related functions

    static void pushDS(asmjit::x86::Gp reg) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- pushDS");
        a.comment(" ; save value to the data stack (r15)");
        a.sub(asmjit::x86::r15, 8);
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::r15), reg);
    }

    static void popDS(asmjit::x86::Gp reg) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- popDS");
        a.comment(" ; fetch value from the data stack (r15)");
        a.nop();
        a.mov(reg, asmjit::x86::qword_ptr(asmjit::x86::r15));
        a.add(asmjit::x86::r15, 8);
    }

    // load the value from the address
    static void loadDS(void *dataAddress) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        // Load the address into rax
        a.comment(" ; ----- loadDS");
        a.comment(" ; ----- Dereference the address provided to get the value");
        a.mov(asmjit::x86::rax, dataAddress);
        // Dereference the address to get the value and store it into rax
        a.mov(asmjit::x86::rax, asmjit::x86::ptr(asmjit::x86::rax));
        // Push the value onto the data stack
        pushDS(asmjit::x86::rax);
    }

    // load address from DS, fetch value and push
    static void loadFromDS() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- loadDS");
        a.comment(" ; ----- Pop the address get the value, push it");

        // Load the address into rax
        popDS(asmjit::x86::rax);
        // Dereference the address to get the value and store it into rax
        a.mov(asmjit::x86::rax, asmjit::x86::ptr(asmjit::x86::rax));
        // Push the value onto the data stack
        pushDS(asmjit::x86::rax);
    }


    // store the value from DS into the address specified, consumes rax.
    static void storeDS(void *dataAddress) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- storeDS");
        a.comment(" ; ----- Pop the value store value at address provided");

        // Pop the value from the data stack into rax
        popDS(asmjit::x86::rax);

        // Load the address into rcx
        a.mov(asmjit::x86::rcx, dataAddress);
        // Store the value from rax into the address pointed to by rcx
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::rcx), asmjit::x86::rax);
    }


    // store the value from DS into the address from DS, consumes rax, rxc
    static void storeFromDS() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- storeFromDS");
        a.comment(" ; ----- Pop address, pop value store value at address ");

        // Pop the value from the data stack into rax
        popDS(asmjit::x86::rcx); // address
        popDS(asmjit::x86::rax); // data
        // Store the value from rax into the address pointed to by rcx
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::rcx), asmjit::x86::rax);
    }


    static void pushRS(asmjit::x86::Gp reg) {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- pushRS");
        a.comment(" ; save value to the return stack (r14)");
        a.nop();
        a.sub(asmjit::x86::r14, 8);
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::r14), reg);
    }

    static void popRS(asmjit::x86::Gp reg) {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- popRS");
        a.comment(" ; fetch value from the return stack (r14)");
        a.nop();
        a.mov(reg, asmjit::x86::qword_ptr(asmjit::x86::r14));
        a.add(asmjit::x86::r14, 8);
    }

    static void pushSS(asmjit::x86::Gp reg) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- pushSS");
        a.comment(" ; save value to the string stack (r12)");
        a.sub(asmjit::x86::r12, 8);
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::r12), reg);
    }


    static void prim_inc_ss() {
        sm.incSS();
    }

    static void pushSSAndBumpRef(asmjit::x86::Gp reg) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;

        a.comment("; pushSSAndBumpRef (arg provided)");
        a.comment("; Decrement string reference");

        a.sub(asmjit::x86::rsp, 8);
        a.call(prim_dec_ss);
        a.add(asmjit::x86::rsp, 8);

        a.comment(" ; ----- pushSS");
        a.comment(" ; save value to the string stack (r12)");
        a.sub(asmjit::x86::r12, 8);
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::r12), reg);
    }


    static void prim_dec_ss() {
        sm.decSS();
    }

    static void popSS(asmjit::x86::Gp reg) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- popSS");
        a.comment(" ; fetch value from the string stack (r12)");
        //a.comment(" ; update string reference count");
        // a.sub(asmjit::x86::rsp, 8);
        // a.call(prim_dec_ss);
        //  a.add(asmjit::x86::rsp, 8);
        // a.nop();
        a.mov(reg, asmjit::x86::qword_ptr(asmjit::x86::r12));
        a.add(asmjit::x86::r12, 8);
    }


    static void loadSS(void *dataAddress) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        // Load the address into rax
        a.mov(asmjit::x86::rax, dataAddress);

        // Dereference the address to get the value and store it into rax
        a.mov(asmjit::x86::rax, asmjit::x86::ptr(asmjit::x86::rax));

        // Push the value onto the string stack
        pushSS(asmjit::x86::rax);
    }


    static void loadFromSS() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        // Load the address into rax
        popSS(asmjit::x86::rax);
        // Dereference the address to get the value and store it into rax
        a.mov(asmjit::x86::rax, asmjit::x86::ptr(asmjit::x86::rax));
        // Push the value onto the string stack
        pushSS(asmjit::x86::rax);
    }

    static void storeSS(void *dataAddress) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        // Pop the value from the string stack into rax
        popSS(asmjit::x86::rax);

        // Load the address into rcx
        a.mov(asmjit::x86::rcx, dataAddress);
        // Store the value from rax into the address pointed to by rcx
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::rcx), asmjit::x86::rax);
    }


    static void storeFromSS() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        // Pop the value from the string stack into rax
        popSS(asmjit::x86::rcx); // address
        popSS(asmjit::x86::rax); // data
        // Store the value from rax into the address pointed to by rcx
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::rcx), asmjit::x86::rax);
    }


    // Function to find local by name
    static int findLocal(const std::string &word) {
        if (arguments.find(word) != arguments.end()) {
            jc.offset = arguments[word].offset;
            return arguments[word].offset;
        } else if (locals.find(word) != locals.end()) {
            jc.offset = locals[word].offset;
            return locals[word].offset;
        } else if (returnValues.find(word) != returnValues.end()) {
            jc.offset = returnValues[word].offset;
            return returnValues[word].offset;
        } else {
            return INVALID_OFFSET;
        }
    }


    // Function to find local by offset
    static std::string findLocalByOffset(int offset) {
        if (argumentsByOffset.find(offset) != argumentsByOffset.end()) {
            return argumentsByOffset[offset];
        } else if (localsByOffset.find(offset) != localsByOffset.end()) {
            return localsByOffset[offset];
        } else if (returnValuesByOffset.find(offset) != returnValuesByOffset.end()) {
            return returnValuesByOffset[offset];
        } else {
            return ""; // Return an empty string if not found
        }
    }

    static void addArgument(const std::string &name, int offset) {
        arguments[name] = {name, offset};
        argumentsByOffset[offset] = name;
    }

    static void addLocal(const std::string &name, int offset) {
        locals[name] = {name, offset};
        localsByOffset[offset] = name;
    }

    static void addReturnValue(const std::string &name, int offset) {
        returnValues[name] = {name, offset};
        returnValuesByOffset[offset] = name;
    }


    static void fetchLocal(asmjit::x86::Gp reg, int offset) {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- fetchLocal");
        a.nop();
        a.mov(reg, asmjit::x86::qword_ptr(asmjit::x86::r13, offset));
        pushDS(reg);
    }


    static void genPushLocal(int offset) {
        printf("genPushLocal %d\n", offset);
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;


        jc.word = findLocalByOffset(offset);

        commentWithWord(" ; ----- fetchLocal");
        asmjit::x86::Gp reg = asmjit::x86::ecx;
        a.nop();
        a.mov(reg, asmjit::x86::qword_ptr(asmjit::x86::r13, offset));
        pushDS(reg);
    }

    static void storeLocal(asmjit::x86::Gp reg, int offset) {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- storeLocal");
        a.nop();
        popDS(reg);
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::r13, offset), reg);
    }

    static void allocateLocals(int count) {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.sub(asmjit::x86::r13, count * 8);
    }


    static void commentWithWord(const std::string &baseComment, const std::string &word) {
        JitContext &jc = JitContext::getInstance();
        if (jc.assembler) {
            std::string comment = baseComment + " " + word;
            jc.assembler->comment(comment.c_str());
        }
    }


    // gen_leftBrace, processes the locals brace { a b | cd -- e } etc.
    static void gen_leftBrace() {
        JitContext &jc = JitContext::getInstance();

        if (!jc.assembler) {
            throw std::runtime_error("gen_leftBrace: Assembler not initialized");
        }

        // Clear previous data
        arguments.clear();
        argumentsByOffset.clear();
        locals.clear();
        localsByOffset.clear();
        returnValues.clear();
        returnValuesByOffset.clear();
        arguments_to_local_count = 0;
        locals_count = 0;
        returned_arguments_count = 0;


        auto &a = *jc.assembler;
        a.comment(" ; ----- leftBrace: locals detected");
        a.nop();

        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1; // Start just after the left brace

        enum ParsingMode {
            ARGUMENTS,
            LOCALS,
            RETURN_VALUES
        } mode = ARGUMENTS;

        int offset = 0;
        while (pos < words.size()) {
            const std::string &word = words[pos];
            jc.word = word;

            if (word == "}") {
                break;
            } else if (word == "|") {
                mode = LOCALS;
            } else if (word == "--") {
                mode = RETURN_VALUES;
            } else {
                commentWithWord(" ; ----- prepare  ", word);
                switch (mode) {
                    case ARGUMENTS:
                        commentWithWord(" ; ----- argument ", word);
                        addArgument(word, offset);
                        arguments_to_local_count++;
                        break;
                    case LOCALS:
                        commentWithWord(" ; ----- local ", word);
                        addLocal(word, offset);
                        locals_count++;
                        break;
                    case RETURN_VALUES:
                        commentWithWord(" ; ----- return value ", word);
                        addReturnValue(word, offset);
                        returned_arguments_count++;
                        break;
                }
                offset += 8;
            }
            ++pos;
        }

        if (logging) {
            printf("arguments_to_local_count: %d\n", arguments_to_local_count);
            printf("locals_count: %d\n", locals_count);
            printf("returned_arguments_count: %d\n", returned_arguments_count);
        }


        jc.pos_last_word = pos;

        // Generate locals code
        const int totalLocalsCount = arguments_to_local_count + locals_count + returned_arguments_count;
        if (totalLocalsCount > 0) {
            a.comment(" ; ----- allocate locals");
            allocateLocals(totalLocalsCount);

            a.comment(" ; --- BEGIN copy args to locals");
            for (int i = 0; i < arguments_to_local_count; ++i) {
                asmjit::x86::Gp argReg = asmjit::x86::rcx;
                int offset = i * 8; // Offsets are allocated upwards from r13.
                jc.word = findLocalByOffset(offset);
                copyLocalFromDS(argReg, offset); // Copy the argument to the return stack
            }
            a.comment(" ; --- END copy args to locals");

            a.comment(" ; --- BEGIN zero remaining locals");
            int zeroOutCount = locals_count + returned_arguments_count;
            for (int j = 0; j < zeroOutCount; ++j) {
                int offset = (j + arguments_to_local_count) * 8; // Offset relative to the arguments.
                jc.word = findLocalByOffset(offset);
                zeroStackLocation(offset); // Use a helper function to zero out the stack location.
            }
            a.comment(" ; --- END zero remaining locals");
        }
    }


    // genFetch - fetch the contents of the address
    static void genFetch(uint64_t address) {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        asmjit::x86::Gp addr = asmjit::x86::rax; // General purpose register for address
        asmjit::x86::Gp value = asmjit::x86::rdi; // General purpose register for the value
        a.mov(addr, address); // Move the address into the register.
        a.mov(value, asmjit::x86::ptr(addr));
        pushDS(value); // Push the value onto the stack.
    }

    // display details on word
    static void see() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;
        std::string w = words[pos];
        jc.word = w;
        // display word w
        d.displayWord(w);
        jc.pos_last_word = pos;
    }


    // The TO word safely updates the container word
    // in compile mode only.
    static void genTO() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;

        std::string w = words[pos];
        jc.word = w;
        // This needs to be a word we can store things in.

        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        // Current assembler instance
        auto &a = *jc.assembler;

        // Check for a local variable
        int offset = findLocal(w);
        if (offset != INVALID_OFFSET) {
            commentWithWord("; TO ----- pop stack into local variable: ", w);
            // Pop the value from the data stack into ecx
            popDS(asmjit::x86::ecx);
            // Store the value into the local variable
            a.mov(asmjit::x86::qword_ptr(asmjit::x86::r13, offset), asmjit::x86::ecx);
            jc.pos_last_word = pos;
            return;
        }

        // Get the word from the dictionary

        auto fword = d.findWord(w.c_str());
        if (fword) {
            auto word_type = fword->type;
            if (logging) printf("word_type: %d\n", word_type);
            if (word_type == ForthWordType::VALUE || word_type == ForthWordType::FLOATVALUE) // value
            {
                auto data_address = d.get_data_ptr();
                if (logging) printf("data_address: %p\n", data_address);
                // Load the address of the word's data
                a.mov(asmjit::x86::rax, data_address);

                // Pop the value from the data stack into rcx
                popDS(asmjit::x86::rcx);

                // Store the value into the address
                a.mov(asmjit::x86::qword_ptr(asmjit::x86::rax), asmjit::x86::rcx);
            } else if (word_type == ForthWordType::CONSTANT) {
                commentWithWord("; TO ----- can not update constant: ", w);
                if (logging) printf("constant: %s\n", w.c_str());
                throw std::runtime_error("TO can not update constant: " + w);
            } else if (word_type == ForthWordType::VARIABLE) // variable
            {
                commentWithWord("; TO ----- pop stack into VARIABLE: ", w);
                auto data_address = d.get_data_ptr();
                a.mov(asmjit::x86::rax, data_address);

                // Pop the value from the data stack into rcx
                popDS(asmjit::x86::rcx);

                // Store the value into the address
                a.mov(asmjit::x86::qword_ptr(asmjit::x86::rax), asmjit::x86::rcx);
            } else if (word_type == ForthWordType::ARRAY) // ARRAY
            {
                commentWithWord("; TO ----- updating ARRAY: ", w);
                const auto limit = fword->getUint64();
                const auto normal_continue = a.newLabel();
                const auto throw_error = a.newLabel();

                printf("array limit = %lld\n", limit);

                // Pop index and value from the data stack
                popDS(asmjit::x86::rdx); // index
                popDS(asmjit::x86::rcx); // value

                // Check if index is in bounds
                a.cmp(asmjit::x86::rdx, limit);
                a.jae(throw_error);

                // Calculate address for the array element
                const auto base_address = reinterpret_cast<uint64_t>(&fword->data);
                a.mov(asmjit::x86::rax, base_address);
                a.add(asmjit::x86::rax, 8);
                a.lea(asmjit::x86::rax, asmjit::x86::qword_ptr(asmjit::x86::rax, asmjit::x86::rdx, 3));
                // Store the value
                a.mov(asmjit::x86::qword_ptr(asmjit::x86::rax), asmjit::x86::rcx);
                a.jmp(normal_continue);

                // Label for out-of-bounds jump
                a.comment("; throw error");
                a.bind(throw_error);

                a.sub(asmjit::x86::rsp, 8);
                a.call(throw_array_index_error);
                a.add(asmjit::x86::rsp, 8);

                a.comment("; continue as normal");
                a.bind(normal_continue);

                jc.pos_last_word = pos;
            } else if (word_type == ForthWordType::STRING) // variable
            {
                // Get the address of the variable's data
                auto *variable_address = reinterpret_cast<int64_t *>(d.get_data_ptr());
                if (logging) printf("string address: %p\n", variable_address);
                if (!variable_address) {
                    throw std::runtime_error("Failed to get string address for word: " + w);
                }

                a.mov(asmjit::x86::rax, variable_address);

                // Pop the value from the data stack into rcx
                popSS(asmjit::x86::rcx);

                // Store the value into the address
                a.mov(asmjit::x86::qword_ptr(asmjit::x86::rax), asmjit::x86::rcx);
            }
            jc.pos_last_word = pos;
        } else {
            throw std::runtime_error("Unknown word in TO: " + w);
        }
    }


    // The TO word safely updates the container word
    // in interpret mode only.
    static void execTO() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;

        std::string w = words[pos];
        jc.word = w;


        // Get the word from the dictionary
        auto fword = d.findWord(w.c_str());
        if (fword) {
            auto word_type = fword->type;
            if (word_type == ForthWordType::VALUE || word_type == ForthWordType::FLOATVALUE) // value
            {
                auto data_address = d.get_data_ptr();

                // Pop the value from the data stack
                auto value = sm.popDS();

                // Store the value into the address
                *reinterpret_cast<int64_t *>(data_address) = value;
            } else if (word_type == ForthWordType::CONSTANT) {
                commentWithWord("; TO ----- can not update constant: ", w);
                throw std::runtime_error("TO can not update constant: " + w);
            } else if (word_type == ForthWordType::VARIABLE) // variable
            {
                // Load the address of the data (double indirect access)
                auto variable_address = reinterpret_cast<uint64_t>(&fword->data);
                // Pop the value from the data stack
                auto value = sm.popDS();
                // Store the value into the address the variable points to
                *reinterpret_cast<int64_t *>(variable_address) = value;
            } else if (word_type == ForthWordType::ARRAY) // ARRAY
            {
                // value index TO array
                // check index in range and if so set value, else throw error.

                const auto limit = fword->getUint64();
                auto index = sm.popDS();
                printf("index = %llu", index);
                if (index >= limit) {
                    throw std::runtime_error("Index out of bounds for array: " + w);
                }
                auto variable_address = reinterpret_cast<uint64_t>(&fword->data);
                variable_address += 8 + (index * 8);

                // Pop the value from the data stack
                const auto value = sm.popDS();
                // Store the value into the address the variable points to
                *reinterpret_cast<int64_t *>(variable_address) = value;
            } else if (word_type == ForthWordType::STRING) // variable
            {
                // update a string variable from the string stack.
                auto variable_address = d.get_data_ptr();
                size_t string_address = sm.popSS();
                strIntern.incrementRef(string_address);
                fword->data = string_address; // update the data pointer to point to the string
            }
            jc.pos_last_word = pos;
        } else {
            throw std::runtime_error("Unknown word in TO: " + w);
        }
    }


    // char a . = 97
    static void genImmediateChar() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;
        // Get the next word from the input stream
        std::string word = words[pos];
        jc.word = word;
        // Extract the first character from the word
        char charValue = word.front();
        auto initialValue = static_cast<uint64_t>(charValue);
        // Reset the context
        jc.resetContext();
        if (!jc.assembler) {
            throw std::runtime_error("genImmediateChar: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- immediate char: ", std::string(1, charValue));
        a.comment(" ; ----- fetch char");
        // move charValue to rax
        a.mov(asmjit::x86::rax, initialValue);
        pushDS(asmjit::x86::rax); // Push the address of the data stack onto the stack.
        jc.pos_last_word = pos;
    }


    static void genTerpImmediateChar() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;
        // Get the next word from the input stream
        std::string word = words[pos];
        // Extract the first character from the word
        char charValue = word.front();
        auto initialValue = static_cast<uint64_t>(charValue);
        sm.pushDS(initialValue);
        jc.pos_last_word = pos;
    }

    // 100 ARRAY test
    // create a value in the dictionary where the value is the array size
    // followed by the allotted data for the array.
    // at run time array returns indexed item
    // e.g 10 test returns value at index 10 of array test


    // check array index error
    static void throw_array_index_error() {
        throw std::runtime_error("Array index out of range.");
    }

    static void genImmediateArray() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;
        std::string word = words[pos];
        jc.word = word;

        // Pop the array size from the data stack
        auto arraySize = sm.popDS();
        //printf("initialValue: %llu\n", initialValue);
        jc.resetContext();
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- immediate array: ", word);

        // Add the word to the dictionary as an array value
        d.addWord(word.c_str(), nullptr, nullptr, nullptr, nullptr);
        d.setData(arraySize); // Set the value
        auto dataAddress = d.get_data_ptr();
        d.setType(ForthWordType::ARRAY); // value array type
        d.allot(arraySize * 8); // create space in dictionary for the array of ints, floats, pointers etc

        a.comment(" ; ----- return content of indexed element");

        auto index_error = a.newLabel();
        asmjit::x86::Gp base = asmjit::x86::rax;
        asmjit::x86::Gp index = asmjit::x86::rbx;
        asmjit::x86::Gp result = asmjit::x86::rcx;

        popDS(index);
        a.cmp(index, arraySize);
        a.jae(index_error);

        a.mov(base, static_cast<uint8_t *>(dataAddress) + 8);
        a.shl(index, 3); // always * 8
        a.add(base, index);
        // load result with contents of base
        a.mov(result, asmjit::x86::ptr(base));
        pushDS(result);
        a.ret();

        a.comment("; throw error - if array index out of bounds");
        a.bind(index_error);
        a.sub(asmjit::x86::rsp, 8);
        a.call(throw_array_index_error);
        a.add(asmjit::x86::rsp, 8);

        a.ret();

        ForthFunction compiledFunc = endGeneration();
        d.setCompiledFunction(compiledFunc);
        jc.pos_last_word = pos;
    }


    // immediate value, runs when value is called.
    // 10 VALUE fred
    static void genImmediateValue() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;

        std::string word = words[pos];
        jc.word = word;


        // Pop the initial value from the data stack
        auto initialValue = sm.popDS();
        //printf("initialValue: %llu\n", initialValue);
        jc.resetContext();
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- immediate value: ", word);
        // Add the word to the dictionary as a value
        d.addWord(word.c_str(), nullptr, nullptr, nullptr, nullptr);
        d.setData(initialValue); // Set the value
        auto dataAddress = d.get_data_ptr();
        d.setType(ForthWordType::VALUE); // value type

        a.comment(" ; ----- fetch value");
        loadDS(dataAddress);
        a.ret();

        ForthFunction compiledFunc = endGeneration();
        d.setCompiledFunction(compiledFunc);
        jc.pos_last_word = pos;
    }


    // immediate value, runs when value is called.
    // 10.0 FVALUE fred
    static void genImmediateFvalue() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;

        std::string word = words[pos];
        jc.word = word;


        // Pop the initial value from the data stack
        auto initialValue = sm.popDS();
        //printf("initialValue: %llu\n", initialValue);
        jc.resetContext();
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- immediate value: ", word);
        // Add the word to the dictionary as a value
        d.addWord(word.c_str(), nullptr, nullptr, nullptr, nullptr);
        d.setData(initialValue); // Set the value
        auto dataAddress = d.get_data_ptr();
        d.setType(ForthWordType::FLOATVALUE); // value type, capture the intention.

        a.comment(" ; ----- fetch value");
        loadDS(dataAddress); // DS is also used for floats

        a.ret();

        ForthFunction compiledFunc = endGeneration();
        d.setCompiledFunction(compiledFunc);
        jc.pos_last_word = pos;
    }


    // immediate value, runs when value is called.
    // 10 VALUE fred
    static void genImmediateConstant() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;

        std::string word = words[pos];
        jc.word = word;
        // Pop the initial value from the data stack
        auto initialValue = sm.popDS();
        //printf("initialValue: %llu\n", initialValue);
        jc.resetContext();
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- immediate value: ", word);
        // Add the word to the dictionary as a value
        d.addWord(word.c_str(), nullptr, nullptr, nullptr, nullptr);
        d.setData(initialValue); // Set the value
        auto dataAddress = d.get_data_ptr();
        d.setType(ForthWordType::CONSTANT); // value type

        a.comment(" ; ----- fetch value");
        loadDS(dataAddress);
        a.ret();

        ForthFunction compiledFunc = endGeneration();
        d.setCompiledFunction(compiledFunc);
        jc.pos_last_word = pos;
    }


    static void genImmediatefConstant() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;

        std::string word = words[pos];
        jc.word = word;

        // Pop the initial value from the data stack
        double initialValue;
        try {
            initialValue = sm.popDSDouble();
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Failed to pop double from stack: ") + e.what());
        }

        jc.resetContext();
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- immediate value: ", word);

        // Add the word to the dictionary as a value
        d.addWord(word.c_str(), nullptr, nullptr, nullptr, nullptr);


        // Set the initial value explicitly
        d.setDataDouble(initialValue);

        auto dataAddress = d.get_data_ptr();
        d.setType(ForthWordType::FLOATCONSTANT); // value type

        a.comment(" ; ----- fetch value");
        loadDS(dataAddress);
        a.ret();

        ForthFunction compiledFunc = endGeneration();
        d.setCompiledFunction(compiledFunc);
        jc.pos_last_word = pos;
    }


    // immediate value, runs when value is called.
    // s" literal string" VALUE fred
    static void genImmediateStringValue() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;

        std::string word = words[pos];
        jc.word = word;


        // Pop the initial value from the data stack
        auto initialValue = sm.popSS();
        strIntern.incrementRef(initialValue);
        printf("initialValue: %llu\n", initialValue);
        jc.resetContext();
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- immediate value: ", word);
        // Add the word to the dictionary as a value
        d.addWord(word.c_str(), nullptr, nullptr, nullptr, nullptr);
        d.setData(initialValue); // Set the value
        auto dataAddress = d.get_data_ptr();
        d.setType(ForthWordType::STRING); // value type

        a.comment(" ; ----- fetch value");
        loadSS(dataAddress);
        a.ret();

        ForthFunction compiledFunc = endGeneration();
        d.setCompiledFunction(compiledFunc);
        // Update position
        jc.pos_last_word = pos;
    }


    static void genImmediateVariable() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;

        std::string word = words[pos];
        jc.word = word;

        jc.resetContext();
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- immediate value: ", word);
        // Add the word to the dictionary as a value
        d.addWord(word.c_str(), nullptr, nullptr, nullptr, nullptr);
        d.setData(0); // Set the value
        d.setType(ForthWordType::VARIABLE); // variable type

        auto dataAddress = d.get_data_ptr();
        // Generate prologue for function
        //printf("dataAddress: %llu\n", dataAddress);
        // use the data address to fetch the value
        a.comment(" ; ----- fetch variable address ");
        a.mov(asmjit::x86::rax, dataAddress);
        pushDS(asmjit::x86::rax);
        a.ret();
        ForthFunction compiledFunc = endGeneration();
        d.setCompiledFunction(compiledFunc);
        // Update position
        jc.pos_last_word = pos;
    }


    static void prim_string_cat() {
        const size_t s1 = sm.popSS();
        const size_t s2 = sm.popSS();
        const size_t s3 = strIntern.StringCat(s2, s1);
        strIntern.incrementRef(s3);
        sm.pushSS(s3);
    }

    // s+
    static void genStringCat() {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- .s+ calls strcat ");
        a.sub(asmjit::x86::rsp, 8);
        a.call(prim_string_cat);
        a.add(asmjit::x86::rsp, 8);
    }

    static void prim_str_pos() {
        const size_t s1 = sm.popSS();
        const size_t s2 = sm.popSS();
        const int pos = strIntern.StrPos(s1, s2);
        sm.pushDS(static_cast<size_t>(pos));
    }

    static void genStrPos() {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- .pos calls strpos ");
        a.sub(asmjit::x86::rsp, 8);
        a.call(prim_str_pos);
        a.add(asmjit::x86::rsp, 8);
    }

    static void prim_string_field() {
        const size_t s1 = sm.popSS();
        const size_t delimiter = sm.popSS();
        const size_t position = sm.popDS();
        const size_t result = strIntern.StringSplit(delimiter, s1, position);
        strIntern.incrementRef(result);
        sm.pushSS(result);
    }

    // extract a field.
    static void genStringField() {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- .split calls string split ");
        a.sub(asmjit::x86::rsp, 8);
        a.call(prim_string_field);
        a.add(asmjit::x86::rsp, 8);
    }

    static void prim_count_fields() {
        const size_t s1 = sm.popSS();
        const size_t s2 = sm.popSS();
        const size_t count = strIntern.CountFields(s2, s1);
        sm.pushDS(count);
    }

    static void genCountFields() {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- .count calls count fields ");
        a.sub(asmjit::x86::rsp, 8);
        a.call(prim_count_fields);
        a.add(asmjit::x86::rsp, 8);
    }


    static size_t stripIndex(const std::string &token) {
        std::string prefix = "sPtr_";
        if (token.compare(0, prefix.size(), prefix) == 0) {
            // Extract the numeric part after the prefix
            std::string numberStr = token.substr(prefix.size());
            std::uintptr_t address = 0;

            // Convert the numeric string to an unsigned integer
            std::istringstream iss(numberStr);
            iss >> address;

            // Return the address as a pointer to const char
            return address;
        }

        // Return nullptr if the prefix does not match
        return -1;
    }

    // supports ."
    static void genImmediateDotQuote() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;
        std::string word = words[pos];
        jc.word = word;
        if (logging) printf("genImmediateDotQuote: %s\n", word.c_str());
        const auto index = stripIndex(word);
        strIntern.incrementRef(index);
        auto address = strIntern.getStringAddress(index);

        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        commentWithWord(" ; ----- .\" displaying text ");

        // put parameter in argument

        a.push(asmjit::x86::rdi);
        a.mov(asmjit::x86::rdi, address);
        a.call(asmjit::imm(reinterpret_cast<uint64_t>(prints))); // Call the function in rax
        a.pop(asmjit::x86::rdi); // Restore RDI after the call

        jc.pos_last_word = pos;
    }

    // support s" for compiler code generation
    static void genImmediateSQuote() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;
        std::string word = words[pos];
        jc.word = word;

        auto address = stripIndex(word);

        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        commentWithWord(" ; ----- s\" stacking text ");
        a.mov(asmjit::x86::rcx, address);
        pushSS(asmjit::x86::rcx);

        jc.pos_last_word = pos;
    }

    // supports sprint
    // takes index from string stack turns to address and prints at run time.
    static void genPrint() {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        commentWithWord(" ; ----- sprint prints string ");

        a.push(asmjit::x86::rdi);
        popDS(asmjit::x86::rdi);
        a.call(asmjit::imm(reinterpret_cast<uint64_t>(prim_sindex))); // Call the function in rax
        a.pop(asmjit::x86::rdi); // Restore RDI after the call

        a.push(asmjit::x86::rdi);
        popDS(asmjit::x86::rdi);
        a.call(asmjit::imm(reinterpret_cast<uint64_t>(prints))); // Call the function in rax
        a.pop(asmjit::x86::rdi); // Restore RDI after the call
    }


    // support s" for interpreter immediate execution.
    static void genTerpImmediateSQuote() {
        const auto &words = *jc.words;
        size_t pos = jc.pos_next_word + 1;
        std::string word = words[pos];
        jc.word = word;

        auto address = stripIndex(word);
        sm.pushSS(static_cast<uint64_t>(address));
        jc.pos_last_word = pos;
    }


    //
    static void copyLocalFromDS(asmjit::x86::Gp reg, int offset) {
        if (!jc.assembler) {
            throw std::runtime_error("entryFunction: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        commentWithWord(" ; ----- pop from stack into ");
        // Pop from the data stack (r15) to the register.
        popDS(reg);
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::r13, offset), reg);
    }

    static void zeroStackLocation(int offset) {
        if (!jc.assembler) {
            throw std::runtime_error("zeroStackLocation: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        commentWithWord(" ; ----- Clearing ");
        asmjit::x86::Gp zeroReg = asmjit::x86::rcx; //
        a.xor_(zeroReg, zeroReg); // Set zeroReg to zero.
        a.mov(asmjit::x86::qword_ptr(asmjit::x86::r13, offset), zeroReg); // Move zero into the stack location.
    }

    // prologue happens when we begin a new word.
    static void genPrologue() {
        jc.resetContext();
        if (logging) std::cout << "; gen_prologue\n";
        if (!jc.assembler) {
            throw std::runtime_error("gen_prologue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- function prologue -------------------------");
        a.nop();
        entryFunction();
        a.comment(" ; ----- ENTRY label");


        FunctionEntryExitLabel funcLabels;
        funcLabels.entryLabel = a.newLabel();
        funcLabels.exitLabel = a.newLabel();
        a.bind(funcLabels.entryLabel);


        // Save on loopStack
        const LoopLabel loopLabel{LoopType::FUNCTION_ENTRY_EXIT, funcLabels};
        loopStack.push(loopLabel);

        if (logging) std::cout << " ; gen_prologue: " << static_cast<void *>(jc.assembler) << "\n";
    }


    // happens just before the function returns
    static void genEpilogue() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_epilogue: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        jc.epilogueLabel = a.newLabel();
        a.bind(jc.epilogueLabel);

        a.nop();
        a.comment(" ; ----- gen_epilogue");

        a.comment(" ; ----- EXIT label");

        // Check if loopStack is empty
        if (loopStack.empty()) {
            throw std::runtime_error("gen_epilogue: loopStack is empty");
        }

        auto loopLabelVariant = loopStack.top();
        if (loopLabelVariant.type != LoopType::FUNCTION_ENTRY_EXIT) {
            throw std::runtime_error("gen_epilogue: Top of loopStack is not a function entry/exit label");
        }

        const auto &label = std::get<FunctionEntryExitLabel>(loopLabelVariant.label);
        a.bind(label.exitLabel);
        loopStack.pop();

        // locals copy return values to stack.
        const int totalLocalsCount = arguments_to_local_count + locals_count + returned_arguments_count;
        if (totalLocalsCount > 0) {
            a.comment(" ; ----- LOCALS in use");

            if (returned_arguments_count > 0) {
                a.comment(" ; ----- copy any return values to stack");
                // Copy `returned_arguments_count` values onto the data stack
                for (int i = 0; i < returned_arguments_count; ++i) {
                    int offset = (i + arguments_to_local_count + locals_count) * 8;
                    // Offset relative to the stack base.
                    jc.word = findLocalByOffset(offset);

                    commentWithWord(" ; ----- copy return value ");
                    asmjit::x86::Gp returnValueReg = asmjit::x86::ecx;
                    a.mov(returnValueReg, asmjit::x86::qword_ptr(asmjit::x86::r13, offset));
                    // Move the return value from the stack location to the register.
                    pushDS(returnValueReg); // Push the return value onto the data stack (r15).
                }
            }
            // Free the total stack space on the locals stack
            a.comment(" ; ----- free locals");
            a.add(asmjit::x86::r13, totalLocalsCount * 8);
            // Restore the return stack pointer by adding the total local count.
            arguments_to_local_count = locals_count = returned_arguments_count = 0;
        }

        exitFunction();
        // Free the total stack space on the return stack pointer.
        a.ret();
    }


    // exit jump off the word.
    // needs to pop values from the return stack.

    static void genExit() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_exit: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_exit");
        // Create a temporary stack to hold the popped labels
        std::stack<LoopLabel> tempStack;
        bool found = false;
        auto drop_bytes = 8 * doLoopDepth;
        a.add(asmjit::x86::r14, drop_bytes);
        a.ret(); // return early from function.
    }


    // spit out a charachter
    static void genEmit() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_emit: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_emit");

        preserveStackPointers();
        a.push(asmjit::x86::rdi);
        popDS(asmjit::x86::rdi);
        a.call(asmjit::imm(reinterpret_cast<uint64_t>(prim_emit))); // Call the function in rax
        a.pop(asmjit::x86::rdi); // Restore RDI after the call
        restoreStackPointers();
    }

    static void dotS() {
        sm.displayStacks();
    }

    static void words() {
        d.list_words();
    }

    static void prim_forget() {
        d.forgetLastWord();
    }

    static void *prim_sindex(size_t index) {
        auto address = strIntern.getStringAddress(index);
        // printf("address: %p\n", address);
        return address;
    }


    static void genForget() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_emit: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_emit");

        preserveStackPointers();
        a.sub(asmjit::x86::rsp, 8);
        a.call(asmjit::imm(reinterpret_cast<void *>(prim_forget)));
        // Restore stack.
        a.add(asmjit::x86::rsp, 8);
        restoreStackPointers();
    }


    // display labels

    static void displayBeginLabel(BeginAgainRepeatUntilLabel *label) {
        // display label contents
        printf(" ; ----- BEGIN label\n");
    }


    static void genDot() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_emit: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_dot");

        preserveStackPointers();
        // Allocate space for the shadow space (32 bytes).
        a.push(asmjit::x86::rdi);
        popDS(asmjit::x86::rdi);
        a.call(asmjit::imm(reinterpret_cast<uint64_t>(printDecimal))); // Call the function in rax
        a.pop(asmjit::x86::rdi); // Restore RDI after the call

        restoreStackPointers();
    }

    static void genHDot() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_emit: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen hex dot");
        popDS(asmjit::x86::rcx);
        preserveStackPointers();
        // Allocate space for the shadow space (32 bytes).
        a.sub(asmjit::x86::rsp, 8);
        a.call(asmjit::imm(reinterpret_cast<void *>(printUnsignedHex)));
        // Restore stack.
        a.add(asmjit::x86::rsp, 8);
        restoreStackPointers();
    }


    static void prim_depth() {
        const uint64_t depth = sm.getDSDepth();
        sm.pushDS(depth + 1);
    }

    static void prim_depth2() {
        const uint64_t depth = sm.getDSDepth();
        sm.pushDS(depth);
    }


    static void genDepth() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_emit: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_emit");
        preserveStackPointers();
        a.push(asmjit::x86::rdi);
        a.call(asmjit::imm(reinterpret_cast<void *>(prim_depth)));
        a.pop(asmjit::x86::rdi); // Restore RDI after the call
        restoreStackPointers();
    }


    static void genDepth2() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_emit: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_emit");

        preserveStackPointers();
        // Allocate space for the shadow space (32 bytes).
        a.push(asmjit::x86::rdi);
        a.call(asmjit::imm(reinterpret_cast<void *>(prim_depth2)));
        a.pop(asmjit::x86::rdi); // Restore RDI after the call
        restoreStackPointers();
    }

    static void genPushLong() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_push_long: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_push_long");
        a.comment(" ; Push long value onto the stack");
        a.mov(asmjit::x86::rcx, jc.uint64_A);
        pushDS(asmjit::x86::rcx);
    }


    static void genPushDouble() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_push_long: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_push_long");
        a.comment(" ; Push long value onto the stack");
        a.mov(asmjit::x86::rcx, jc.double_A);
        pushDS(asmjit::x86::rcx);
    }


    static void genSubLong() {
        if (!jc.assembler) {
            throw std::runtime_error("genSubLong: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genSubLong");
        a.comment(" ; Subtract immediate long value from the top of the stack");

        // Pop value from the stack into `rax`
        popDS(asmjit::x86::rax);

        // Subtract immediate value `jc.uint64_A` from `rax`
        a.sub(asmjit::x86::rax, jc.uint64_A);

        // Push the result back onto the stack
        pushDS(asmjit::x86::rax);
    }

    static void genPlusLong() {
        if (!jc.assembler) {
            throw std::runtime_error("genPlusLong: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genPlusLong");
        a.comment(" ; Add immediate long value to the top of the stack");

        // Pop value from the stack into `rax`
        popDS(asmjit::x86::rax);

        // Add immediate value `jc.uint64_A` to `rax`
        a.add(asmjit::x86::rax, jc.uint64_A);

        // Push the result back onto the stack
        pushDS(asmjit::x86::rax);
    }

    static ForthFunction endGeneration() {
        if (!jc.assembler) {
            throw std::runtime_error("end: Assembler not initialized");
        }
        // Finalize the function
        ForthFunction func;
        if (const asmjit::Error err = jc.rt.add(&func, &jc.code)) {
            throw std::runtime_error(asmjit::DebugUtils::errorAsString(err));
        }

        return func;
    }

    // return a function after building a function around its generator fn
    static ForthFunction build_forth(const ForthFunction fn) {
        if (logging) std::cout << "; building forth function ... \n";
        if (!jc.assembler) {
            throw std::runtime_error("build: Assembler not initialized");
        }
        genPrologue();
        fn();
        genEpilogue();
        const ForthFunction new_func = endGeneration();
        return new_func;
    }

    static void genCall2(ForthFunction fn) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_call: Assembler not initialized");
        }
        auto &a = *jc.assembler;

        a.comment(" ; ----- gen_call");
        a.mov(asmjit::x86::rax, fn);
        a.push(asmjit::x86::rdi);
        a.call(asmjit::x86::rax);
        a.pop(asmjit::x86::rdi);
    }


    static void genCall(ForthFunction fn) {
        if (!jc.assembler) {
            throw std::runtime_error("gen_call: Assembler not initialized");
        }
        auto &a = *jc.assembler;

        a.comment(" ; ----- gen_call");

        a.push(asmjit::x86::rdi);
        a.call(asmjit::imm(fn));
        a.pop(asmjit::x86::rdi);
    }

    // Executable function pointer


    /*
    static ExecFunc endExec()
    {
        if (!jc.assembler)
        {
            throw std::runtime_error("end: Assembler not initialized");
        }
        // Finalize the function
        ExecFunc func;
        if (const asmjit::Error err = jc.rt.add(&func, &jc.code))
        {
            throw std::runtime_error(asmjit::DebugUtils::errorAsString(err));
        }

        return func;
    }


    static void generateExecFunction()
    {
        if (!jc.assembler)
        {
            throw std::runtime_error("gen_push_long: Assembler not initialized");
        }

        auto& a = *jc.assembler;
        a.comment(" ; ----- gen_exec");

        a.mov(asmjit::x86::rax, asmjit::x86::ptr(asmjit::x86::rsp, 8));
        a.call(asmjit::x86::rax);

        a.ret();

        const ExecFunc new_func = endExec();
        exec = new_func;
    }
    */


    // return stack words..

    static void genToR() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_toR: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_toR");

        asmjit::x86::Gp value = asmjit::x86::r8; // Temporary register for value

        // Pop value from DS (represented by r15)
        a.comment(" ; Pop value from DS");
        a.mov(value, asmjit::x86::ptr(asmjit::x86::r15));
        a.add(asmjit::x86::r15, sizeof(uint64_t));

        // Push value to RS (represented by r14)
        a.comment(" ; Push value to RS");
        a.sub(asmjit::x86::r14, sizeof(uint64_t));
        a.mov(asmjit::x86::ptr(asmjit::x86::r14), value);
    }

    static void genRFrom() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_rFrom: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_rFrom");

        asmjit::x86::Gp value = asmjit::x86::r8; // Temporary register for value

        // Pop value from RS (represented by r14)
        a.comment(" ; Pop value from RS");
        a.mov(value, asmjit::x86::ptr(asmjit::x86::r14));
        a.add(asmjit::x86::r14, sizeof(uint64_t));

        // Push value to DS (represented by r15)
        a.comment(" ; Push value to DS");
        a.sub(asmjit::x86::r15, sizeof(uint64_t));
        a.mov(asmjit::x86::ptr(asmjit::x86::r15), value);
    }


    static void genRFetch() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_rFetch: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_rFetch");

        asmjit::x86::Gp value = asmjit::x86::r8; // Temporary register for value

        // Fetch (not pop) value from RS (represented by r14)
        a.comment(" ; Fetch value from RS");
        a.mov(value, asmjit::x86::ptr(asmjit::x86::r14));

        // Push fetched value to DS (represented by r15)
        a.comment(" ; Push fetched value to DS");
        a.sub(asmjit::x86::r15, sizeof(uint64_t));
        a.mov(asmjit::x86::ptr(asmjit::x86::r15), value);
    }


    static void genRPFetch() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_rpFetch: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_rpFetch");

        asmjit::x86::Gp rsPointer = asmjit::x86::r8; // Temporary register for RS pointer

        // Get RS pointer (which is in r14) and push it to DS (r15)
        a.comment(" ; Fetch RS pointer and push to DS");
        a.mov(rsPointer, asmjit::x86::r14);
        a.sub(asmjit::x86::r15, sizeof(uint64_t));
        a.mov(asmjit::x86::ptr(asmjit::x86::r15), rsPointer);
    }


    static void genSPFetch() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_spFetch: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_spFetch");

        asmjit::x86::Gp dsPointer = asmjit::x86::r8; // Temporary register for DS pointer

        // Get DS pointer (which is in r15) and push it to the DS itself
        a.comment(" ; Fetch DS pointer and push to DS");
        a.mov(dsPointer, asmjit::x86::r15);
        a.sub(asmjit::x86::r15, sizeof(uint64_t));
        a.mov(asmjit::x86::ptr(asmjit::x86::r15), dsPointer);
    }


    static void genSPStore() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_spStore: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_spStore");

        asmjit::x86::Gp newDsPointer = asmjit::x86::r8; // Temporary register for new DS pointer

        // Pop new data stack pointer value from the data stack itself
        a.comment(" ; Pop new DS pointer from DS");
        a.mov(newDsPointer, asmjit::x86::ptr(asmjit::x86::r15));
        a.add(asmjit::x86::r15, sizeof(uint64_t));

        // Set DS pointer (r15) to the new data stack pointer value
        a.comment(" ; Store new DS pointer to r15");
        a.mov(asmjit::x86::r15, newDsPointer);
    }


    static void genRPStore() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_rpStore: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_rpStore");

        asmjit::x86::Gp newRsPointer = asmjit::x86::r8; // Temporary register for new RS pointer

        // Pop new return stack pointer value from the data stack
        a.comment(" ; Pop new RS pointer from DS");
        a.mov(newRsPointer, asmjit::x86::ptr(asmjit::x86::r15));
        a.add(asmjit::x86::r15, sizeof(uint64_t));

        // Set RS pointer (r14) to the new return stack pointer value
        a.comment(" ; Store new RS pointer to r14");
        a.mov(asmjit::x86::r14, newRsPointer);
    }

    // store and fetch from memory


    // Generates the Forth @ (fetch) operation
    static void genAT() {
        if (!jc.assembler) {
            throw std::runtime_error("genFetch: Assembler not initialized");
        }

        // Fetch the value at the address and push it onto the data stack
        loadFromDS();
    }

    static void genStore() {
        if (!jc.assembler) {
            throw std::runtime_error("genStore: Assembler not initialized");
        }

        // Store the value from the data stack into the specified address
        storeFromDS();
    }


    static void genDo() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_push_long: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_do");
        a.nop();
        asmjit::x86::Gp currentIndex = asmjit::x86::rdx; // Current index
        asmjit::x86::Gp limit = asmjit::x86::rcx; // Limit

        // Pop current index and limit from the data stack
        popDS(currentIndex);
        popDS(limit);

        // Push current index and limit onto the return stack
        pushRS(limit);
        pushRS(currentIndex);

        // Increment the DO loop depth counter
        doLoopDepth++;

        // Create labels for loop start and end
        a.nop();
        a.comment(" ; ----- DO label");

        DoLoopLabel doLoopLabel;
        doLoopLabel.doLabel = a.newLabel();
        doLoopLabel.loopLabel = a.newLabel();
        doLoopLabel.leaveLabel = a.newLabel();
        doLoopLabel.hasLeave = false;
        a.bind(doLoopLabel.doLabel);

        // Create a LoopLabel struct and push it onto the unified loopStack
        LoopLabel loopLabel;
        loopLabel.type = LoopType::DO_LOOP;
        loopLabel.label = doLoopLabel;

        loopStack.push(loopLabel);
    }


    static void genLoop() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_loop: Assembler not initialized");
        }

        // check if loopStack is empty
        if (loopStack.empty())
            throw std::runtime_error("gen_loop: loopStack is empty");

        const auto loopLabelVariant = loopStack.top();
        loopStack.pop(); // We are at the end of the loop.

        if (loopLabelVariant.type != LoopType::DO_LOOP)
            throw std::runtime_error("gen_loop: Current loop is not a DO loop");

        const auto &loopLabel = std::get<DoLoopLabel>(loopLabelVariant.label);

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_loop");
        a.nop();

        genLeaveLoopOnEscapeKey(a, loopLabel);

        asmjit::x86::Gp currentIndex = asmjit::x86::rcx; // Current index
        asmjit::x86::Gp limit = asmjit::x86::rdx; // Limit
        a.nop(); // no-op

        a.comment(" ; Pop current index and limit from return stack");
        popRS(currentIndex);
        popRS(limit);
        a.comment(" ; Pop limit back on return stack");
        pushRS(limit);

        a.comment(" ; Increment current index by 1");
        a.add(currentIndex, 1);

        // Push the updated index back onto RS
        a.comment(" ; Push current index back to RS");
        pushRS(currentIndex);

        // Check if current index is less than limit
        a.cmp(currentIndex, limit);

        a.comment(" ; Jump to loop start if still looping.");
        // Jump to loop start if current index is less than the limit

        a.jl(loopLabel.doLabel);

        a.comment(" ; ----- LEAVE and loop label");
        a.bind(loopLabel.loopLabel);
        a.bind(loopLabel.leaveLabel);

        // Drop the current index and limit from the return stack
        a.comment(" ; ----- drop loop counters");
        popRS(currentIndex);
        popRS(limit);
        a.nop(); // no-op

        // Decrement the DO loop depth counter
        doLoopDepth--;
    }

    static void genPlusLoop() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_plus_loop: Assembler not initialized");
        }

        // check if loopStack is empty
        if (loopStack.empty())
            throw std::runtime_error("gen_plus_loop: loopStack is empty");

        const auto loopLabelVariant = loopStack.top();
        loopStack.pop(); // We are at the end of the loop.

        if (loopLabelVariant.type != LoopType::DO_LOOP)
            throw std::runtime_error("gen_plus_loop: Current loop is not a DO loop");

        const auto &loopLabel = std::get<DoLoopLabel>(loopLabelVariant.label);

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_plus_loop");

        asmjit::x86::Gp currentIndex = asmjit::x86::rcx; // Current index
        asmjit::x86::Gp limit = asmjit::x86::rdx; // Limit
        asmjit::x86::Gp increment = asmjit::x86::rsi; // Increment value

        genLeaveLoopOnEscapeKey(a, loopLabel);
        a.nop(); // no-op

        // Pop current index and limit from return stack
        a.comment(" ; Pop current index and limit from return stack");
        popRS(currentIndex);
        popRS(limit);
        a.comment(" ; Pop limit back on return stack");
        pushRS(limit);

        // Pop the increment value from data stack
        a.comment(" ; Pop the increment value from data stack");
        popDS(increment);

        // Add increment to current index
        a.comment(" ; Add increment to current index");
        a.add(currentIndex, increment);

        // Push the updated index back onto RS
        a.comment(" ; Push current index back to RS");
        pushRS(currentIndex);

        // Check if current index is less than limit for positive increment
        // or greater than limit for negative increment
        a.comment(" ; Check loop condition based on increment direction");
        a.cmp(increment, 0);
        asmjit::Label positiveIncrement = a.newLabel();
        asmjit::Label loopEnd = a.newLabel();
        a.jg(positiveIncrement);

        // Negative increment
        a.cmp(currentIndex, limit);
        a.jge(loopLabel.doLabel); // Jump if currentIndex >= limit for negative increment
        a.jmp(loopEnd); // Skip positive increment check

        a.comment(" ; ----- Positive increment");
        a.bind(positiveIncrement);
        a.cmp(currentIndex, limit);
        a.jl(loopLabel.doLabel); // Jump if currentIndex < limit for positive increment

        a.comment(" ; ----- LOOP END");
        a.bind(loopEnd);

        a.comment(" ; ----- LEAVE and loop label");
        a.bind(loopLabel.loopLabel);
        a.bind(loopLabel.leaveLabel);

        // Drop the current index and limit from the return stack
        a.comment(" ; ----- drop loop counters");
        popRS(currentIndex);
        popRS(limit);
        a.nop(); // no-op

        // Decrement the DO loop depth counter
        doLoopDepth--;
    }


    static void genI() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_I: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_I");

        // Check if there is at least one loop counter on the unified stack
        if (doLoopDepth == 0) {
            throw std::runtime_error("gen_I: No matching DO_LOOP structure on the stack");
        }

        // Temporary register to hold the current index
        asmjit::x86::Gp currentIndex = asmjit::x86::rcx;

        // Load the innermost loop index (top of the RS) into currentIndex
        a.comment(" ; Copy top of RS to currentIndex");
        a.mov(currentIndex, asmjit::x86::ptr(asmjit::x86::r14)); // Assuming r14 is used for the RS

        // Push currentIndex onto DS
        a.comment(" ; Push currentIndex onto DS");
        pushDS(currentIndex);
    }


    static void genJ() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_j: Assembler not initialized");
        }

        if (doLoopDepth < 2) {
            throw std::runtime_error("gen_j: Not enough nested DO-loops available");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_j");

        asmjit::x86::Gp indexReg = asmjit::x86::rax;

        // Read `J` (which is at depth - 2)
        a.comment(" ; Load index of outer loop (depth - 2) into indexReg");
        a.mov(indexReg, asmjit::x86::ptr(asmjit::x86::r14, 3 * 8)); // Offset for depth - 2 for index

        // Push indexReg onto DS
        a.comment(" ; Push indexReg onto DS");
        pushDS(indexReg);
    }


    static void genK() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_k: Assembler not initialized");
        }

        if (doLoopDepth < 3) {
            throw std::runtime_error("gen_k: Not enough nested DO-loops available");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_k");

        asmjit::x86::Gp indexReg = asmjit::x86::rax;

        a.comment(" ; Load index of outermost loop (depth - 3) into indexReg");
        a.mov(indexReg, asmjit::x86::ptr(asmjit::x86::r14, 5 * 8));

        // Push indexReg onto DS
        a.comment(" ; Push indexReg onto DS");
        pushDS(indexReg);
    }


    static void genLeave() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_leave: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_leave");
        a.nop();

        if (loopStack.empty()) {
            throw std::runtime_error("gen_leave: No loop to leave from");
        }

        // Save current state of loop stack to temp stack
        saveStackToTemp();

        bool found = false;
        asmjit::Label targetLabel;

        std::stack<LoopLabel> workingStack = tempLoopStack;

        // Search for the appropriate leave label in the temporary stack
        while (!workingStack.empty()) {
            LoopLabel topLabel = workingStack.top();
            workingStack.pop();

            switch (topLabel.type) {
                case DO_LOOP: {
                    const auto &loopLabel = std::get<DoLoopLabel>(topLabel.label);
                    targetLabel = loopLabel.leaveLabel;
                    found = true;
                    a.comment(" ; Jumps to do loop's leave label");
                    break;
                }
                case BEGIN_AGAIN_REPEAT_UNTIL: {
                    const auto &loopLabel = std::get<BeginAgainRepeatUntilLabel>(topLabel.label);
                    targetLabel = loopLabel.leaveLabel;
                    found = true;
                    a.comment(" ; Jumps to begin/again/repeat/until leave label");
                    break;
                }
                default:
                    // Continue to look for the correct label
                    break;
            }

            if (found) {
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("gen_leave: No valid loop label found");
        }

        // Reconstitute the temporary stack back into the loopStack
        restoreStackFromTemp();

        // Jump to the found leave label
        a.jmp(targetLabel);
    }


    static void genBegin() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_begin: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_begin");
        a.nop();

        BeginAgainRepeatUntilLabel beginLabel;
        // Create all possible labels here.
        beginLabel.beginLabel = a.newLabel();
        beginLabel.untilLabel = a.newLabel();
        beginLabel.againLabel = a.newLabel();
        beginLabel.whileLabel = a.newLabel();
        beginLabel.leaveLabel = a.newLabel();

        a.comment(" ; LABEL for BEGIN");
        a.bind(beginLabel.beginLabel);

        // Push the new label struct onto the unified stack
        loopStack.push({BEGIN_AGAIN_REPEAT_UNTIL, beginLabel});
    }


    static void genLeaveAgainOnEscapeKey(asmjit::x86::Assembler &a, const BeginAgainRepeatUntilLabel &beginLabels) {
        // optionally generate code to check for escape key pressed
        // if (jc.optLoopCheck) {
        //     // if escape key pressed leave loop.
        //     // shadow stack
        //     a.comment("; -- check for escape key and leave if pressed");
        //     a.push(asmjit::x86::rax);
        //     a.sub(asmjit::x86::rsp, 8);
        //     a.call(escapePressed);
        //     a.add(asmjit::x86::rsp, 8);
        //     // compare rax with 0
        //     a.cmp(asmjit::x86::rax, 0);
        //     a.pop(asmjit::x86::rax);
        //     a.comment("; jump to leave label");
        //     a.jne(beginLabels.leaveLabel);
        // }
    }


    static void genLeaveLoopOnEscapeKey(asmjit::x86::Assembler &a, const DoLoopLabel &l) {
        // // optionally generate code to check for escape key pressed
        // if (jc.optLoopCheck) {
        //     // if escape key pressed leave loop.
        //     // shadow stack
        //     a.comment("; -- check for escape key and leave if pressed");
        //     a.push(asmjit::x86::rax);
        //     a.sub(asmjit::x86::rsp, 8);
        //     a.call(escapePressed);
        //     a.add(asmjit::x86::rsp, 8);
        //     // compare rax with 0
        //     a.cmp(asmjit::x86::rax, 0);
        //     a.pop(asmjit::x86::rax);
        //     a.comment("; jump to leave label");
        //     a.jne(l.leaveLabel);
        // }
    }


    static void genAgain() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_again: Assembler not initialized");
        }

        if (loopStack.empty() || loopStack.top().type != BEGIN_AGAIN_REPEAT_UNTIL) {
            throw std::runtime_error("gen_again: No matching BEGIN_AGAIN_REPEAT_UNTIL structure on the stack");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_again");
        a.nop();

        auto beginLabels = std::get<BeginAgainRepeatUntilLabel>(loopStack.top().label);
        loopStack.pop();

        genLeaveAgainOnEscapeKey(a, beginLabels);
        beginLabels.againLabel = a.newLabel();
        a.jmp(beginLabels.beginLabel);

        a.nop();
        a.comment("LABEL for AGAIN");
        a.bind(beginLabels.againLabel);
        a.nop();
        a.comment("LABEL for LEAVE");
        a.bind(beginLabels.leaveLabel);

        a.nop();
        a.comment("LABEL for WHILE");
        a.bind(beginLabels.whileLabel);
    }


    static void genRepeat() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_repeat: Assembler not initialized");
        }

        if (loopStack.empty() || loopStack.top().type != BEGIN_AGAIN_REPEAT_UNTIL) {
            throw std::runtime_error("gen_repeat: No matching BEGIN_AGAIN_REPEAT_UNTIL structure on the stack");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_repeat");
        a.nop();

        auto beginLabels = std::get<BeginAgainRepeatUntilLabel>(loopStack.top().label);
        loopStack.pop();

        genLeaveAgainOnEscapeKey(a, beginLabels);
        beginLabels.repeatLabel = a.newLabel();
        a.jmp(beginLabels.beginLabel);
        a.bind(beginLabels.repeatLabel);
        a.nop();
        a.comment("LABEL for LEAVE");
        a.bind(beginLabels.leaveLabel);
        a.comment("LABEL for UNTIL");
        a.nop();
        a.bind(beginLabels.whileLabel);
    }


    static void genUntil() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_until: Assembler not initialized");
        }

        if (loopStack.empty() || loopStack.top().type != BEGIN_AGAIN_REPEAT_UNTIL) {
            throw std::runtime_error("gen_until: No matching BEGIN_AGAIN_REPEAT_UNTIL structure on the stack");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_until");
        a.nop();

        // Get the label from the unified stack
        const auto &beginLabels = std::get<BeginAgainRepeatUntilLabel>(loopStack.top().label);

        asmjit::x86::Gp topOfStack = asmjit::x86::rax;
        popDS(topOfStack);
        genLeaveAgainOnEscapeKey(a, beginLabels);
        a.comment(" ; Jump back to beginLabel if top of stack is zero");
        a.test(topOfStack, topOfStack);
        a.jz(beginLabels.beginLabel);

        a.comment("LABEL for UNTIL");
        // Bind the appropriate labels
        a.bind(beginLabels.untilLabel);
        a.nop();
        a.comment("LABEL for LEAVE");
        a.bind(beginLabels.leaveLabel);

        // Pop the stack element as we're done with this construct
        loopStack.pop();
    }


    static void genWhile() {
        if (!jc.assembler) {
            throw std::runtime_error("gen_while: Assembler not initialized");
        }

        if (loopStack.empty() || loopStack.top().type != BEGIN_AGAIN_REPEAT_UNTIL) {
            throw std::runtime_error("gen_while: No matching BEGIN_AGAIN_REPEAT_UNTIL structure on the stack");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_while");
        a.nop();

        auto beginLabel = std::get<BeginAgainRepeatUntilLabel>(loopStack.top().label);
        asmjit::x86::Gp topOfStack = asmjit::x86::rax;
        popDS(topOfStack);

        a.comment(" ; Conditional jump to whileLabel if top of stack is zero");
        a.test(topOfStack, topOfStack);
        a.comment("Jump to WHILE if zero");
        a.jz(beginLabel.whileLabel);
        a.nop();
    }


    // recursion
    static void genRecurse() {
        if (!jc.assembler) {
            throw std::runtime_error("genRecurse: Assembler not initialized");
        }

        auto &a = *jc.assembler;

        // Look for the current function's entry label on the loop stack
        if (!loopStack.empty() && loopStack.top().type == FUNCTION_ENTRY_EXIT) {
            auto functionLabels = std::get<FunctionEntryExitLabel>(loopStack.top().label);

            a.comment(" ; ----- gen_recurse");
            a.nop();

            // Generate a call to the entry label (self-recursion)
            a.call(functionLabels.entryLabel);
        } else {
            throw std::runtime_error("genRecurse: No matching FUNCTION_ENTRY_EXIT structure on the stack");
        }
    }

    static void genIf() {
        if (!jc.assembler) {
            throw std::runtime_error("genIf: Assembler not initialized");
        }

        auto &a = *jc.assembler;

        IfThenElseLabel branches;
        branches.ifLabel = a.newLabel();
        branches.elseLabel = a.newLabel();
        branches.thenLabel = a.newLabel();
        branches.leaveLabel = a.newLabel();
        branches.exitLabel = a.newLabel();
        branches.hasElse = false;
        branches.hasLeave = false;
        branches.hasExit = false;

        // Push the new IfThenElseLabel structure onto the unified loopStack
        loopStack.push({IF_THEN_ELSE, branches});

        a.comment(" ; ----- gen_if");
        a.nop();

        // Pop the condition flag from the data stack
        asmjit::x86::Gp flag = asmjit::x86::rax;
        popDS(flag);

        // Conditional jump to either the ELSE or THEN location
        a.test(flag, flag);
        a.jz(branches.ifLabel);
    }


    static void genElse() {
        if (!jc.assembler) {
            throw std::runtime_error("genElse: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen_else");
        a.nop();

        if (!loopStack.empty() && loopStack.top().type == IF_THEN_ELSE) {
            auto branches = std::get<IfThenElseLabel>(loopStack.top().label);
            a.comment(" ; jump past else block");
            a.jmp(branches.elseLabel); // Jump to the code after the ELSE block
            a.comment(" ; ----- label for ELSE");
            a.bind(branches.ifLabel);
            branches.hasElse = true;

            // Update the stack with the modified branches
            loopStack.pop();
            loopStack.push({IF_THEN_ELSE, branches});
        } else {
            throw std::runtime_error("genElse: No matching IF_THEN_ELSE structure on the stack");
        }
    }

    static void genThen() {
        if (!jc.assembler) {
            throw std::runtime_error("genThen: Assembler not initialized");
        }

        auto &a = *jc.assembler;

        if (!loopStack.empty() && loopStack.top().type == IF_THEN_ELSE) {
            auto branches = std::get<IfThenElseLabel>(loopStack.top().label);
            if (branches.hasElse) {
                a.bind(branches.elseLabel); // Bind the ELSE label
            } else if (branches.hasLeave) {
                a.bind(branches.leaveLabel); // Bind the leave label
            } else if (branches.hasExit) {
                a.bind(branches.exitLabel); // Bind the exit label
            } else {
                a.bind(branches.ifLabel);
            }
            loopStack.pop();
        } else {
            throw std::runtime_error("genThen: No matching IF_THEN_ELSE structure on the stack");
        }
    }

    // implement the case statement
    // case of endof endcase
    //
    //  Start
    //          |
    //         CASE
    //          |
    //          v
    //     - Compare value with 1 -----
    //    |                               |
    //    v                               v
    //   OF 1            -----> Jump to next OF (if false)
    //    |  (if true)
    //    v
    //  Execute block
    //    |
    //   ENDOF ------------------> End of CASE
    //    |                           (jump here if true)
    //    v
    //     - Compare value with 2 -----
    //    |                               |
    //    v                               v
    //   OF 2            -----> Jump to next OF (if false)
    //    |  (if true)
    //    v
    //  Execute block
    //    |
    //   ENDOF ------------------> End of CASE
    //    |                           (jump here if true)
    //    v
    //     - Compare value with 3 -----
    //    |                               |
    //    v                               v
    //   OF 3            -----> Jump to default (if false)
    //    |  (if true)
    //    v
    //  Execute block
    //    |
    //   ENDOF ------------------> End of CASE
    //    |                           (jump here if true)
    //    v
    //   DEFAULT case block (no comparison)
    //    |
    // ENDCASE (marks the end of CASE structure, all jumps converge here)
    //    |
    //    v
    //   End
    //

    // case of endof endcase Implementation

    static void genCase() {
        if (!jc.assembler) {
            throw std::runtime_error("genCase: Assembler not initialized");
        }
        auto &a = *jc.assembler;

        CaseLabel branches;
        branches.end_case_label = a.newLabel();

        branches.ofCount = -1;
        loopStack.push({LoopType::CASE_CONTROL, branches});

        asmjit::x86::Gp value = asmjit::x86::rax;
        popDS(value);
        pushRS(value);

        a.comment(" ; ---- genCase");
    }

    static void genOf() {
        if (!jc.assembler) {
            throw std::runtime_error("genOf: Assembler not initialized");
        }
        auto &a = *jc.assembler;

        if (!loopStack.empty() && loopStack.top().type == LoopType::CASE_CONTROL) {
            auto &branches = std::get<CaseLabel>(loopStack.top().label);

            a.comment(" ; ---- genOf");

            // Create a new endOfLabel for this specific `OF`
            asmjit::Label endOfLabel = a.newLabel();
            branches.ofCount = branches.ofCount + 1; // work on next branches label
            branches.endOfLabels.push_back(endOfLabel);
            // Save modifications back to the stack
            loopStack.pop();
            loopStack.push({LoopType::CASE_CONTROL, branches});

            a.comment(" ; compare and jump to endof if false");
            asmjit::x86::Gp value = asmjit::x86::rax;
            popRS(value);
            pushRS(value);

            asmjit::x86::Gp tos = asmjit::x86::rbx;
            popDS(tos);

            a.cmp(tos, value);
            a.jnz(branches.endOfLabels.at(branches.ofCount)); // Jump to OUR endOfLabel if comparison fails
        } else {
            throw std::runtime_error("genOf: No matching CASE_CONTROL structure on the stack");
        }
    }

    static void genEndOf() {
        if (!jc.assembler) {
            throw std::runtime_error("genEndOf: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        a.comment(" ; ---- genEndOf");

        if (!loopStack.empty() && loopStack.top().type == LoopType::CASE_CONTROL) {
            auto branches = std::get<CaseLabel>(loopStack.top().label);
            a.comment("; jump to endcase");
            // Jump to the end of the case block (final label) after completing the block
            a.jmp(branches.end_case_label);

            // after the jump to endcase.
            // Bind the endOfLabel for this OF block
            if (!branches.endOfLabels.empty()) {
                a.comment("; -- Label for endof ");
                a.bind(branches.endOfLabels.at(branches.ofCount));
                printf("bind success, of label %d", branches.ofCount);
            }
        } else {
            throw std::runtime_error("genEndOf: No matching CASE_CONTROL structure on the stack");
        }
    }

    static void genDefault() {
        if (!jc.assembler) {
            throw std::runtime_error("genDefault: Assembler not initialized");
        }
        auto &a = *jc.assembler;

        if (!loopStack.empty() && loopStack.top().type == LoopType::CASE_CONTROL) {
            auto branches = std::get<CaseLabel>(loopStack.top().label);

            a.comment(" ; ---- genDefault");


            // Save modifications back to the stack
            loopStack.pop();
            loopStack.push({LoopType::CASE_CONTROL, branches});
        } else {
            throw std::runtime_error("genDefault: No matching CASE_CONTROL structure on the stack");
        }
    }

    static void genEndCase() {
        if (!jc.assembler) {
            throw std::runtime_error("genEndCase: Assembler not initialized");
        }
        auto &a = *jc.assembler;

        if (!loopStack.empty() && loopStack.top().type == LoopType::CASE_CONTROL) {
            auto branches = std::get<CaseLabel>(loopStack.top().label);

            a.comment(" ; ---- genEndCase");

            // Final bind at the exit point of the case block
            a.bind(branches.end_case_label);

            // Clear the loop stack
            loopStack.pop();
            // drop case comparison from return stack.
            a.comment(" ; ----  drop case comparison from return stack.");
            asmjit::x86::Gp value = asmjit::x86::rax;
            popRS(value);
        } else {
            throw std::runtime_error("genEndCase: No matching CASE_CONTROL structure on the stack");
        }
    }


    // end of case statements


    static void genSub() {
        if (!jc.assembler) {
            throw std::runtime_error("genSub: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genSub");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;

        // Pop two values from the stack, subtract the first from the second, and push the result

        a.comment(" ; Pop two values from the stack");
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.sub(asmjit::x86::qword_ptr(ds), firstVal); // Subtract first value from second value
    }


    static void genPlus() {
        if (!jc.assembler) {
            throw std::runtime_error("genPlus: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genPlus");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Add two values from the stack");
        popDS(firstVal);
        popDS(secondVal);
        a.add(secondVal, firstVal); // Add first value to second value
        pushDS(secondVal);
    }

    static void genDiv() {
        if (!jc.assembler) {
            throw std::runtime_error("genDiv: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genDiv");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp dividend = asmjit::x86::rax;
        asmjit::x86::Gp divisor = asmjit::x86::rcx;

        // Pop two values from the stack, divide the first by the second, and push the quotient

        a.comment(" ; Perform / division");
        a.mov(divisor, asmjit::x86::qword_ptr(ds)); // Load the second value (divisor)
        a.add(ds, 8); // Adjust stack pointer

        a.mov(dividend, asmjit::x86::qword_ptr(ds)); // Load the first value (dividend)
        a.add(ds, 8); // Adjust stack pointer

        a.mov(asmjit::x86::rdx, 0); // Clear RDX for unsigned division
        // RAX already contains the dividend (first value)

        a.idiv(divisor); // Perform signed division: RDX:RAX / divisor

        a.sub(ds, 8); // Adjust stack pointer back
        a.mov(asmjit::x86::qword_ptr(ds), asmjit::x86::rax); // Store quotient back on stack
    }

    static void genMul() {
        if (!jc.assembler) {
            throw std::runtime_error("genMul: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genMul");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rdx;

        // Pop two values from the stack, multiply them, and push the result

        a.comment(" ; * -multiply two values from the stack");
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.mov(secondVal, asmjit::x86::qword_ptr(ds)); // Load second value

        a.imul(firstVal, secondVal); // Multiply first and second value, result in firstVal

        a.comment(" ; Push the result onto the stack");
        a.mov(asmjit::x86::qword_ptr(ds), firstVal); // Store result back on stack
    }

    static void genMod() {
        if (!jc.assembler) {
            throw std::runtime_error("genMod: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genMod");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp dividend = asmjit::x86::rax;
        asmjit::x86::Gp divisor = asmjit::x86::rcx;

        // Pop two values from the stack, compute the remainder, and push the result
        a.comment(" ; Perform % modulus");
        a.mov(divisor, asmjit::x86::qword_ptr(ds)); // Load the second value (divisor)
        a.add(ds, 8); // Adjust stack pointer

        a.mov(dividend, asmjit::x86::qword_ptr(ds)); // Load the first value (dividend)
        a.add(ds, 8); // Adjust stack pointer

        a.mov(asmjit::x86::rdx, 0); // Clear RDX for unsigned division
        // RAX already contains the dividend (first value)

        a.idiv(divisor); // Perform signed division: RDX:RAX / divisor

        a.sub(ds, 8); // Adjust stack pointer back
        a.mov(asmjit::x86::qword_ptr(ds), asmjit::x86::rdx); // Store the remainder (RDX) back on stack
    }

    // Error handler for division by zero
    static void divide_by_zero() {
        std::cerr << "Division by zero error in */MOD word." << std::endl;
        throw std::runtime_error("Division by zero error in */MOD word.");
    }


    //
    // static void genStarSlashMod()
    // {
    //     if (!jc.assembler)
    //     {
    //         throw std::runtime_error("genStarSlashMod: Assembler not initialized");
    //     }
    //
    //     auto& a = *jc.assembler;
    //     a.comment(" ; ----- genStarSlashMod");
    //
    //     // Assuming r15 is the stack pointer
    //     asmjit::x86::Gp ds = asmjit::x86::r15;
    //     asmjit::x86::Gp n1 = asmjit::x86::rax;
    //     asmjit::x86::Gp n2 = asmjit::x86::rbx;
    //     asmjit::x86::Gp n3 = asmjit::x86::rcx;
    //     asmjit::x86::Gp remainder = asmjit::x86::rdx; // rdx:rax is used in idiv
    //     asmjit::x86::Gp quotient = asmjit::x86::rax;
    //
    //     asmjit::Label divByZero = a.newLabel();
    //     asmjit::Label proceed = a.newLabel();
    //
    //     // Load n3, n2, and n1 from the stack
    //     a.mov(n3, asmjit::x86::qword_ptr(ds)); // Top of stack is n3
    //     a.add(ds, 8); // Move stack pointer up
    //     a.mov(n2, asmjit::x86::qword_ptr(ds)); // Next value is n2
    //     a.add(ds, 8); // Move stack pointer up
    //     a.mov(n1, asmjit::x86::qword_ptr(ds)); // Next value is n1
    //
    //     a.comment(" ; Multiply n1 by n2 producing a double-cell result");
    //
    //     // Perform multiplication n1 * n2 -> rdx:rax
    //     a.imul(n2);
    //
    //     a.comment(" ; Check for division by zero");
    //
    //     // Handle division by zero
    //     a.test(n3, n3);
    //     a.jz(divByZero);
    //
    //     a.comment(" ; Divide double-cell result by n3");
    //
    //     // Divide double-cell result by n3
    //     a.idiv(n3); // quotient = rax, remainder = rdx
    //
    //     a.comment(" ; Push remainder and quotient back onto the stack");
    //
    //     // Push remainder and quotient back to stack
    //     a.sub(ds, 8); // Move stack pointer down
    //     a.mov(asmjit::x86::qword_ptr(ds), quotient); // Push quotient
    //     a.sub(ds, 8); // Move stack pointer down
    //     a.mov(asmjit::x86::qword_ptr(ds), remainder); // Push remainder
    //
    //     a.jmp(proceed);
    //     a.comment("; divide by zero error");
    //     a.bind(divByZero);
    //     a.call(divide_by_zero); // Handle division by zero
    //
    //     a.bind(proceed);
    //
    // }


    static void genNegate() {
        if (!jc.assembler) {
            throw std::runtime_error("genNegate: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genNegate");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp value = asmjit::x86::rax;

        a.comment(" ; Negate the top value of the stack");
        a.mov(value, asmjit::x86::qword_ptr(ds)); // Load the top value
        a.neg(value); // Negate the value
        a.mov(asmjit::x86::qword_ptr(ds), value); // Store the result back on stack
    }


    static void genInvert() {
        if (!jc.assembler) {
            throw std::runtime_error("genInvert: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genInvert");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp value = asmjit::x86::rax;

        a.comment(" ; Invert the top value of the stack");
        a.mov(value, asmjit::x86::qword_ptr(ds)); // Load the top value
        a.not_(value); // Invert the value
        a.mov(asmjit::x86::qword_ptr(ds), value); // Store the result back on stack
    }


    static void genAbs() {
        if (!jc.assembler) {
            throw std::runtime_error("genAbs: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genAbs");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp value = asmjit::x86::rax;

        a.comment(" ; Absolute value of the top value of the stack");
        a.mov(value, asmjit::x86::qword_ptr(ds)); // Load the top value

        // If value is negative, negate it
        a.test(value, value);
        asmjit::Label positive = a.newLabel();
        a.jns(positive);
        a.neg(value);
        a.bind(positive);

        a.mov(asmjit::x86::qword_ptr(ds), value); // Store the result back on stack
    }


    static void genMin() {
        if (!jc.assembler) {
            throw std::runtime_error("genMin: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genMin");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Find the minimum of two values from the stack");

        // Pop two values from the stack
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.mov(secondVal, asmjit::x86::qword_ptr(ds)); // Load second value
        a.add(ds, 8); // Adjust stack pointer

        // Compare and move the smaller value to firstVal
        a.cmp(firstVal, secondVal);
        a.cmovg(firstVal, secondVal);

        a.sub(ds, 8); // Adjust stack pointer back
        a.mov(asmjit::x86::qword_ptr(ds), firstVal); // Store result back on stack
    }


    static void genMax() {
        if (!jc.assembler) {
            throw std::runtime_error("genMax: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genMax");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Find the maximum of two values from the stack");

        // Pop two values from the stack
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.mov(secondVal, asmjit::x86::qword_ptr(ds)); // Load second value
        a.add(ds, 8); // Adjust stack pointer

        // Compare and move the larger value to firstVal
        a.cmp(firstVal, secondVal);
        a.cmovl(firstVal, secondVal);

        a.sub(ds, 8); // Adjust stack pointer back
        a.mov(asmjit::x86::qword_ptr(ds), firstVal); // Store result back on stack
    }


    static void genWithin() {
        if (!jc.assembler) {
            throw std::runtime_error("genWithin: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genWithin");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp value = asmjit::x86::rax;
        asmjit::x86::Gp lower_bound = asmjit::x86::rbx;
        asmjit::x86::Gp upper_bound = asmjit::x86::rcx;
        asmjit::x86::Gp result = asmjit::x86::rdx;

        asmjit::Label out = a.newLabel();

        // Load the upper bound, lower bound, and value from the stack
        a.mov(upper_bound, asmjit::x86::qword_ptr(ds)); // Top of stack is upper_bound
        a.add(ds, 8); // Move stack pointer up
        a.mov(lower_bound, asmjit::x86::qword_ptr(ds)); // Next value is lower_bound
        a.add(ds, 8); // Move stack pointer up
        a.mov(value, asmjit::x86::qword_ptr(ds)); // Next value is value

        a.comment(" ; Check if value is within the bounds");

        // Set result to 0 initially
        a.xor_(result, result); // result = 0

        a.cmp(value, lower_bound); // compare value with lower bound
        a.jl(out); // if value < lower_bound, jump to out

        a.cmp(value, upper_bound); // compare value with upper bound
        a.jge(out); // if value >= upper_bound, jump to out

        a.mov(result, -1); // result = -1 (value is within the range)

        a.bind(out);
        // Store the result back on the stack
        a.mov(asmjit::x86::qword_ptr(ds), result);
    }

    static void genIntSqrt() {
        if (!jc.assembler) {
            throw std::runtime_error("genSqrt: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genSqrt");

        // Declaring labels for control flow
        asmjit::Label startLoop = a.newLabel();
        asmjit::Label loopCheck = a.newLabel();
        asmjit::Label continueLabel = a.newLabel();
        asmjit::Label done = a.newLabel();

        // Using registers for q, r, t, and tracking x
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp x = asmjit::x86::rax; // Initial input
        asmjit::x86::Gp q = asmjit::x86::rbx; // Iterator q
        asmjit::x86::Gp r = asmjit::x86::rcx; // Result accumulation
        asmjit::x86::Gp t = asmjit::x86::rdx; // Temporary storage for calculations

        a.comment(" ; Load x from stack and initialize q and r");
        a.mov(x, asmjit::x86::qword_ptr(ds)); // Load x from the stack
        a.mov(q, 1); // Initialize q to 1
        a.xor_(r, r); // Clear r

        a.comment(" ; Move q to maximum valid shift");
        a.bind(startLoop);
        a.cmp(q, x); // Compare q to x
        a.jg(loopCheck); // Jump if q is greater than x
        a.shl(q, 2); // Shift q left by 2 bits
        a.jmp(startLoop); // Repeat loop

        a.bind(loopCheck);
        a.cmp(q, 1); // If q <= 1, break out of loop
        a.jle(done);

        a.comment(" ; Perform Newton-like iteration");
        a.shr(q, 2); // Reduce q by shifting right twice per iteration

        a.mov(t, x); // Calculate t = x - r - q
        a.sub(t, r);
        a.sub(t, q);
        a.shr(r, 1); // Shift r to the right
        a.cmp(t, 0); // Check whether t >= 0
        a.jl(continueLabel); // If t < 0, skip add step

        a.mov(x, t); // New x value after successful subtraction
        a.add(r, q); // Accumulate the result in r

        a.bind(continueLabel);
        a.test(q, q); // Check for loop end condition
        a.jnz(loopCheck); // Repeat if more work to do

        a.bind(done);
        a.comment(" ; Store final result in r back to stack");
        a.mov(asmjit::x86::qword_ptr(ds), r); // Store result r back on stack
    }

    static void genGcd() {
        if (!jc.assembler) {
            throw std::runtime_error("genGcdEuclidean: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genGcdEuclidean");

        // Declare labels for control flow
        asmjit::Label loopStart = a.newLabel();
        asmjit::Label doneLabel = a.newLabel();

        asmjit::x86::Gp ds = asmjit::x86::r15; // Data stack
        asmjit::x86::Gp aValue = asmjit::x86::rax;
        asmjit::x86::Gp bValue = asmjit::x86::rbx;
        asmjit::x86::Gp remainder = asmjit::x86::rdx;

        a.comment(" ; Load initial values from stack into registers");
        a.mov(aValue, asmjit::x86::qword_ptr(ds));
        a.add(ds, 8);
        a.mov(bValue, asmjit::x86::qword_ptr(ds));

        a.comment(" ; Begin Euclidean algorithm loop");
        a.bind(loopStart);
        a.test(bValue, bValue);
        a.jz(doneLabel);

        a.xor_(remainder, remainder); // Clear remainder before division
        a.div(bValue);
        a.mov(remainder, asmjit::x86::rdx); // rdx contains the remainder

        a.mov(aValue, bValue); // Swap aValue and bValue for the next iteration
        a.mov(bValue, remainder); // bValue gets the remainder

        a.jmp(loopStart); // Repeat the loop

        a.bind(doneLabel);
        a.comment(" ; Store the GCD result back on stack");
        a.sub(ds, 8);
        a.mov(asmjit::x86::qword_ptr(ds), aValue);

        a.comment(" ; End of Euclidean GCD implementation");
    }

    // comparisons

    static void genZeroEquals() {
        if (!jc.assembler) {
            throw std::runtime_error("genZeroEquals: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genZeroEquals");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp value = asmjit::x86::rax;
        asmjit::x86::Gp flag = asmjit::x86::rbx;

        a.comment(" ; Check if the top value of the stack is zero");
        a.mov(value, asmjit::x86::qword_ptr(ds)); // Load the top value
        a.test(value, value); // Test if value is zero
        a.setz(asmjit::x86::bl); // Set flag (bl) to 1 if zero, otherwise 0

        a.comment(" ; Convert the flag to Forth truth values (-1 or 0)");
        a.neg(asmjit::x86::bl); // Negate the flag value to make 1 -> -1 (true), and 0 stays 0 (false)
        a.movsx(flag, asmjit::x86::bl); // Sign-extend bl to the full width of flag (rbx)
        a.mov(asmjit::x86::qword_ptr(ds), flag); // Store the result back on stack
    }


    static void genZeroLessThan() {
        if (!jc.assembler) {
            throw std::runtime_error("genZeroLessThan: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genZeroLessThan");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp value = asmjit::x86::rax;
        asmjit::x86::Gp flag = asmjit::x86::rbx;

        a.comment(" ; Check if the top value of the stack is less than zero");
        a.mov(value, asmjit::x86::qword_ptr(ds)); // Load the top value
        a.test(value, value); // Test if value is less than zero
        a.setl(asmjit::x86::bl); // Set flag (bl) to 1 if less than zero, otherwise 0

        a.comment(" ; Convert the flag to Forth truth values (-1 or 0)");
        a.neg(asmjit::x86::bl); // Negate the flag value to make 1 -> -1 (true), and 0 stays 0 (false)
        a.movsx(flag, asmjit::x86::bl); // Sign-extend bl to the full width of flag (rbx)
        a.mov(asmjit::x86::qword_ptr(ds), flag); // Store the result back on stack
    }


    static void genZeroGreaterThan() {
        if (!jc.assembler) {
            throw std::runtime_error("genZeroGreaterThan: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genZeroGreaterThan");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp value = asmjit::x86::rax;
        asmjit::x86::Gp flag = asmjit::x86::rbx;

        a.comment(" ; Check if the top value of the stack is greater than zero");
        a.mov(value, asmjit::x86::qword_ptr(ds)); // Load the top value
        a.test(value, value); // Test if value is greater than zero
        a.setg(asmjit::x86::bl); // Set flag (bl) to 1 if greater than zero, otherwise 0

        a.comment(" ; Convert the flag to Forth truth values (-1 or 0)");
        a.neg(asmjit::x86::bl); // Negate the flag value to make 1 -> -1 (true), and 0 stays 0 (false)
        a.movsx(flag, asmjit::x86::bl); // Sign-extend bl to the full width of flag (rbx)
        a.mov(asmjit::x86::qword_ptr(ds), flag); // Store the result back on stack
    }


    static void genEq() {
        if (!jc.assembler) {
            throw std::runtime_error("genEq: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genEq");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;

        // Pop two values, compare them and push the result (0 or -1)
        a.comment(" ; Pop two values compare them and push the result (0 or -1)");
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.cmp(asmjit::x86::qword_ptr(ds), firstVal); // Compare with second value
        a.sete(asmjit::x86::al); // Set AL to 1 if equal

        a.neg(asmjit::x86::al); // Set AL to -1 if true, 0 if false
        a.movsx(asmjit::x86::rax, asmjit::x86::al); // Sign-extend AL to RAX (-1 or 0)
        a.mov(asmjit::x86::qword_ptr(ds), asmjit::x86::rax); // Store result on stack
    }

    static void genLt() {
        if (!jc.assembler) {
            throw std::runtime_error("genLt: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genLt");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rdx;

        // Pop two values, compare them and push the result (0 or -1)
        a.comment(" ; < compare them and push the result (0 or -1)");

        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.mov(secondVal, asmjit::x86::qword_ptr(ds)); // Load second value
        a.add(ds, 8); // Adjust stack pointer

        a.cmp(secondVal, firstVal); // Compare second value to first value
        a.setl(asmjit::x86::al); // Set AL to 1 if secondVal < firstVal

        a.movzx(asmjit::x86::rax, asmjit::x86::al); // Zero-extend AL to RAX
        a.neg(asmjit::x86::rax); // Set RAX to -1 if true (1 -> -1), 0 remains 0

        a.sub(ds, 8); // Adjust stack pointer
        a.mov(asmjit::x86::qword_ptr(ds), asmjit::x86::rax); // Store result on stack
    }

    static void genGt() {
        if (!jc.assembler) {
            throw std::runtime_error("genGt: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genGt");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;

        // Pop two values, compare them and push the result (0 or -1)
        a.comment(" ; > compare them and push the result (0 or -1)");
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.cmp(asmjit::x86::qword_ptr(ds), firstVal); // Compare with second value
        a.setg(asmjit::x86::al); // Set AL to 1 if greater

        a.neg(asmjit::x86::al); // Set AL to -1 if true, 0 if false
        a.movsx(asmjit::x86::rax, asmjit::x86::al); // Sign-extend AL to RAX (-1 or 0)
        a.mov(asmjit::x86::qword_ptr(ds), asmjit::x86::rax); // Store result on stack
    }

    static void genNot() {
        if (!jc.assembler) {
            throw std::runtime_error("genNot: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genNot");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;

        // Load the value from the stack
        a.mov(asmjit::x86::rax, asmjit::x86::qword_ptr(ds)); // Load value

        // Use the `NOT` operation to flip the bits
        a.not_(asmjit::x86::rax); // Perform NOT operation

        // Store the result back on the stack
        a.mov(asmjit::x86::qword_ptr(ds), asmjit::x86::rax); // Store result back on stack
    }

    static void genAnd() {
        if (!jc.assembler) {
            throw std::runtime_error("genAnd: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genAnd");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;

        // Pop two values, perform AND, and push the result
        a.comment(" ; AND two values and push the result");
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.and_(firstVal, asmjit::x86::qword_ptr(ds)); // Perform AND with second value

        a.mov(asmjit::x86::qword_ptr(ds), firstVal); // Store result back on stack
    }


    static void genOR() {
        if (!jc.assembler) {
            throw std::runtime_error("genOR: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genOR");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;

        // Pop two values, perform OR, and push the result
        a.comment(" ; OR two values and push the result");
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.or_(firstVal, asmjit::x86::qword_ptr(ds)); // Perform OR with second value

        a.mov(asmjit::x86::qword_ptr(ds), firstVal); // Store result back on stack
    }


    static void genXOR() {
        if (!jc.assembler) {
            throw std::runtime_error("genXOR: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genXOR");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp firstVal = asmjit::x86::rax;

        // Pop two values, perform XOR, and push the result
        a.comment(" ; XOR two values and push the result");
        a.mov(firstVal, asmjit::x86::qword_ptr(ds)); // Load first value
        a.add(ds, 8); // Adjust stack pointer

        a.xor_(firstVal, asmjit::x86::qword_ptr(ds)); // Perform XOR with second value

        a.mov(asmjit::x86::qword_ptr(ds), firstVal); // Store result back on stack
    }

    // stack ops

    static void genDSAT() {
        if (!jc.assembler) {
            throw std::runtime_error("genDSAT: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genDSAT");

        asmjit::x86::Gp ds = asmjit::x86::r15; // stack pointer in r15
        asmjit::x86::Gp tempReg = asmjit::x86::rax; // temporary register to hold the stack pointer value

        // Move the stack pointer to the temporary register
        a.comment(" ; SP@ - Get the stack pointer value");
        a.mov(tempReg, ds);

        // Push the stack pointer value onto the data stack
        a.comment(" ; Push the stack pointer value onto the data stack");
        pushDS(tempReg);

        a.comment(" ; ----- end of genDSAT");
    }

    static void genDrop() {
        if (!jc.assembler) {
            throw std::runtime_error("genDrop: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genDrop");

        // Assuming r15 is the stack pointer
        a.comment(" ; drop top value");
        asmjit::x86::Gp ds = asmjit::x86::r15;
        a.add(ds, 8); // Adjust stack pointer to drop the top value
    }


    static void genDup() {
        if (!jc.assembler) {
            throw std::runtime_error("genDup: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genDup");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp topValue = asmjit::x86::rax;

        // Duplicate the top value on the stack
        a.comment(" ; Duplicate the top value on the stack");
        a.mov(topValue, asmjit::x86::qword_ptr(ds)); // Load top value
        a.sub(ds, 8); // Adjust stack pointer
        a.mov(asmjit::x86::qword_ptr(ds), topValue); // Push duplicated value
    }

    static void genSwap() {
        if (!jc.assembler) {
            throw std::runtime_error("genSwap: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genSwap");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp topValue = asmjit::x86::rax;
        asmjit::x86::Gp secondValue = asmjit::x86::rcx;

        // Swap the top two values on the stack
        a.comment(" ; Swap top two values on the stack");
        a.mov(topValue, asmjit::x86::qword_ptr(ds)); // Load top value
        a.mov(secondValue, asmjit::x86::qword_ptr(ds, 8)); // Load second value
        a.mov(asmjit::x86::qword_ptr(ds), secondValue); // Store second value in place of top value
        a.mov(asmjit::x86::qword_ptr(ds, 8), topValue); // Store top value in place of second value
    }

    static void genRot() {
        if (!jc.assembler) {
            throw std::runtime_error("genRot: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genRot");

        asmjit::x86::Gp ds = asmjit::x86::r15; // stack pointer in r15
        asmjit::x86::Gp topValue = asmjit::x86::rax; // top value in rax
        asmjit::x86::Gp secondValue = asmjit::x86::rcx; // second value in rcx
        asmjit::x86::Gp thirdValue = asmjit::x86::rdx; // third value in rdx

        // Rotate the top three values on the stack
        a.comment(" ; Rotate top three values on the stack");
        a.mov(topValue, asmjit::x86::qword_ptr(ds)); // Load top value (TOS)
        a.mov(secondValue, asmjit::x86::qword_ptr(ds, 8)); // Load second value (NOS)
        a.mov(thirdValue, asmjit::x86::qword_ptr(ds, 16)); // Load third value (TOS+2)
        a.mov(asmjit::x86::qword_ptr(ds), thirdValue);
        // Store third value in place of top value (TOS = TOS+2)
        a.mov(asmjit::x86::qword_ptr(ds, 16), secondValue);
        // Store second value into third position (TOS+2 = NOS)
        a.mov(asmjit::x86::qword_ptr(ds, 8), topValue); // Store top value into second position (NOS = TOS)
        a.comment(" ; ----- end of genRot");
    }

    static void genOver() {
        if (!jc.assembler) {
            throw std::runtime_error("genOver: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genOver");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp secondValue = asmjit::x86::rax;

        // Duplicate the second value on the stack
        a.comment(" ; Duplicate the second value on the stack");
        a.mov(secondValue, asmjit::x86::qword_ptr(ds, 8)); // Load second value
        a.sub(ds, 8); // Adjust stack pointer
        a.mov(asmjit::x86::qword_ptr(ds), secondValue); // Push duplicated value
    }


    static void genTuck() {
        if (!jc.assembler) {
            throw std::runtime_error("genTuck: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genTuck");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp topValue = asmjit::x86::rax;
        asmjit::x86::Gp secondValue = asmjit::x86::rcx;

        // Tuck the top value under the second value on the stack
        a.comment(" ; Tuck the top value under the second value on the stack");
        a.mov(topValue, asmjit::x86::qword_ptr(ds)); // Load top value
        a.mov(secondValue, asmjit::x86::qword_ptr(ds, 8)); // Load second value
        a.sub(ds, 8); // Adjust stack pointer to make space
        a.mov(asmjit::x86::qword_ptr(ds), topValue); // Push top value
        a.mov(asmjit::x86::qword_ptr(ds, 8), secondValue); // Access memory at ds + 8
        a.mov(asmjit::x86::qword_ptr(ds, 16), topValue); // Access memory at ds + 16
    }


    static void genNip() {
        // generate forth NIP stack word
        if (!jc.assembler) {
            throw std::runtime_error("genNip: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genNip");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp topValue = asmjit::x86::rax;

        // NIP's operation is to remove the second item on the stack
        a.comment(" ; Remove the second item from the stack");
        a.mov(topValue, asmjit::x86::qword_ptr(ds)); // Load top value
        a.add(ds, 8); // Adjust stack pointer to skip the second item
        a.mov(asmjit::x86::qword_ptr(ds, 0), topValue); // Move top value to the new top position
    }

    static void genPick(int n) {
        if (!jc.assembler) {
            throw std::runtime_error("genPick: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genPick");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;
        asmjit::x86::Gp value = asmjit::x86::rax;

        // Pick the nth value from the stack
        a.comment(" ; Pick the nth value from the stack");

        // Calculate the address offset for the nth element
        int offset = n * 8;

        a.mov(value, asmjit::x86::qword_ptr(ds, offset)); // Load the nth value
        a.sub(ds, 8); // Adjust stack pointer for pushing value
        a.mov(asmjit::x86::qword_ptr(ds), value); // Push the picked value onto the stack
    }


    // Helper function to generate code for pushing constants onto the stack

    static void genPushConstant(int64_t value) {
        auto &a = *jc.assembler;
        asmjit::x86::Gp ds = asmjit::x86::r15; // Stack pointer register

        if (value >= std::numeric_limits<int32_t>::min() && value <= std::numeric_limits<int32_t>::max()) {
            // Push a 32-bit immediate value (optimized for smaller constants)
            a.sub(ds, 8); // Reserve space on the stack
            a.mov(asmjit::x86::qword_ptr(ds), static_cast<int32_t>(value));
        } else {
            // For larger/negative 64-bit constants
            asmjit::x86::Mem stackMem(ds, -8); // Memory location for pushing the constant
            a.sub(ds, 8); // Reserve space on the stack
            a.mov(stackMem, value); // Move the 64-bit value directly to the reserved space
        }
    }

    // Macros or inline functions to call the helper function with specific values
#define GEN_PUSH_CONSTANT_FN(name, value) \
static void name()                     \
{                                      \
genPushConstant(value);            \
}

    // Define specific constant push functions
    GEN_PUSH_CONSTANT_FN(push1, 1)
    GEN_PUSH_CONSTANT_FN(push2, 2)
    GEN_PUSH_CONSTANT_FN(push3, 3)
    GEN_PUSH_CONSTANT_FN(push4, 4)
    GEN_PUSH_CONSTANT_FN(push8, 8)
    GEN_PUSH_CONSTANT_FN(push16, 16)
    GEN_PUSH_CONSTANT_FN(push32, 32)
    GEN_PUSH_CONSTANT_FN(push64, 64)
    GEN_PUSH_CONSTANT_FN(pushNeg1, -1)
    GEN_PUSH_CONSTANT_FN(SPBASE, sm.getDStop())


    static void gen1Inc() {
        if (!jc.assembler) {
            throw std::runtime_error("gen1inc: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen1inc - use inc instruction");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;

        // Increment the value at the memory location pointed to by r15
        a.inc(asmjit::x86::qword_ptr(ds));
    }


    static void gen1Dec() {
        if (!jc.assembler) {
            throw std::runtime_error("gen1inc: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- gen1inc - use dec instruction");

        // Assuming r15 is the stack pointer
        asmjit::x86::Gp ds = asmjit::x86::r15;

        // Decrement the value at the memory location pointed to by r15
        a.dec(asmjit::x86::qword_ptr(ds));
    }


#define GEN_INC_DEC_FN(name, operation, value) \
static void name()                         \
{                                          \
jc.uint64_A = value;                   \
operation();                           \
}

    // Define specific increment functions
    GEN_INC_DEC_FN(gen2Inc, genPlusLong, 2)
    GEN_INC_DEC_FN(gen16Inc, genPlusLong, 16)
    // Define specific decrement functions

    GEN_INC_DEC_FN(gen2Dec, genSubLong, 2)
    GEN_INC_DEC_FN(gen16Dec, genSubLong, 16)

    static void genMulBy10() {
        auto &a = *jc.assembler;
        asmjit::x86::Gp ds = asmjit::x86::r15; // Stack pointer register
        asmjit::x86::Gp tempValue = asmjit::x86::rax; // Temporary register for value
        asmjit::x86::Gp tempResult = asmjit::x86::rdx; // Temporary register for intermediate result

        a.comment("; multiply by ten");
        // Load the top stack value into tempValue
        popDS(tempValue);

        // Perform the shift left by 3 (Value * 8)
        a.mov(tempResult, tempValue);
        a.shl(tempResult, 3);

        // Perform the shift left by 1 (Value * 2)
        a.shl(tempValue, 1);

        // Add the two shifted values
        a.add(tempResult, tempValue);

        // Store the result back on the stack
        pushDS(tempResult);
    }


    // Helper function for left shifts
    static void genLeftShift(int shiftAmount) {
        auto &a = *jc.assembler;
        asmjit::x86::Gp ds = asmjit::x86::r15; // Stack pointer register
        asmjit::x86::Gp tempValue = asmjit::x86::rax; // Temporary register for value

        // Load the top stack value into tempValue
        a.mov(tempValue, asmjit::x86::qword_ptr(ds));

        // Perform the shift
        a.shl(tempValue, shiftAmount);

        // Store the result back on the stack
        a.mov(asmjit::x86::qword_ptr(ds), tempValue);
    }

    // Helper function for right shifts
    static void genRightShift(int shiftAmount) {
        auto &a = *jc.assembler;
        asmjit::x86::Gp ds = asmjit::x86::r15; // Stack pointer register
        asmjit::x86::Gp tempValue = asmjit::x86::rax; // Temporary register for value

        // Load the top stack value into tempValue
        a.mov(tempValue, asmjit::x86::qword_ptr(ds));

        // Perform the shift
        a.shr(tempValue, shiftAmount);

        // Store the result back on the stack
        a.mov(asmjit::x86::qword_ptr(ds), tempValue);
    }

    // Macros to define the shift operations
#define GEN_SHIFT_FN(name, shiftAction, shiftAmount) \
static void name()                               \
{                                                \
shiftAction(shiftAmount);                    \
}

    // Define specific shift functions for multiplication
    GEN_SHIFT_FN(gen2mul, genLeftShift, 1)
    GEN_SHIFT_FN(gen4mul, genLeftShift, 2)
    GEN_SHIFT_FN(gen8mul, genLeftShift, 3)
    GEN_SHIFT_FN(gen16mul, genLeftShift, 4)

    // Define specific shift functions for division
    GEN_SHIFT_FN(gen2Div, genRightShift, 1)
    GEN_SHIFT_FN(gen4Div, genRightShift, 2)
    GEN_SHIFT_FN(gen8Div, genRightShift, 3)


    //  abort e.g. IF ABORT" error" THEN
    //  neither throw nor longjmp work yet

    // abort with error message
    // static void genImmediateAbortQuote()
    // {
    //     const auto& words = *jc.words;
    //     size_t pos = jc.pos_next_word + 1;
    //     std::string word = words[pos];
    //     jc.word = word;
    //     if (logging)
    //         printf("genImmediateAbortQuote: %s\n", word.c_str());
    //
    //     const auto index = stripIndex(word);
    //     strIntern.incrementRef(index);
    //     auto address = strIntern.getStringAddress(index);
    //
    //     if (!jc.assembler)
    //     {
    //         throw std::runtime_error("genImmediateAbortQuote: Assembler not initialized");
    //     }
    //
    //
    //     auto& a = *jc.assembler;
    //     commentWithWord(" ; ----- ABORT\" displaying text exiting  ");
    //
    //     // Display the string
    //     a.mov(asmjit::x86::rcx, address); // put parameter in argument
    //     a.sub(asmjit::x86::rsp, 8); // Allocate space for the shadow space
    //     a.call(prints); // call puts
    //      a.add(asmjit::x86::rsp, 8); // restore shadow space
    //
    //     // Generate the jump to the function exit label
    //     a.mov(asmjit::x86::rax, callLongjmp);
    //     a.sub(asmjit::x86::rsp, 8); // Allocate space for the shadow space
    //     a.call(asmjit::x86::rax);
    //      // never gets here
    //
    //     jc.pos_last_word = pos;
    // }


    // floating point support


    static void genIntToFloat() {
        if (!jc.assembler) {
            throw std::runtime_error("genIntToFloat: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genIntToFloat");

        asmjit::x86::Gp intVal = asmjit::x86::rax;

        a.comment(" ; Convert integer to floating point");
        popDS(intVal); // Pop integer value from the stack
        a.cvtsi2sd(asmjit::x86::xmm0, intVal); // Convert integer in RAX to double in XMM0
        a.movq(intVal, asmjit::x86::xmm0); // Move the double from XMM0 back to a general-purpose register
        pushDS(intVal); // Push the floating point value back onto the stack
    }


    static void genFloatToInt() {
        if (!jc.assembler) {
            throw std::runtime_error("genFloatToInt: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFloatToInt");

        asmjit::x86::Gp floatVal = asmjit::x86::rax;

        a.comment(" ; Convert floating point to integer");
        popDS(floatVal); // Pop floating point value from the stack
        a.movq(asmjit::x86::xmm0, floatVal); // Move the floating point value to XMM0
        a.cvttsd2si(floatVal, asmjit::x86::xmm0); // Convert floating point in XMM0 to integer in RAX
        pushDS(floatVal); // Push the integer value back onto the stack
    }


    static void genFPlus() {
        if (!jc.assembler) {
            throw std::runtime_error("genFPlus: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFPlus");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Add two floating point values from the stack");
        popDS(firstVal); // Pop the first floating point value
        popDS(secondVal); // Pop the second floating point value
        a.movq(asmjit::x86::xmm0, firstVal); // Move the first value to XMM0
        a.movq(asmjit::x86::xmm1, secondVal); // Move the second value to XMM1
        a.addsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Add the two floating point values
        a.movq(firstVal, asmjit::x86::xmm0); // Move the result back to a general-purpose register
        pushDS(firstVal); // Push the result back onto the stack
    }


    static void genFSub() {
        if (!jc.assembler) {
            throw std::runtime_error("genFSub: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFSub");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Subtract two floating point values from the stack");
        popDS(firstVal); // Pop the first floating point value
        popDS(secondVal); // Pop the second floating point value
        a.movq(asmjit::x86::xmm0, secondVal);
        a.movq(asmjit::x86::xmm1, firstVal);
        a.subsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Subtract the floating point values
        a.movq(firstVal, asmjit::x86::xmm0); // Move the result back to a general-purpose register
        pushDS(firstVal); // Push the result back onto the stack
    }


    static void genFMul() {
        if (!jc.assembler) {
            throw std::runtime_error("genFMul: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFMul");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Multiply two floating point values from the stack");
        popDS(firstVal); // Pop the first floating point value
        popDS(secondVal); // Pop the second floating point value
        a.movq(asmjit::x86::xmm0, firstVal); // Move the first value to XMM0
        a.movq(asmjit::x86::xmm1, secondVal); // Move the second value to XMM1
        a.mulsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Multiply the floating point values
        a.movq(firstVal, asmjit::x86::xmm0); // Move the result back to a general-purpose register
        pushDS(firstVal); // Push the result back onto the stack
    }

    // Floating point division
    static void genFDiv() {
        if (!jc.assembler) {
            throw std::runtime_error("genFDiv: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFDiv");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Divide two floating point values from the stack");
        popDS(firstVal); // Pop the first floating point value
        popDS(secondVal); // Pop the second floating point value
        a.movq(asmjit::x86::xmm0, secondVal);
        a.movq(asmjit::x86::xmm1, firstVal);
        a.divsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Divide the floating point values
        a.movq(firstVal, asmjit::x86::xmm0); // Move the result back to a general-purpose register
        pushDS(firstVal); // Push the result back onto the stack
    }

    static void genFMod() {
        if (!jc.assembler) {
            throw std::runtime_error("genFMod: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFMod");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Modulus two floating point values from the stack");
        popDS(firstVal); // Pop the first floating point value
        popDS(secondVal); // Pop the second floating point value
        a.movq(asmjit::x86::xmm0, secondVal);
        a.movq(asmjit::x86::xmm1, firstVal);
        a.divsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Divide the values
        a.roundsd(asmjit::x86::xmm0, asmjit::x86::xmm0, 1); // Floor the result
        a.mulsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Multiply back
        a.movq(firstVal, asmjit::x86::xmm0); // Move the intermediate result to firstVal
        a.movq(asmjit::x86::xmm0, secondVal); // Move the first value back to XMM0
        a.movq(asmjit::x86::xmm1, firstVal); // Move the intermediate result to XMM1
        a.subsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Subtract to get modulus
        a.movq(firstVal, asmjit::x86::xmm0); // Move the result back to a general-purpose register
        pushDS(firstVal); // Push the result back onto the stack
    }


    static void genSqrt() {
        if (!jc.assembler) {
            throw std::runtime_error("genSqrt: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genSqrt");

        asmjit::x86::Gp val = asmjit::x86::rax;
        popDS(val); // Pop the value from the stack
        a.movq(asmjit::x86::xmm0, val); // Move the value to XMM0
        a.sqrtsd(asmjit::x86::xmm0, asmjit::x86::xmm0); // Compute the square root
        a.movq(val, asmjit::x86::xmm0); // Move the result back to a general-purpose register
        pushDS(val); // Push the result back onto the stack
    }

    static void genFMax() {
        if (!jc.assembler) {
            throw std::runtime_error("genFMax: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFMax");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Find the maximum of two floating point values from the stack");
        popDS(firstVal); // Pop the first floating point value
        popDS(secondVal); // Pop the second floating point value
        a.movq(asmjit::x86::xmm0, firstVal); // Move the first value to XMM0
        a.movq(asmjit::x86::xmm1, secondVal); // Move the second value to XMM1
        a.maxsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Compute the maximum of the two values
        a.movq(firstVal, asmjit::x86::xmm0); // Move the result back to a general-purpose register
        pushDS(firstVal); // Push the result back onto the stack
    }

    static void genFMin() {
        if (!jc.assembler) {
            throw std::runtime_error("genFMin: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFMin");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Find the minimum of two floating point values from the stack");
        popDS(firstVal); // Pop the first floating point value
        popDS(secondVal); // Pop the second floating point value
        a.movq(asmjit::x86::xmm0, firstVal); // Move the first value to XMM0
        a.movq(asmjit::x86::xmm1, secondVal); // Move the second value to XMM1
        a.minsd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Compute the minimum of the two values
        a.movq(firstVal, asmjit::x86::xmm0); // Move the result back to a general-purpose register
        pushDS(firstVal); // Push the result back onto the stack
    }


    static void genFAbs() {
        if (!jc.assembler) {
            throw std::runtime_error("genFAbs: Assembler not initialized");
        }
        auto &a = *jc.assembler;
        a.comment(" ; ----- genFAbs");

        asmjit::x86::Gp val = asmjit::x86::rax;
        asmjit::x86::Gp mask = asmjit::x86::rbx;

        uint64_t absMask = 0x7FFFFFFFFFFFFFFF; // Mask to clear the sign bit

        a.comment(" ; Compute the absolute value of a floating point value from the stack");
        popDS(val); // Pop the floating point value from the stack
        a.mov(mask, absMask); // Move the mask into a register
        a.and_(val, mask); // Clear the sign bit
        pushDS(val); // Push the result back onto the stack
    }


    // TODO comparisons

    static void genFLess() {
        if (!jc.assembler) {
            throw std::runtime_error("genFLess: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFLess");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Compare if second floating-point value is less than the first one");
        popDS(secondVal); // Pop the second floating-point value (firstVal should store the second one)
        popDS(firstVal); // Pop the first floating-point value (secondVal should store the first one)

        a.movq(asmjit::x86::xmm0, firstVal); // Move the second value to XMM0
        a.movq(asmjit::x86::xmm1, secondVal); // Move the first value to XMM1
        a.comisd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Compare the two values

        a.setb(asmjit::x86::al); // Set AL to 1 if less than, 0 otherwise

        a.movzx(firstVal, asmjit::x86::al); // Zero extend AL to the full register

        // Now convert the boolean result to the expected -1 for true and 0 for false
        a.neg(firstVal); // Perform two's complement negation to get -1 if AL was set to 1

        pushDS(firstVal); // Push the result (-1 for true, 0 for false)
    }


    static void genFGreater() {
        if (!jc.assembler) {
            throw std::runtime_error("genFGreater: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFGreater");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        a.comment(" ; Compare if second floating-point value is greater than the first one");
        popDS(firstVal); // Pop the first floating-point value
        popDS(secondVal); // Pop the second floating-point value
        a.movq(asmjit::x86::xmm0, firstVal); // Move the first value to XMM0
        a.movq(asmjit::x86::xmm1, secondVal); // Move the second value to XMM1
        a.comisd(asmjit::x86::xmm0, asmjit::x86::xmm1); // Compare the two values

        a.setb(asmjit::x86::al); // Set AL to 1 if less than, 0 otherwise

        a.movzx(firstVal, asmjit::x86::al); // Zero extend AL to the full register

        // Now convert the boolean result to the expected -1 for true and 0 for false
        a.neg(firstVal); // Perform two's complement negation to get -1 if AL was set to 1

        pushDS(firstVal); // Push the result (-1 for true, 0 for false)
    }


    static void genLoadXMM1(double value) {
        // Use the existing `.data` section to embed the constant
        asmjit::Section *dataSection = jc.code.sectionByName(".data");

        if (nullptr == dataSection) {
            throw std::runtime_error(".data section not found. Ensure the code holder is properly set up.");
        }
        dataSection->setAlignment(16); // Align the data section to 16 bytes (128-bit)

        // Bind the variable label to the .data section
        jc.assembler->section(dataSection); // Switch to the .data section
        asmjit::Label valueLabel = jc.assembler->newLabel(); // Create a label for referencing the constant

        jc.assembler->bind(valueLabel); // Bind the label to this memory location
        jc.assembler->embedDouble(value); // Embed the double value in the section


        jc.assembler->section(jc.code.textSection());
        // Load the value from `.data` into xmm1
        jc.assembler->movsd(asmjit::x86::xmm1, asmjit::x86::ptr(valueLabel)); // Load constant value into xmm1
    }

    static void genFApproxEquals() {
        if (!jc.assembler) {
            throw std::runtime_error("genFApproxEquals: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFApproxEquals (Epsilon-Based Floating-Point Equality)");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        asmjit::x86::Xmm xmm0 = asmjit::x86::xmm0;
        asmjit::x86::Xmm xmm1 = asmjit::x86::xmm1;
        asmjit::x86::Xmm xmm2 = asmjit::x86::xmm2;

        // Load EPSILON directly as immediate data
        static const uint64_t EPSILON_BITS = 0x3DAA3B294F62C8C0; // 1e-9 as IEEE 754
        a.mov(asmjit::x86::rax, EPSILON_BITS);
        a.movq(xmm2, asmjit::x86::rax); // Load epsilon into xmm2

        // Pop floating-point values
        popDS(firstVal); // Get first floating-point value
        popDS(secondVal); // Get second floating-point value

        a.movq(xmm0, firstVal); // Move first value to XMM0
        a.movq(xmm1, secondVal); // Move second value to XMM1

        // Compute absolute difference |a - b|
        a.subsd(xmm0, xmm1); // xmm0 = a - b
        a.andpd(xmm0, asmjit::x86::ptr(reinterpret_cast<uint64_t>(&maskAbs))); // xmm0 = fabs(a - b)

        // Compare fabs(a - b) < EPSILON
        a.comisd(xmm0, xmm2); // Compare |a - b| and ε
        a.setb(asmjit::x86::al); // Set AL to 1 if fabs(a - b) < EPSILON

        a.movzx(firstVal, asmjit::x86::al); // Zero extend AL to full register
        a.neg(firstVal); // Convert 1 to -1 (FORTH boolean convention)

        pushDS(firstVal); // Push result (-1 for true, 0 for false)
    }

    static void genFApproxNotEquals() {
        if (!jc.assembler) {
            throw std::runtime_error("genFApproxNotEquals: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFApproxNotEquals (Epsilon-Based Floating-Point Inequality)");

        asmjit::x86::Gp firstVal = asmjit::x86::rax;
        asmjit::x86::Gp secondVal = asmjit::x86::rbx;

        asmjit::x86::Xmm xmm0 = asmjit::x86::xmm0;
        asmjit::x86::Xmm xmm1 = asmjit::x86::xmm1;
        asmjit::x86::Xmm xmm2 = asmjit::x86::xmm2;

        // Load EPSILON directly as immediate data
        static const uint64_t EPSILON_BITS = 0x3DAA3B294F62C8C0; // 1e-9 as IEEE 754
        a.mov(asmjit::x86::rax, EPSILON_BITS);
        a.movq(xmm2, asmjit::x86::rax); // Load epsilon into xmm2

        // Pop floating-point values
        popDS(firstVal); // Get first floating-point value
        popDS(secondVal); // Get second floating-point value

        a.movq(xmm0, firstVal); // Move first value to XMM0
        a.movq(xmm1, secondVal); // Move second value to XMM1

        // Compute absolute difference |a - b|
        a.subsd(xmm0, xmm1); // xmm0 = a - b
        a.andpd(xmm0, asmjit::x86::ptr(reinterpret_cast<uint64_t>(&maskAbs))); // xmm0 = fabs(a - b)

        // Compare fabs(a - b) >= EPSILON
        a.comisd(xmm2, xmm0); // Compare EPSILON with |a - b|
        a.setbe(asmjit::x86::al); // Set AL to 1 if EPSILON <= fabs(a - b) (not approximately equal)

        a.movzx(firstVal, asmjit::x86::al); // Zero extend AL to full register
        a.neg(firstVal); // Convert 1 to -1 (FORTH boolean convention)

        pushDS(firstVal); // Push result (-1 for true, 0 for false)
    }


    static void genFDot() {
        if (!jc.assembler) {
            throw std::runtime_error("genFDot: Assembler not initialized");
        }

        auto &a = *jc.assembler;
        a.comment(" ; ----- genFDot");

        asmjit::x86::Gp tmpReg = asmjit::x86::rcx;

        // Pop the floating-point value and move it into the XMM register
        popDS(tmpReg);
        a.movq(asmjit::x86::xmm0, tmpReg); // Move the integer representation of the float to XMM0

        // Preserve the stack pointers
        preserveStackPointers();

        // Allocate space for the shadow space (32 bytes) + an additional space required by the ABI (8 bytes)
        a.sub(asmjit::x86::rsp, 8);

        // Call the printFloat function
        a.call(asmjit::imm(reinterpret_cast<void *>(printFloat)));

        // Restore the stack
        a.add(asmjit::x86::rsp, 8);

        // Restore the stack pointers
        restoreStackPointers();
    }


private
:
    // Private constructor to prevent instantiation
    JitGenerator() = default;

    // Private destructor if needed
    ~JitGenerator() = default;
};

#endif //JITGENERATOR_H
