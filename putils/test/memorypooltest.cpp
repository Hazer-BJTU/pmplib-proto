#include <latch>
#include <cassert>
#include "TaskHandler.h"
#include "MemoryAllocator.h"

using namespace std;

constexpr size_t num_tasks = 128;
constexpr size_t test_per_task = 1024;
constexpr auto level = putils::RuntimeLog::Level::INFO;
std::random_device seed_gen;

int main() {
    using ull = unsigned long long;
    putils::MemoryPool::set_global_memorypool();
    auto& memory_pool = putils::MemoryPool::get_global_memorypool();
    putils::ThreadPool::set_global_threadpool();
    auto& thread_pool = putils::ThreadPool::get_global_threadpool();
    putils::RuntimeLog::set_global_log("runtime_log.txt", level, 4096);
    auto& logger = putils::RuntimeLog::get_global_log();

    std::latch task_latch{num_tasks}; 
    putils::ThreadPool::TaskList task_list;
    for (size_t i = 0; i < num_tasks; i++) {
        task_list.push_back(putils::wrap_task([&memory_pool, &task_latch, &logger, i]() {
            std::mt19937 gen(seed_gen());
            std::uniform_int_distribution<size_t> udist_release(1, 10);
            std::uniform_int_distribution<size_t> udist_num(1, 4096);
            std::uniform_int_distribution<uint64_t> udist_val;
            for (size_t j = 0; j < test_per_task; j++) {
                size_t total_num = udist_num(gen);
                auto handle = memory_pool.allocate(total_num * sizeof(ull));
                assert(handle && handle->length() >= total_num * sizeof(ull));
                ull* arr = handle->get<ull>();
                for (size_t k = 0; k < handle->length<ull>(); k++) {
                    arr[k] = udist_val(gen);
                }
                if (udist_release(gen) >= 3) {
                    putils::release(handle);
                }
                std::stringstream ss;
                ss << "Task #" << i << ": attempt " << j << ": " << total_num << " unsigned long long(s).";
                logger.add(ss.str());
            }
            task_latch.count_down();
        }));
    }
    
    auto start = std::chrono::high_resolution_clock::now();

    thread_pool.submit(task_list);
    task_latch.wait();
    thread_pool.shutdown();

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Test time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    putils::MemoryPool::MemView view = memory_pool.report();
    std::cout << "Total bytes: " << view.bytes_total << std::endl;
    std::cout << "Num blocks: " << view.num_blocks << std::endl;
    std::cout << "Average block size: " << view.avg_block_size << std::endl;
    std::cout << "Min block size: " << view.min_block_size << std::endl;
    std::cout << "Max block size: " << view.max_block_size << std::endl;
    std::cout << "Bytes in use: " << view.bytes_in_use << std::endl;
    std::cout << "Usage ratio: " << std::fixed << std::setprecision(2) << view.usage_ratio * 100 << "\%" << std::endl;
    std::cout << "Total memory usage: " << putils::human(view.bytes_total) << std::endl;
    return 0;
}