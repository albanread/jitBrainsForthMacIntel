#ifndef STRINGINTERNER_H
#define STRINGINTERNER_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

class StringStorage
{
public:
    // Adds a string and returns the index
    size_t addString(const std::string& str)
    {
        strings.push_back(str);
        return strings.size() - 1;
    }

    // Retrieves a string by its index
    [[nodiscard]] std::string getString(size_t index) const
    {
        if (index >= strings.size())
        {
            throw std::out_of_range("Index out of range");
        }
        return strings[index];
    }

    [[nodiscard]] void* getStringAddress(const size_t index) const
    {
        if (index >= strings.size())
        {
            throw std::out_of_range("Index out of range");
        }

        const auto address = (void*)strings[index].c_str();
        return address;
    }

    // Clears all stored strings
    void clearStrings()
    {
        strings.clear();
    }

    // Removes a string by its index
    void removeString(size_t index)
    {
        if (index < strings.size())
        {
            strings[index].clear(); // Clearing the string content
        }
    }

private:
    std::vector<std::string> strings;
};

class StringInterner
{
public:
    // Returns the singleton instance of StringInterner.
    static StringInterner& getInstance()
    {
        static StringInterner instance;
        return instance;
    }

    // Deletes the copy constructor and assignment operator to prevent copying.
    StringInterner(const StringInterner&) = delete;
    StringInterner& operator=(const StringInterner&) = delete;

    // Interns a string and returns its index.
    size_t intern(const std::string& str)
    {

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = intern_map.find(str);
        if (it != intern_map.end())
        {
            ref_counts[it->second]++;
            return it->second;
        }

        // Add the string to storage and get the index
        size_t index = storage.addString(str);
        intern_map[str] = index;
        ref_counts[index] = 1;

        return index;
    }

    // Retrieves a string by its index.
    std::string getString(size_t index) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return storage.getString(index);
    }

    void* getStringAddress(size_t index) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return storage.getStringAddress(index);
    }

    // increment ref for string at index
    void incrementRef(size_t index)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ref_counts.find(index) != ref_counts.end())
        {
            ref_counts[index]++;
        }
    }

    // decrement ref for string at index
    void decrementRef(size_t index)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ref_counts.find(index) != ref_counts.end())
        {
            ref_counts[index]--;

            // Release resources if the reference count drops to zero
            if (ref_counts[index] <= 0)
            {
                removeString(index);
            }
        }
    }

    // Decreases the reference count of the string by its index and removes it if the count reaches zero.
    void release(size_t index)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        decrementRef(index);
    }

    void releaseIf1(size_t index)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // decrement ref for string at index
        if (ref_counts.find(index) != ref_counts.end())
        {
            if (ref_counts[index] == 1)
                decrementRef(index);
        }
    }


    // Lists all interned strings along with their reference counts.
    std::vector<std::pair<std::string, size_t>> list() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::pair<std::string, size_t>> list;
        for (const auto& entry : intern_map)
        {
            list.emplace_back(entry.first, ref_counts.at(entry.second));
        }
        return list;
    }

    // Displays list of strings, their indices, and reference counts.
    void display_list() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& entry : intern_map)
        {
            const auto& str = entry.first;
            const auto index = entry.second;
            const auto ref_count = ref_counts.at(index);
            const auto address = storage.getStringAddress(index);
            std::cout << "[" << str << "] (Index: " << index << ", Ref Count: " << ref_count << ", Address: " << address
                << ")" << std::endl;
        }
    }

    // Concatenate two strings by their indices and intern the result.
    size_t StringCat(const size_t index1, const size_t index2)
    {
        const std::string newStr = getString(index1) + getString(index2);
        return intern(newStr);
    }

    // Compare two strings by their indices to check if they are the same.
    bool StrEqual(const size_t index1, const size_t index2) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return getString(index1) == getString(index2);
    }

    bool StrContains(size_t index1, size_t index2) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return getString(index1).find(getString(index2)) != std::string::npos;
    }

    // Return the position of the string at index1 within the string at index2, or -1 if not found.
    int StrPos(size_t index1, size_t index2) const
    {
        size_t pos = getString(index2).find(getString(index1));
        return (pos != std::string::npos) ? static_cast<int>(pos) : -1;
    }


    // Split the string at index1 by the string at index2 and return the interned string at the given position.
    size_t StringSplit(size_t index1, size_t delimiterIndex, size_t position)
    {

        std::string str = getString(index1);
        std::string delimiter = getString(delimiterIndex);
        size_t start = 0;
        size_t end = 0;
        size_t current_pos = 0;

        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            if (current_pos == position)
            {
                return intern(str.substr(start, end - start));
            }
            start = end + delimiter.length();
            ++current_pos;
        }

        if (current_pos == position)
        {
            return intern(str.substr(start));
        }

        throw std::out_of_range("Position exceeds the number of substrings");
    }


    // Count the number of fields in the string at index1 that would result from splitting by the string at delimiterIndex.
    size_t CountFields(size_t index1, size_t delimiterIndex) const
    {
        std::string str = getString(index1);
        std::string delimiter = getString(delimiterIndex);
        size_t field_count = 0;
        size_t start = 0;
        size_t end = 0;

        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            ++field_count;
            start = end + delimiter.length();
        }

        // Account for the last field after the final delimiter
        if (start < str.length())
        {
            ++field_count;
        }

        return field_count;
    }





private:
    StringInterner() = default; // Private constructor for singleton.

    // Private members for holding the interned strings and mutex for thread safety.
    std::unordered_map<std::string, size_t> intern_map;
    std::unordered_map<size_t, size_t> ref_counts; // Store ref counts by index
    mutable std::mutex mutex_; // Mutable to allow locking in const functions.
    StringStorage storage; // Storage for the interned strings

    void removeString(size_t index)
    {
        auto it = std::find_if(intern_map.begin(), intern_map.end(),
                               [index](const auto& pair) { return pair.second == index; });
        if (it != intern_map.end())
        {
            intern_map.erase(it);
            ref_counts.erase(index);
            storage.removeString(index);
        }
    }
};

#endif // STRINGINTERNER_H
