#pragma once
#include <cassert>
#include <cstddef>

// Access the next pointer stored inside a free memory block.
static inline void*& ObjNext(void* obj) {
    return *reinterpret_cast<void**>(obj);
}

// Manage reusable memory blocks using a singly linked list.
class FreeList {
    public:
        // Insert a free memory block at the head.
        void Push(void* obj) {
            assert(obj != nullptr);
    
            ObjNext(obj) = _freeList;
            _freeList = obj;
            _size++;
        }
    
        // Remove and return the first free memory block.
        void* Pop() {
            assert(_freeList != nullptr);
    
            void* obj = _freeList;
            _freeList = ObjNext(obj);
            _size--;

            return obj;
        }

        // Access the current batch limit for slow-start allocation.
        size_t& MaxSize() {
            return _maxSize;
        }

        // Insert a linked range of free blocks at the head.
        void PushRange(void* start, void* end, size_t n) {
            assert(start != nullptr);
            assert(end != nullptr);
            assert(n > 0);

            ObjNext(end) = _freeList;
            _freeList = start;
            _size += n;
        }

        // Return the number of free blocks.
        size_t Size() const {
            return _size;
        }

        // Remove a linked range of free blocks from the head.
        void PopRange(void*& start, void*& end, size_t n) {
            assert(n > 0 && n <= _size);

            start = _freeList;
            end = start;

            // Find the last block in the requested range.
            for (size_t i = 1; i < n; ++i) {
                end = ObjNext(end);
            }

            // Update the free-list head.
            _freeList = ObjNext(end);

            // Disconnect the extracted range.
            ObjNext(end) = nullptr;

            _size -= n;
        }
    
    private:
        void* _freeList = nullptr;
        // Maximum batch size requested during slow start.
        size_t _maxSize = 1;
        // Number of free blocks currently stored in the list.
        size_t _size = 0;
    };

    // Map allocation sizes to aligned size classes.
class SizeClass {
    public:
        // Round up the requested size to the required alignment.
        static inline size_t RoundUp(size_t size) {
            assert(size > 0 && size <= 256 * 1024);
    
            if (size <= 128) {
                return _RoundUp(size, 8);
            }
            else if (size <= 1024) {
                return _RoundUp(size, 16);
            }
            else if (size <= 8 * 1024) {
                return _RoundUp(size, 128);
            }
            else if (size <= 64 * 1024) {
                return _RoundUp(size, 1024);
            }
            else {
                return _RoundUp(size, 8 * 1024);
            }
        }

        // Map a requested size to its free-list bucket index.
    static inline size_t Index(size_t size) {
        assert(size > 0 && size <= 256 * 1024);

        if (size <= 128) {
            return _Index(size, 3);
        }
        else if (size <= 1024) {
            return _Index(size - 128, 4) + 16;
        }
        else if (size <= 8 * 1024) {
            return _Index(size - 1024, 7) + 72;
        }
        else if (size <= 64 * 1024) {
            return _Index(size - 8 * 1024, 10) + 128;
        }
        else {
            return _Index(size - 64 * 1024, 13) + 184;
        }
    }

    // Calculate the recommended number of objects per batch.
    static inline size_t NumMoveSize(size_t size) {
        assert(size > 0 && size <= 256 * 1024);

        size_t num = (64 * 1024) / size;

        // Limit the batch size to avoid excessive memory usage.
        if (num < 2) {
            num = 2;
        }

        if (num > 512) {
            num = 512;
        }

        return num;
    }
    
    private:
        // Round up to the nearest multiple of alignment.
        static inline size_t _RoundUp(size_t size, size_t alignment) {
            return ((size + alignment - 1) / alignment) * alignment;
        }

        // Calculate the zero-based bucket index within a size range.
        static inline size_t _Index(size_t size, size_t alignShift) {
            return ((size + (size_t{1} << alignShift) - 1)
                    >> alignShift) - 1;
        }
        
    };