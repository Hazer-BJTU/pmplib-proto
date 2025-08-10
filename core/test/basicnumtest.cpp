#include <latch>
#include <chrono>
#include "Basics.h"
#include "TaskHandler.h"

constexpr size_t DATA_LOG_LEN_MIN = 8;
constexpr size_t DATA_LOG_LEN_MAX = 20;
constexpr size_t TASK_NUM = 1000;

std::random_device seed_generator;

int main() {
    putils::ThreadPool::TaskList task_list;
    std::latch task_latch{TASK_NUM};
    for (size_t i = 0; i < TASK_NUM; i++) {
        task_list.emplace_back(putils::wrap_task([&task_latch] { 
            std::mt19937 gen(seed_generator());
            std::uniform_int_distribution<uint64_t> udist_value;
            std::uniform_int_distribution<size_t> udist_log_len(DATA_LOG_LEN_MIN, DATA_LOG_LEN_MAX);
            size_t log_len = udist_log_len(gen);
            mpengine::BasicIntegerType X(log_len), Y(log_len);
            mpengine::BasicIntegerType::ElementType *x = X.get_pointer(), *y = Y.get_pointer();
            for (int i = 0; i < X.len; i++) {
                x[i] = udist_value(gen);
                y[i] = x[i];
            }
            for (int i = X.len - 1; i >= 0; i--) {
                if (x[i] != y[i]) {
                    throw PUTILS_GENERAL_EXCEPTION("Unexpected data modification!", "test error");
                    std::abort();
                }
            }
            task_latch.count_down();
        }));
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    putils::ThreadPool::get_global_threadpool().submit(task_list);
    task_latch.wait();
    putils::ThreadPool::get_global_threadpool().shutdown();
    auto end_time = std::chrono::high_resolution_clock::now();

    std::cout << "Test passed in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms." << std::endl;

    putils::MemoryPool::MemView view = putils::MemoryPool::get_global_memorypool().report();
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