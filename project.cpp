
#include "project.h"

int main()
{
    al::engine::MemoryManager::initialize();

    al::engine::JobSystem jobSystem{ std::thread::hardware_concurrency() - 1 };

    al::engine::MemoryManager::terminate();

    return 0;
}
