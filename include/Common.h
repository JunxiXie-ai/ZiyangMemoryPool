#pragma once
#include <cassert>

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
        }
    
        // Remove and return the first free memory block.
        void* Pop() {
            assert(_freeList != nullptr);
    
            void* obj = _freeList;
            _freeList = ObjNext(obj);
    
            return obj;
        }
    
    private:
        void* _freeList = nullptr;
    };