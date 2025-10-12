#ifndef LAB2_CPP_MEMORY_HPP
#define LAB2_CPP_MEMORY_HPP
#include "types.hpp"

class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager() = default;

    void *startup_alloc(uint64_t size);

private:
    char *startup_heap_end;
};

#endif
