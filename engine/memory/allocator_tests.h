#ifndef AL_ALLOCATOR_TESTS_H
#define AL_ALLOCATOR_TESTS_H

#include <stdlib.h>
#include <iostream>
#include <span>

#include "allocator_base.h"
#include "pool_allocator.h"
#include "system_allocator.h"
#include "dl_allocator.h"
#include "engine/job_system/job_system.h"
#include "engine/asserts/asserts.h"

#include "utilities/constexpr_functions.h"
#include "utilities/static_unordered_list.h"
#include "utilities/scope_timer.h"
#include "utilities/smooth_average.h"

namespace al::engine::test
{
    Job allocatorTestJob;

    static constexpr std::size_t MAX_MEM_BLOCKS = 10000;

    struct AllocatorTestSettings
    {
        const std::size_t ALLOCATION_SIZE_MIN;
        const std::size_t ALLOCATION_SIZE_MAX;

        const std::size_t SEQUENCE_ALLOCATION_NUM;

        const std::size_t RANDOM_ALLOCATION_ITER_ALLOC_MAX;
        const std::size_t RANDOM_ALLOCATION_ITERATIONS;
        const float RANDOM_ALLOCATION_DEALLOCATION_PROB;
    };

    struct PtrSizePair
    {
        std::byte* ptr;
        std::size_t size;
    };

    struct AllocatorTestResult
    {
        double seqSystemAllocator;
        double seqDlAllocator;
        double seqPoolAllocator;
        double rndSystemAllocator;
        double rndDlAllocator;
        double rndPoolAllocator;
    };

    template<float SmoothingConstant>
    struct FinalAllocatorTestResult
    {
        SmoothAverage<double> seqSystemAllocator    { SmoothingConstant };
        SmoothAverage<double> seqDlAllocator        { SmoothingConstant };
        SmoothAverage<double> seqPoolAllocator      { SmoothingConstant };
        SmoothAverage<double> rndSystemAllocator    { SmoothingConstant };
        SmoothAverage<double> rndDlAllocator        { SmoothingConstant };
        SmoothAverage<double> rndPoolAllocator      { SmoothingConstant };
    };

    double test_sequence_allocation_deallocation(std::ostream& stream, AllocatorTestSettings settings, AllocatorBase* allocator, const char* name)
    {
        ScopeTimer timer { [&](double time)
        {
            stream << " Sequence allocation for " << name << " : " << time << " ms" << std::endl;
        }};

        SuList<PtrSizePair, MAX_MEM_BLOCKS> memBlocks;

        for (std::size_t it = 0; it < settings.SEQUENCE_ALLOCATION_NUM; it++)
        {
            std::size_t memAllocationSizeBytes = ((rand()) % (settings.ALLOCATION_SIZE_MAX - settings.ALLOCATION_SIZE_MIN)) + settings.ALLOCATION_SIZE_MIN;
            PtrSizePair* mem = memBlocks.get();
            al_assert(mem);

            mem->ptr = allocator->allocate(memAllocationSizeBytes);
            al_assert(mem->ptr);

            mem->size = memAllocationSizeBytes;
        }

        memBlocks.for_each([allocator](PtrSizePair* mem)
        {
            allocator->deallocate(mem->ptr, mem->size);
        });

        return timer.get_current_time();
    }

