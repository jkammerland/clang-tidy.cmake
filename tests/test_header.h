#ifndef TEST_HEADER_H
#define TEST_HEADER_H

// Intentional clang-tidy violations to test header checking

// modernize-use-nullptr violation
void* getPointer() {
    return 0;  // Should suggest nullptr
}

// readability-magic-numbers violation (not disabled in .clang-tidy)
inline int getMagicValue() {
    return 42;  // Magic number
}

// modernize-use-override violation
class Base {
public:
    virtual void doSomething() {}
    virtual ~Base() {}
};

class Derived : public Base {
public:
    virtual void doSomething() {}  // Missing override
};

// readability-container-size-empty violation
#include <vector>
inline bool checkVector(const std::vector<int>& vec) {
    return vec.size() == 0;  // Should use vec.empty()
}

// performance-unnecessary-value-param violation
#include <string>
inline void processString(std::string str) {  // Should be const std::string&
    // Do something
}

#endif // TEST_HEADER_H