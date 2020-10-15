#ifndef __ALFINA_STACK_ALLOCATOR_H__
#define __ALFINA_STACK_ALLOCATOR_H__

#include <cstdint>
#include <functional>

#include "allocator_interface.h"
#include "engine/engine_utilities/error_info.h"
#include "engine/asserts/asserts.h"

namespace al::engine
{
	class StackAllocator : public AllocatorInterface<StackAllocator>
	{
	public:
		StackAllocator	();
		~StackAllocator	() = default;

					    ErrorInfo	init				(size_t sizeBytes, std::function<uint8_t*(size_t)> allocate) 			noexcept;
						void		terminate			(std::function<void(uint8_t*)> deallocate)								noexcept;

		[[nodiscard]] 	uint8_t*	allocateImpl		(size_t sizeBytes)														noexcept;
		[[nodiscard]]	uint8_t*	get_current_top		() 																const 	noexcept;
						void		free_to_pointer		(uint8_t* ptr)															noexcept;
                        void		clear		        ()															            noexcept;

	private:
		uint8_t* data;
		uint8_t* end;
		uint8_t* currentTop;
	};

    StackAllocator::StackAllocator()
        : AllocatorInterface<StackAllocator>(this)
    { }

	ErrorInfo StackAllocator::init(size_t sizeBytes, std::function<uint8_t* (size_t)> allocate) noexcept
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

	void StackAllocator::terminate(std::function<void(uint8_t*)> deallocate) noexcept
	{
		deallocate(data);
	}

	uint8_t* StackAllocator::allocateImpl(size_t sizeBytes) noexcept
	{
		if ((currentTop + sizeBytes) > end) return nullptr;
		uint8_t* ptr = currentTop;
		currentTop += sizeBytes;
		return ptr;
	}

	uint8_t* StackAllocator::get_current_top() const noexcept
	{
		return currentTop;
	}

    // This method cannot be used as regular deallocate method
	void StackAllocator::free_to_pointer(uint8_t* ptr) noexcept
	{
		al_assert(ptr >= data && ptr < end);
		currentTop = ptr;
	}

    void StackAllocator::clear() noexcept
	{
		currentTop = data;
	}
}

#endif
