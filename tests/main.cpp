#include <iostream>
#include <vector>
#include <cassert>

#include "ObjectPool.h"
#include "Common.h"

struct TestObject {
    int id = 0;
    double value = 0.0;
};

void TestSizeClassRoundUp() {
    assert(SizeClass::RoundUp(7) == 8);
    assert(SizeClass::RoundUp(13) == 16);
    assert(SizeClass::RoundUp(128) == 128);
    assert(SizeClass::RoundUp(130) == 144);
    assert(SizeClass::RoundUp(1025) == 1152);
    assert(SizeClass::RoundUp(8193) == 9216);

    std::cout << "[PASS] SizeClass RoundUp" << std::endl;
}

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

void TestSizeClassIndex() {
    assert(SizeClass::Index(1) == 0);
    assert(SizeClass::Index(8) == 0);
    assert(SizeClass::Index(13) == 1);
    assert(SizeClass::Index(128) == 15);

    assert(SizeClass::Index(129) == 16);
    assert(SizeClass::Index(130) == 16);
    assert(SizeClass::Index(1024) == 71);

    assert(SizeClass::Index(1025) == 72);
    assert(SizeClass::Index(8192) == 127);

    assert(SizeClass::Index(8193) == 128);
    assert(SizeClass::Index(65536) == 183);

    assert(SizeClass::Index(65537) == 184);
    assert(SizeClass::Index(256 * 1024) == 207);

    std::cout << "[PASS] SizeClass Index" << std::endl;
}

void TestNumMoveSize() {
    assert(SizeClass::NumMoveSize(8) == 512);
    assert(SizeClass::NumMoveSize(128) == 512);
    assert(SizeClass::NumMoveSize(1024) == 64);
    assert(SizeClass::NumMoveSize(8192) == 8);
    assert(SizeClass::NumMoveSize(65536) == 2);

    std::cout << "[PASS] NumMoveSize" << std::endl;
}

void TestFreeListMaxSize() {
    FreeList list;

    // The initial batch limit should be one.
    assert(list.MaxSize() == 1);

    // Increase the batch limit for the next request.
    list.MaxSize()++;

    assert(list.MaxSize() == 2);

    std::cout << "[PASS] FreeList MaxSize" << std::endl;
}

void TestFreeListPushRange() {
    FreeList list;

    alignas(void*) char a[sizeof(void*)] = {};
    alignas(void*) char b[sizeof(void*)] = {};
    alignas(void*) char c[sizeof(void*)] = {};
    alignas(void*) char d[sizeof(void*)] = {};

    list.Push(a);

    // Prepare a linked range: b -> c -> d.
    ObjNext(b) = c;
    ObjNext(c) = d;
    ObjNext(d) = nullptr;

    list.PushRange(b, d, 3);

    assert(list.Size() == 4);

    // Verify the insertion order.
    assert(list.Pop() == b);
    assert(list.Pop() == c);
    assert(list.Pop() == d);
    assert(list.Pop() == a);

    assert(list.Size() == 0);

    std::cout << "[PASS] FreeList PushRange" << std::endl;
}

int main() {
    TestBasicAllocation();
    TestFreeListReuse();
    TestLargeAllocation();
    TestSmallObject();
    TestObjNext();
    TestFreeList();
    TestSizeClassRoundUp();
    TestSizeClassIndex();
    TestNumMoveSize();
    TestFreeListMaxSize();
    TestFreeListPushRange();
    std::cout << "All ObjectPool tests passed!" << std::endl;

    return 0;
}