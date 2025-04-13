#include "ThreadPool.h"

namespace rpc1k {

std::mt19937 Task::gen(DEFAULT_RANDOM_SEED);
std::uniform_int_distribution<int> Task::udist(UDIST_LOWER_BOUND, UDIST_UPPER_BOUND);

Task::Task() {
    task_idx = udist(gen);
}

Task::~Task() {}

void Task::run() {
    return;
}

int ThreadPool::total_workers = 16;
int ThreadPool::num_queues = 2;
int ThreadPool::maxn = 256;

bool ThreadPool::setGlobalTaskHandlerConfig(int total_workers, int num_queues, int maxn) {
    return true;
}

}