#ifndef __ALFINA_STACK_ALLOCATOR_H__
#define __ALFINA_STACK_ALLOCATOR_H__

#include <cstdint>
#include <functional>

#include "engine/engine_utilities/error_info.h"

namespace al::engine
{
	class StackAllocator
	{
	public:
		StackAllocator	() = default;
		~StackAllocator	() = default;

		ErrorInfo	init				(size_t sizeBytes, std::function<uint8_t*(size_t)> allocate);
		void		terminate			(std::function<void(uint8_t*)> deallocate);

		uint8_t*	allocate			(size_t sizeBytes);
		uint8_t*	get_current_top		() const;
		void		free_to_pointer		(uint8_t* ptr);

	private:
		uint8_t* data;
		uint8_t* end;
		uint8_t* currentTop;
	};

	ErrorInfo StackAllocator::init(size_t sizeBytes, std::function<uint8_t* (size_t)> allocate)
	{
		data = allocate(sizeBytes);
		if (!data)
		{
			return { ErrorInfo::Code::ALLOCATION_ERROR, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::UNABLE_TO_ALLOCATE_MEMORY] };
		}

		end = data + sizeBytes;
		currentTop = data;

		return { ErrorInfo::Code::ALL_FINE, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::ALL_FINE] };
	}

	void StackAllocator::terminate(std::function<void(uint8_t*)> deallocate)
	{
		deallocate(data);
	}

	uint8_t* StackAllocator::allocate(size_t sizeBytes)
	{
		if ((currentTop + sizeBytes) > end) return nullptr;
		uint8_t* ptr = currentTop;
		currentTop += sizeBytes;
		return ptr;
	}

	uint8_t* StackAllocator::get_current_top() const
	{
		return currentTop;
	}

	void StackAllocator::free_to_pointer(uint8_t* ptr)
	{
		// @TODO : add argument validation mb
		currentTop = ptr;
	}
}

#endif
