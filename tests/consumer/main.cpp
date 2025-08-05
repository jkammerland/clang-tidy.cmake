#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> numbers = {5, 2, 8, 1, 9};
    
    // Sort the vector
    std::sort(numbers.begin(), numbers.end());
    
    std::cout << "Sorted numbers: ";
    for (const auto& num : numbers) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    
    // Add some intentional issues for testing
    int* ptr = 0;  // Should suggest nullptr
    if (numbers.size() == 0) {  // Should suggest empty()
        std::cout << "Empty!" << std::endl;
    }
    
    return 0;
}