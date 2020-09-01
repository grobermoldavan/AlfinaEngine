#ifndef __ALFINA_STRING_UTILITIES_H__
#define __ALFINA_STRING_UTILITIES_H__

#include <functional>

namespace al::engine
{
	template<size_t size>
	inline bool is_starts_with(const char* str, const char (&word)[size])
	{
		return strncmp(str, word, size - 1) == 0;
	}

	void for_each_line(const char* text, const size_t size, std::function<void(const char* const, const size_t)> processCb)
	{
		const char* ptr = text;
		while ((ptr - text) < size)
		{
			const char* lineBeginning = ptr;
			size_t lineLength = 0;
			while ((ptr - text) < size)
			{
				if (*ptr == '\n') break;
				++ptr;
				++lineLength;
			}
			processCb(lineBeginning, lineLength);
			++ptr;
		}
	}

	void get_next_word(const char* line, const size_t lineLength, const char** resultStart, size_t* resultLen)
	{
		size_t length = 0;
		const char* ptr = line;
		while (*ptr == ' ') ++ptr;

		*resultStart = ptr;
		while ((*ptr != ' ') && (length < lineLength))
		{
			++ptr;
			++length;
		}

		*resultLen = length;
	}
}

#endif
