#ifndef AL_STRING_PROCESSING_H
#define AL_STRING_PROCESSING_H

#include <cstring>
#include <string_view>

#include "utilities/function.h"

namespace al::engine
{
    template<std::size_t Size>
    inline bool is_starts_with(const char* str, const char (&word)[Size])
    {
        return std::strncmp(str, word, Size - 1) == 0;
    }

    void for_each_line(std::string_view text, al::Function<void(std::string_view)> processCb)
    {
        const char* textData = text.data();
        const std::size_t textSize = text.size();

        const char* textPtr = text.data();
        while ((textPtr - textData) < textSize)
        {
            const char* lineBeginning = textPtr;
            std::size_t lineLength = 0;
            while ((textPtr - textData) < textSize)
            {
                if (*textPtr == '\n') break;
                ++textPtr;
                ++lineLength;
            }
            processCb({ lineBeginning, lineLength });
            ++textPtr;
        }
    }

    std::string_view get_next_word(std::string_view text, const char* additionalSeparators = "")
    {
        const std::size_t textSize = text.size();

        auto isSeparatedByMainSeparators = [](const char symbol) -> bool
        {
            return (symbol == '\0') || (symbol == ' ');
        };

        auto isSeparatedByAdditionalSeparators = [](const char symbol, const char* additionalSeparators) -> bool
        {
            bool result = false;
            const char* ptr = additionalSeparators;
            while (*ptr != '\0')
            {
                if (symbol == *ptr)
                {
                    result = true;
                    break;
                }
                ptr++;
            }
            return result;
        };

        // Skip all spaces and additional separators before the word
        const char* textPtr = text.data();
        while ( ((*textPtr == ' ') || isSeparatedByAdditionalSeparators(*textPtr, additionalSeparators)) && (*textPtr != '\0') )
        {
            ++textPtr;
        }

        const char* wordBeginning = textPtr;
        std::size_t wordLength = 0;
        while (!isSeparatedByAdditionalSeparators(*textPtr, additionalSeparators) && !isSeparatedByMainSeparators(*textPtr) && (wordLength < textSize))
        {
            ++textPtr;
            ++wordLength;
        }

        return { wordBeginning, wordLength };
    }

    void for_each_word(std::string_view text, al::Function<void(std::string_view)> processCb)
    {
        std::string_view line = text;
        while (true)
        {
            std::string_view word = get_next_word(line);
            if (word.empty())
            {
                break;
            }
            processCb(word);
            line = std::string_view{ word.data() + word.length() };
        }
    }
}

#endif
