#ifndef STRING_STORAGE_H
#define STRING_STORAGE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <mutex>

/**
 * GlobalString
 *  - Lifetime: entire program/compilation
 *  - Underlying storage: never moves, never freed until program end
 *  - Interned so each unique string is stored only once
 */

class GlobalString {
public:
    // Construct an empty GlobalString
    GlobalString() : data_(nullptr) {}

    // Access the raw C-string
    const char* c_str() const {
        return data_;
    }

    // Check if it's non-empty
    bool empty() const {
        return data_ == nullptr || data_[0] == '\0';
    }

    // Equality comparisons by pointer or content
    bool operator==(const GlobalString& other) const {
        return data_ == other.data_;
    }

private:
    // Only GlobalStringManager can create valid GlobalStrings
    friend class GlobalStringManager;
    explicit GlobalString(const char* p) : data_(p) {}

    const char* data_;
};


/**
 * TransientString
 *  - Lifetime: from the moment of creation until endFunction() is called.
 *  - Freed in a batch when the function ends.
 */
class TransientString {
public:
    // Construct an empty TransientString
    TransientString() : data_(nullptr) {}

    // Access the raw C-string
    const char* c_str() const {
        return data_;
    }

    bool empty() const {
        return data_ == nullptr || data_[0] == '\0';
    }

private:
    // Only TransientStringManager can create valid TransientStrings
    friend class TransientStringManager;
    explicit TransientString(const char* p) : data_(p) {}

    const char* data_;
};


/**
 * GlobalStringManager
 *  - Holds a map of interned strings -> stable char* memory
 *  - The same text always returns the same pointer
 *  - Freed (if ever) at program shutdown
 */
class GlobalStringManager {
public:
    static GlobalStringManager& instance() {
        // Thread-safe static initialization in C++11+
        static GlobalStringManager theInstance;
        return theInstance;
    }

    // Creates or returns the interned GlobalString for 'text'.
    GlobalString create(const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if we have it already
        auto it = interned_.find(text);
        if (it != interned_.end()) {
            return GlobalString(it->second);
        }

        // Otherwise, allocate new permanent storage
        char* storage = new char[text.size() + 1];
        std::memcpy(storage, text.data(), text.size());
        storage[text.size()] = '\0';

        interned_[text] = storage;
        return GlobalString(storage);
    }


    // List all interned strings.
    void listStrings() const {

        for (const auto& entry : interned_) {
            std::cout << "Key: " << entry.first
                      << ", Value: " << entry.second << "\n";
        }
    }

private:
    GlobalStringManager() = default;
    ~GlobalStringManager() = default;

    GlobalStringManager(const GlobalStringManager&) = delete;
    GlobalStringManager& operator=(const GlobalStringManager&) = delete;

    std::unordered_map<std::string, const char*> interned_;
    std::mutex mutex_;
};


/**
 * TransientStringManager
 *  - Manages transient strings for the current "function compilation" (or scope).
 *  - You call beginFunction() before you start using transient strings,
 *    then endFunction() to free them all.
 *    Only use while compiling, the interpreter already has the string in the token stream.
 */
class TransientStringManager {
public:
    static TransientStringManager& instance() {
        static TransientStringManager theInstance;
        return theInstance;
    }

    // Called at the start of compiling a function
    void beginFunction() {
        // If you’re nesting function compilations, you’d push a new
        // vector or region on a stack. For simplicity, we’ll just assume
        // a single function at a time. So we ensure the current storage is empty.
        clearTransientStrings();
    }

    // Called at the end of compiling a function
    void endFunction() {
        clearTransientStrings();
    }

    // Create a new transient string whose lifetime ends when endFunction is called
    TransientString create(const std::string& text) {
        // Allocate memory
        char* storage = new char[text.size() + 1];
        std::memcpy(storage, text.data(), text.size());
        storage[text.size()] = '\0';

        // Keep track so we can free later
        allocated_.push_back(storage);

        // Return the TransientString
        return TransientString(storage);
    }

    // Optional: list or print the currently allocated transient strings (for debugging)
    void listTransientStrings() const {
        std::cout << "Currently allocated transient strings:\n";
        for (auto ptr : allocated_) {
            std::cout << "  " << ptr << "\n";
        }
    }

    TransientStringManager(const TransientStringManager&) = delete;
    TransientStringManager& operator=(const TransientStringManager&) = delete;

private:
    TransientStringManager() = default;
    ~TransientStringManager() {
        // In a real system, we might free at program shutdown
        clearTransientStrings();
    }

    // Free all currently allocated transient strings
    void clearTransientStrings() {
        for (auto ptr : allocated_) {
            delete[] ptr;
        }
        allocated_.clear();
    }

private:
    // We track the pointers so we can free them in bulk
    std::vector<char*> allocated_;
};



inline GlobalString promoteToGlobal(const TransientString& ts) {
    // Use the global string manager, passing the transient string's content
    return GlobalStringManager::instance().create(ts.c_str());
}

#endif // STRING_STORAGE_H
