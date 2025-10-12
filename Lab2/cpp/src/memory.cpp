#include "memory.hpp"
#include "common.hpp"

extern char startup_heap_base;
extern char startup_heap_boundary;

MemoryManager::MemoryManager() : startup_heap_end(&startup_heap_base) {}

void *MemoryManager::startup_alloc(uint64_t size)
{
    size = util::round_up(size, 8);  // 8-byte align
    if (startup_heap_end + size > &startup_heap_boundary) {
        return nullptr;
    }

    void *ret = startup_heap_end;
    startup_heap_end += size;
    return ret;
}