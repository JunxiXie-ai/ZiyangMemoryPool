#pragma once

#include <cstddef>
#include <new>
#include <sys/mman.h>
#include <stdexcept>
#include <vector>

template<class T>
class ObjectPool {
public:
    // Allocate memory and construct a new object.
    T* New();

    // Destroy the object and recycle its memory.
    void Delete(T* obj);
    
    // Release all memory blocks when the pool is destroyed.
    ~ObjectPool();

    ObjectPool() = default;

    // Prevent copying to avoid releasing the same blocks twice.
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Prevent moving until ownership transfer is explicitly supported.
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

private:
    // Ensure each slot can store either an object or a free-list pointer.
    static constexpr size_t kAlignment =
        alignof(T) > alignof(void*) ? alignof(T) : alignof(void*);

    static constexpr size_t kRawSize =
        sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);

    // Round up the slot size to satisfy alignment requirements.
    static constexpr size_t kSlotSize =
        ((kRawSize + kAlignment - 1) / kAlignment) * kAlignment;

    static_assert(alignof(T) <= alignof(std::max_align_t),
                "Over-aligned types are not supported yet.");
    static_assert(kSlotSize <= 128 * 1024,
                "Object size exceeds the memory block size.");

    // Pointer to the next available memory location.
    char* _memory = nullptr;

    // Number of remaining bytes in the current memory block.
    size_t _remainBytes = 0;

    // Head of the free list for recycling released objects.
    void* _freeList = nullptr;

    // Store the starting address of each allocated memory block.
    std::vector<void*> _blocks;
};

template<class T>
T* ObjectPool<T>::New() {
    // Reuse a previously released memory slot if available.
    if (_freeList != nullptr) {
        void* slot = _freeList;

        // Remove the first node from the free list.
        _freeList = *reinterpret_cast<void**>(_freeList);

        // Construct a new object in the recycled slot.
        return new (slot) T();
    }

    // Request a new memory block when the current block is exhausted.
    if (_remainBytes < kSlotSize) {
        constexpr size_t blockSize = 128 * 1024;

        void* ptr = mmap(
            nullptr,
            blockSize,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANON,
            -1,
            0
        );

        if (ptr == MAP_FAILED) {
            throw std::bad_alloc();
        }

        // Track the allocated block for cleanup.
        try {
            _blocks.push_back(ptr);
        } catch (...) {
            munmap(ptr, blockSize);
            throw;
        }

        _memory = static_cast<char*>(ptr);
        _remainBytes = blockSize;
    }

    // Reserve space for one object.
    void* slot = _memory;

    _memory += kSlotSize;
    _remainBytes -= kSlotSize;

    // Construct the object in the reserved memory.
    return new (slot) T();
}

template<class T>
void ObjectPool<T>::Delete(T* obj) {
    if (obj == nullptr) {
        return;
    }

    // Destroy the object without releasing its memory.
    obj->~T();

    // Store the previous free-list head in the recycled slot.
    *reinterpret_cast<void**>(obj) = _freeList;

    // Update the free-list head.
    _freeList = obj;
}

template<class T>
ObjectPool<T>::~ObjectPool() {
    // Release all memory blocks owned by the pool.
    for (void* block : _blocks) {
        munmap(block, 128 * 1024);
    }
}