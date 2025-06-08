#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "Log.h"

using namespace putils;

void test_logger_with_small_buffer() {
    Logger::get_global_logger().set("test_log.txt", Logger::Level::INFO);
    
    const int num_threads = 4;
    const int messages_per_thread = 100;
    
    auto worker = [](int thread_id) {
        for (int i = 0; i < messages_per_thread; ++i) {
            std::string msg = "Thread " + std::to_string(thread_id) + 
                             " message " + std::to_string(i);
            
            // Alternate between different log levels
            Logger::Level level = Logger::Level::INFO;
            if (i % 3 == 1) level = Logger::Level::WARN;
            if (i % 3 == 2) level = Logger::Level::ERROR;
            
            Logger::get_global_logger().add(msg, level);
            
            // Add some delay to simulate real-world usage
            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();
    
    // Start worker threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }
    
    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Force final flush
    Logger::get_global_logger().add("Final message before flush", Logger::Level::INFO);
    Logger::get_global_logger().flush();
    
    std::cout << "Test completed in " << duration.count() << "ms\n";
    std::cout << "Check test_log.txt for output\n";
}

void test_log_level_filtering() {
    Logger::get_global_logger().set("level_test.txt", Logger::Level::WARN);
    
    Logger::get_global_logger().add("This INFO message should NOT appear", Logger::Level::INFO);
    Logger::get_global_logger().add("This WARN message should appear", Logger::Level::WARN);
    Logger::get_global_logger().add("This ERROR message should appear", Logger::Level::ERROR);
    
    Logger::get_global_logger().flush();
    
    std::cout << "Log level filtering test completed\n";
}

int main() {
    std::cout << "Starting Logger tests...\n";
    
    // Test 1: Small buffer capacity
    std::cout << "\n=== Testing small buffer capacity ===\n";
    test_logger_with_small_buffer();
    
    // Test 2: Log level filtering
    std::cout << "\n=== Testing log level filtering ===\n";
    test_log_level_filtering();
    
    std::cout << "\nAll tests completed.\n";
    return 0;
}