#pragma once

// Access the next pointer stored inside a free memory block.
static inline void*& ObjNext(void* obj) {
    return *reinterpret_cast<void**>(obj);
}