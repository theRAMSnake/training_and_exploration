#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>

void run_with_stride(size_t stride_size) {
    // Cache sizes for my CPU:
    // L1 Cache
    // 512 KB
    // L2 Cache
    // 8 MB
    // L3 Cache
    // 96 MB
    const std::size_t array_size = 8 * 1024 * 1024;

    std::uint8_t* array = new std::uint8_t[array_size * stride_size];

    // Array left intentionally uninitialized
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < array_size; i += stride_size) {
        sum += array[i];
    }

    delete[] array;
}

int main(int argc, char* argv[]) {
    // Check if exactly one argument is provided
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <stride_size>" << std::endl;
        std::cerr << "  stride_size: Positive integer specifying the stride size" << std::endl;
        return 1;
    }

    try {
        // Parse the stride size argument
        std::string arg = argv[1];
        size_t stride_size = std::stoul(arg);
        
        // Validate that stride size is positive
        if (stride_size == 0) {
            std::cerr << "Error: Stride size must be a positive integer" << std::endl;
            return 1;
        }

        std::cout << "Stride size: " << stride_size << std::endl;
        
        // Measure wall time
        auto start = std::chrono::high_resolution_clock::now();
        run_with_stride(stride_size);
        auto end = std::chrono::high_resolution_clock::now();
        
        // Calculate duration
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Wall time: " << duration.count() << " microseconds" << std::endl;
        
        return 0;
        
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: Invalid stride size. Please provide a positive integer." << std::endl;
        return 1;
    } catch (const std::out_of_range& e) {
        std::cerr << "Error: Stride size is too large." << std::endl;
        return 1;
    }
}