    double test_randomized_allocation_deallocation(std::ostream& stream, AllocatorTestSettings settings, AllocatorBase* allocator, const char* name)
    {
        ScopeTimer timer { [&](double time)
        {
            stream << " Random allocation for " << name << " : " << time << " ms" << std::endl;
        }};

        SuList<PtrSizePair, MAX_MEM_BLOCKS> memBlocks;

        for (std::size_t it = 0; it < settings.RANDOM_ALLOCATION_ITERATIONS; it++)
        {
            for (std::size_t it2 = 0; it2 < settings.RANDOM_ALLOCATION_ITER_ALLOC_MAX; it2++)
            {
                std::size_t memAllocationSizeBytes = ((rand()) % (settings.ALLOCATION_SIZE_MAX - settings.ALLOCATION_SIZE_MIN)) + settings.ALLOCATION_SIZE_MIN;
                PtrSizePair* mem = memBlocks.get();
                al_assert(mem);

                mem->ptr = allocator->allocate(memAllocationSizeBytes);
                al_assert(mem->ptr);

                mem->size = memAllocationSizeBytes;

                memBlocks.remove_by_condition([&](PtrSizePair* mem)
                {
                    float rnd = (float)rand() / (float)RAND_MAX;
                    if (rnd > settings.RANDOM_ALLOCATION_DEALLOCATION_PROB)
                    {
                        allocator->deallocate(mem->ptr, mem->size);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                });
            }

            memBlocks.remove_by_condition([allocator](PtrSizePair* mem)
            {
                allocator->deallocate(mem->ptr, mem->size);
                return true;
            });
        }

        return timer.get_current_time();
    }

    AllocatorTestResult run_test_iteration(std::ostream& stream, AllocatorTestSettings settings)
    {
        AllocatorTestResult result;

        // Sequence allocations
        {
            SystemAllocator sysAlloc;
            result.seqSystemAllocator = test_sequence_allocation_deallocation(stream, settings, &sysAlloc, "System Allocator");

            DlAllocator dlAlloc;
            result.seqDlAllocator = test_sequence_allocation_deallocation(stream, settings, &dlAlloc, "DL Allocator");

            PoolAllocator poolAlloc;
            poolAlloc.initialize({
                bucket_desc(settings.ALLOCATION_SIZE_MAX / 8    , settings.ALLOCATION_SIZE_MAX * settings.SEQUENCE_ALLOCATION_NUM),
                bucket_desc(settings.ALLOCATION_SIZE_MAX / 4    , settings.ALLOCATION_SIZE_MAX * settings.SEQUENCE_ALLOCATION_NUM),
                bucket_desc(settings.ALLOCATION_SIZE_MAX / 2    , settings.ALLOCATION_SIZE_MAX * settings.SEQUENCE_ALLOCATION_NUM),
                bucket_desc(settings.ALLOCATION_SIZE_MAX        , settings.ALLOCATION_SIZE_MAX * settings.SEQUENCE_ALLOCATION_NUM)
            }, &sysAlloc);
            result.seqPoolAllocator = test_sequence_allocation_deallocation(stream, settings, &poolAlloc, "Pool Allocator");
        }

        // Random allocations
        {
            SystemAllocator sysAlloc;
            result.rndSystemAllocator = test_randomized_allocation_deallocation(stream, settings, &sysAlloc, "System Allocator");

            DlAllocator dlAlloc;
            result.rndDlAllocator = test_randomized_allocation_deallocation(stream, settings, &dlAlloc, "DL Allocator");

            PoolAllocator poolAlloc;
            poolAlloc.initialize({
                bucket_desc(settings.ALLOCATION_SIZE_MAX / 8    , settings.ALLOCATION_SIZE_MAX * settings.RANDOM_ALLOCATION_ITERATIONS * settings.RANDOM_ALLOCATION_ITER_ALLOC_MAX),
                bucket_desc(settings.ALLOCATION_SIZE_MAX / 4    , settings.ALLOCATION_SIZE_MAX * settings.RANDOM_ALLOCATION_ITERATIONS * settings.RANDOM_ALLOCATION_ITER_ALLOC_MAX),
                bucket_desc(settings.ALLOCATION_SIZE_MAX / 2    , settings.ALLOCATION_SIZE_MAX * settings.RANDOM_ALLOCATION_ITERATIONS * settings.RANDOM_ALLOCATION_ITER_ALLOC_MAX),
                bucket_desc(settings.ALLOCATION_SIZE_MAX        , settings.ALLOCATION_SIZE_MAX * settings.RANDOM_ALLOCATION_ITERATIONS * settings.RANDOM_ALLOCATION_ITER_ALLOC_MAX)
            }, &sysAlloc);
            result.rndPoolAllocator = test_randomized_allocation_deallocation(stream, settings, &poolAlloc, "Pool Allocator");
        }

        return result;
    }

    void run_allocator_tests(std::ostream& stream)
    {
        AllocatorTestSettings settingsArray[] = 
        {
            AllocatorTestSettings {
                .ALLOCATION_SIZE_MIN = 256,
                .ALLOCATION_SIZE_MAX = kilobytes<std::size_t>(2),
                .SEQUENCE_ALLOCATION_NUM = 10000,
                .RANDOM_ALLOCATION_ITER_ALLOC_MAX = 1000,
                .RANDOM_ALLOCATION_ITERATIONS = 100,
                .RANDOM_ALLOCATION_DEALLOCATION_PROB = 10
            },
            AllocatorTestSettings {
                .ALLOCATION_SIZE_MIN = 256,
                .ALLOCATION_SIZE_MAX = kilobytes<std::size_t>(2),
                .SEQUENCE_ALLOCATION_NUM = 10000,
                .RANDOM_ALLOCATION_ITER_ALLOC_MAX = 1000,
                .RANDOM_ALLOCATION_ITERATIONS = 100,
                .RANDOM_ALLOCATION_DEALLOCATION_PROB = 25
            },
            AllocatorTestSettings {
                .ALLOCATION_SIZE_MIN = 256,
                .ALLOCATION_SIZE_MAX = kilobytes<std::size_t>(2),
                .SEQUENCE_ALLOCATION_NUM = 10000,
                .RANDOM_ALLOCATION_ITER_ALLOC_MAX = 1000,
                .RANDOM_ALLOCATION_ITERATIONS = 100,
                .RANDOM_ALLOCATION_DEALLOCATION_PROB = 40
            },
            AllocatorTestSettings {
                .ALLOCATION_SIZE_MIN = 256,
                .ALLOCATION_SIZE_MAX = kilobytes<std::size_t>(2),
                .SEQUENCE_ALLOCATION_NUM = 10000,
                .RANDOM_ALLOCATION_ITER_ALLOC_MAX = 1000,
                .RANDOM_ALLOCATION_ITERATIONS = 100,
                .RANDOM_ALLOCATION_DEALLOCATION_PROB = 55
            },
            AllocatorTestSettings {
                .ALLOCATION_SIZE_MIN = 512,
                .ALLOCATION_SIZE_MAX = kilobytes<std::size_t>(8),
                .SEQUENCE_ALLOCATION_NUM = 10000,
                .RANDOM_ALLOCATION_ITER_ALLOC_MAX = 1000,
                .RANDOM_ALLOCATION_ITERATIONS = 100,
                .RANDOM_ALLOCATION_DEALLOCATION_PROB = 10
            },
            AllocatorTestSettings {
                .ALLOCATION_SIZE_MIN = 512,
                .ALLOCATION_SIZE_MAX = kilobytes<std::size_t>(8),
                .SEQUENCE_ALLOCATION_NUM = 10000,
                .RANDOM_ALLOCATION_ITER_ALLOC_MAX = 1000,
                .RANDOM_ALLOCATION_ITERATIONS = 100,
                .RANDOM_ALLOCATION_DEALLOCATION_PROB = 25
            },
            AllocatorTestSettings {
                .ALLOCATION_SIZE_MIN = 512,
                .ALLOCATION_SIZE_MAX = kilobytes<std::size_t>(8),
                .SEQUENCE_ALLOCATION_NUM = 10000,
                .RANDOM_ALLOCATION_ITER_ALLOC_MAX = 1000,
                .RANDOM_ALLOCATION_ITERATIONS = 100,
                .RANDOM_ALLOCATION_DEALLOCATION_PROB = 40
            },
            AllocatorTestSettings {
                .ALLOCATION_SIZE_MIN = 512,
                .ALLOCATION_SIZE_MAX = kilobytes<std::size_t>(8),
                .SEQUENCE_ALLOCATION_NUM = 10000,
                .RANDOM_ALLOCATION_ITER_ALLOC_MAX = 1000,
                .RANDOM_ALLOCATION_ITERATIONS = 100,
                .RANDOM_ALLOCATION_DEALLOCATION_PROB = 55
            }
        };

        std::span<AllocatorTestSettings> settings { settingsArray };

        constexpr std::size_t settingsNum = sizeof(settingsArray) / sizeof(AllocatorTestSettings);
        SuList<AllocatorTestResult, settingsNum> results;

        for (auto& settingsEntry : settings)
        {
            AllocatorTestResult* result = results.get();
            *result = run_test_iteration(stream, settingsEntry);
            stream << std::endl;
        }

        FinalAllocatorTestResult<0.25f> averageResult;

        results.for_each([&] (AllocatorTestResult* result)
        {
            averageResult.seqSystemAllocator.push(result->seqSystemAllocator);
            averageResult.seqDlAllocator.push(result->seqDlAllocator);
            averageResult.seqPoolAllocator.push(result->seqPoolAllocator);

            averageResult.rndSystemAllocator.push(result->rndSystemAllocator);
            averageResult.rndDlAllocator.push(result->rndDlAllocator);
            averageResult.rndPoolAllocator.push(result->rndPoolAllocator);
        });

        stream << " ================================================================================ " << std::endl;
        stream << " Average time :" << std::endl;
        stream << " Sequence allocation for System Allocator : " << averageResult.seqSystemAllocator.get()  << " ms" << std::endl;
        stream << " Sequence allocation for Dl Allocator     : " << averageResult.seqDlAllocator.get()      << " ms" << std::endl;
        stream << " Sequence allocation for Pool Allocator   : " << averageResult.seqPoolAllocator.get()    << " ms" << std::endl;
        stream << std::endl;
        stream << " Random allocation for System Allocator   : " << averageResult.rndSystemAllocator.get()  << " ms" << std::endl;
        stream << " Random allocation for Dl Allocator       : " << averageResult.rndDlAllocator.get()      << " ms" << std::endl;
        stream << " Random allocation for Pool Allocator     : " << averageResult.rndPoolAllocator.get()    << " ms" << std::endl;
        stream << " ================================================================================ " << std::endl;
        stream << " Sequence System to Dl     Allocator relation    : " << averageResult.seqSystemAllocator.get()   / averageResult.seqDlAllocator.get()        << std::endl;
        stream << " Sequence System to Pool   Allocator relation    : " << averageResult.seqSystemAllocator.get()   / averageResult.seqPoolAllocator.get()      << std::endl;
        stream << " Sequence Dl     to System Allocator relation    : " << averageResult.seqDlAllocator.get()       / averageResult.seqSystemAllocator.get()    << std::endl;
        stream << " Sequence Dl     to Pool   Allocator relation    : " << averageResult.seqDlAllocator.get()       / averageResult.seqPoolAllocator.get()      << std::endl;
        stream << " Sequence Pool   to System Allocator relation    : " << averageResult.seqPoolAllocator.get()     / averageResult.seqSystemAllocator.get()    << std::endl;
        stream << " Sequence Pool   to Dl     Allocator relation    : " << averageResult.seqPoolAllocator.get()     / averageResult.seqDlAllocator.get()        << std::endl;
        stream << std::endl;
        stream << " Random   System to Dl     Allocator relation    : " << averageResult.rndSystemAllocator.get()   / averageResult.rndDlAllocator.get()        << std::endl;
        stream << " Random   System to Pool   Allocator relation    : " << averageResult.rndSystemAllocator.get()   / averageResult.rndPoolAllocator.get()      << std::endl;
        stream << " Random   Dl     to System Allocator relation    : " << averageResult.rndDlAllocator.get()       / averageResult.rndSystemAllocator.get()    << std::endl;
        stream << " Random   Dl     to Pool   Allocator relation    : " << averageResult.rndDlAllocator.get()       / averageResult.rndPoolAllocator.get()      << std::endl;
        stream << " Random   Pool   to System Allocator relation    : " << averageResult.rndPoolAllocator.get()     / averageResult.rndSystemAllocator.get()    << std::endl;
        stream << " Random   Pool   to Dl     Allocator relation    : " << averageResult.rndPoolAllocator.get()     / averageResult.rndDlAllocator.get()        << std::endl;
        stream << " ================================================================================ " << std::endl;
    }
}

#endif
