#include "test_header.h"

// This file includes the header to ensure clang-tidy checks it
// The violations are in the header file, not here

int main() {
    // Use functions from header to ensure they're not optimized away
    void* ptr = getPointer(); // NOLINT(clang-analyzer-deadcode.DeadStores)
    int magic = getMagicValue(); // NOLINT(clang-analyzer-deadcode.DeadStores)
    
    std::vector<int> vec;
    bool empty = checkVector(vec); // NOLINT(clang-analyzer-deadcode.DeadStores)
    
    processString("test");
    
    Derived d;
    d.doSomething();
    
    return 0;
}