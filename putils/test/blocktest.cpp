#include <latch>
#include <cassert>
#include "TaskHandler.h"
#include "MemoryAllocator.h"

using namespace std;

constexpr size_t total_size = 4 * 1024 * 1024;
constexpr int num_test = 1000;
constexpr int num_tasks = 128;

std::random_device rd;

int main() {
    using ull = unsigned long long;
    auto memory_pool = putils::MemBlock::make_head(total_size);

    std::latch task_latch{num_tasks};
    putils::ThreadPool::TaskList task_list;
    for (int i = 0; i < num_tasks; i++) {
        task_list.emplace_back(putils::wrap_task([&memory_pool, &task_latch, i]() {
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> udist(512, 4096);
            std::uniform_int_distribution<uint64_t> udist4num;
            auto& logger = putils::RuntimeLog::get_global_log();

            for (int j = 0; j < num_test; j++) {
                int num = udist(gen);
                auto handle = putils::MemBlock::assign(memory_pool, num * sizeof(ull));
                if (!handle) {
                    std::stringstream ss;
                    ss << "Task #" << i << " allocation attempt " << j << ": " << num << " x ull failed.";
                    logger.add(ss.str());
                    putils::MemBlock::compact(memory_pool);
                    continue;
                }
                ull* arr = handle->get<ull>();
                assert(handle->length() >= num * sizeof(ull));
                for (int k = 0; k < num; k++) {
                    arr[k] = udist4num(gen);
                }
                handle->release();
            }

            task_latch.count_down();
        }));
    }

    putils::ThreadPool::get_global_threadpool().submit(task_list);
    task_latch.wait();
    putils::ThreadPool::get_global_threadpool().shutdown();

    putils::MemBlock::compact(memory_pool);
    auto& logger = putils::RuntimeLog::get_global_log();
    auto handle = putils::MemBlock::assign(memory_pool, total_size);
    assert(handle != nullptr);
    std::stringstream ss;
    ss << "block content = { ";
    char* arr = handle->get<char>();
    for (int i = 0; i < handle->length(); i++) {
        ss << static_cast<unsigned int>(arr[i]);
        if (i + 1 != handle->length()) {
            ss << ", ";
        }
    }
    ss << " }" << std::endl;
    logger.add(ss.str());
    return 0;
}
