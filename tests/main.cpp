#include <iostream>
#include <vector>
#include <cassert>

#include "ObjectPool.h"
#include "Common.h"

struct TestObject {
    int id = 0;
    double value = 0.0;
};

void TestFreeList() {
    FreeList list;

    alignas(void*) char a[sizeof(void*)] = {};
    alignas(void*) char b[sizeof(void*)] = {};
    alignas(void*) char c[sizeof(void*)] = {};

    list.Push(a);
    list.Push(b);
    list.Push(c);

    // Verify LIFO ordering.
    assert(list.Pop() == c);
    assert(list.Pop() == b);
    assert(list.Pop() == a);

    std::cout << "[PASS] FreeList Push/Pop" << std::endl;
}

void TestObjNext() {
    // Simulate two aligned free memory blocks.
    alignas(void*) char blockA[sizeof(void*)] = {};
    alignas(void*) char blockB[sizeof(void*)] = {};

    ObjNext(blockA) = blockB;
    ObjNext(blockB) = nullptr;

    assert(ObjNext(blockA) == blockB);
    assert(ObjNext(blockB) == nullptr);

    std::cout << "[PASS] ObjNext" << std::endl;
}

void TestBasicAllocation() {
    ObjectPool<TestObject> pool;

    TestObject* obj = pool.New();

    obj->id = 100;
    obj->value = 3.14;

    assert(obj->id == 100);
    assert(obj->value == 3.14);

    pool.Delete(obj);

    std::cout << "[PASS] Basic Allocation" << std::endl;
}

void TestFreeListReuse() {
    ObjectPool<TestObject> pool;

    TestObject* obj1 = pool.New();
    TestObject* obj2 = pool.New();

    pool.Delete(obj1);

    // The next allocation should reuse obj1's memory.
    TestObject* obj3 = pool.New();

    assert(obj3 == obj1);

    pool.Delete(obj2);
    pool.Delete(obj3);

    std::cout << "[PASS] FreeList Reuse" << std::endl;
}

void TestLargeAllocation() {
    ObjectPool<TestObject> pool;
    std::vector<TestObject*> objects;

    // Allocate enough objects to exceed a 128KB block.
    for (int i = 0; i < 10000; ++i) {
        TestObject* obj = pool.New();
        obj->id = i;
        objects.push_back(obj);
    }

    // Verify that each object retains its value.
    for (int i = 0; i < 10000; ++i) {
        assert(objects[i]->id == i);
    }

    for (TestObject* obj : objects) {
        pool.Delete(obj);
    }

    std::cout << "[PASS] Large Allocation" << std::endl;
}

void TestSmallObject() {
    struct SmallObject {
        char value;
    };

    ObjectPool<SmallObject> pool;

    SmallObject* a = pool.New();
    SmallObject* b = pool.New();

    a->value = 'A';
    b->value = 'B';

    // Verify that adjacent objects do not overlap.
    assert(a->value == 'A');
    assert(b->value == 'B');

    pool.Delete(a);

    // Verify that the freed slot can be reused safely.
    SmallObject* c = pool.New();
    assert(c == a);

    // The second object must remain unchanged.
    assert(b->value == 'B');

    pool.Delete(b);
    pool.Delete(c);

    std::cout << "[PASS] Small Object" << std::endl;
}

int main() {
    TestBasicAllocation();
    TestFreeListReuse();
    TestLargeAllocation();
    TestSmallObject();
    TestObjNext();
    TestFreeList();

    std::cout << "All ObjectPool tests passed!" << std::endl;

    return 0;
}