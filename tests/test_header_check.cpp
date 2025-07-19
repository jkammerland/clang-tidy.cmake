#include "test_header.h"

// This file includes the header to ensure clang-tidy checks it
// The violations are in the header file, not here

int main() {
    // Use functions from header to ensure they're not optimized away
    void* ptr = getPointer();
    int magic = getMagicValue();
    
    std::vector<int> vec;
    bool empty = checkVector(vec);
    
    processString("test");
    
    Derived d;
    d.doSomething();
    
    return 0;
}