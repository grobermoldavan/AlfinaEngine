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

    const char* advance_to_next_line(const char* text)
    {
        const char* res = text;
        while (*res != '\0' && *res != '\n')
        {
            res++;
        }
        if (*res == '\n')
        {
            res++;
        }
        return res;
    }

    const char* advance_to_word_ending(const char* text)
    {
        const char* res = text;
        while (*res != '\n' && *res != '\r' && *res != ' ' && *res != '\0')
        {
            res++;
        }
        return res;
    }

    const char* advance_to_next_word(const char* text, char addtitionalSeparator = ' ')
    {
        const char* res = text;
        if (*res != '\r' && *res != '\n' && *res != ' ' && *res != addtitionalSeparator)
        {
            while (*res != '\n' && *res != '\r' && *res != ' ' && *res != addtitionalSeparator && *res != '\0')
            {
                res++;
            }
        }
        while ((*res == '\r' || *res == '\n' || *res == ' ' || *res == addtitionalSeparator) && *res != '\0')
        {
            res++;
        }
        return res;
    }
}

#endif
