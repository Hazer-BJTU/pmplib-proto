#include "ThreadPool.h"
#include "Prememory.h"
#include <chrono>
#include <memory>

#define MAXN 1000

class MMtask: public rpc1k::Task {
public:
    int starting, ending;
    double *A, *B, *C;
    MMtask(double *A, double *B, double *C, int starting, int ending): Task() {
        this->A = A;
        this->B = B;
        this->C = C;
        this->starting = starting;
        this->ending = ending;
    }
    ~MMtask() {}
    void run() override {
        for (int i = starting; i < ending; i++) {
            for (int j = 0; j < MAXN; j++) {
                C[i * MAXN + j] = 0.0;
                for (int k = 0; k < MAXN; k++) {
                    C[i * MAXN + j] += A[i * MAXN + k] * B[k * MAXN + j];
                }
            }
        }
        return;
    }
};

void fillWithRandomDoubles(double* arr, size_t size, double min = 0.0, double max = 1.0) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(min, max);
    std::generate(arr, arr + size, [&]() { return dist(gen); });
}

double *A, *B, *C1, *C2;

int main() {
    A = reinterpret_cast<double*>(rpc1k::aligned_alloc((size_t)64, sizeof(double) * MAXN * MAXN));
    B = reinterpret_cast<double*>(rpc1k::aligned_alloc((size_t)64, sizeof(double) * MAXN * MAXN));
    C1 = reinterpret_cast<double*>(rpc1k::aligned_alloc((size_t)64, sizeof(double) * MAXN * MAXN));
    C2 = reinterpret_cast<double*>(rpc1k::aligned_alloc((size_t)64, sizeof(double) * MAXN * MAXN));
    fillWithRandomDoubles(A, MAXN * MAXN);
    fillWithRandomDoubles(B, MAXN * MAXN);
    std::vector<std::shared_ptr<MMtask>> mmt;

    auto& threadpool = rpc1k::ThreadPool::get_global_taskHandler();
    for (int i = 0; i < MAXN; i += 4) {
        mmt.emplace_back(std::make_shared<MMtask>(A, B, C1, i, i + 4));
    }
    auto start = std::chrono::high_resolution_clock::now();
    for (auto& task_ptr: mmt) {
        threadpool.enqueue(task_ptr);
    }
    threadpool.wait_for_all_subgroups();
    auto end = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Thread pool done. Time consumed: " << total_time << std::endl;
    
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < MAXN; i++) {
        for (int j = 0; j < MAXN; j++) {
            C2[i * MAXN + j] = 0.0;
            for (int k = 0; k < MAXN; k++) {
                C2[i * MAXN + j] += A[i * MAXN + k] * B[k * MAXN + j];
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    total_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Single thread done. Time consumed: " << total_time << std::endl;

    double diff = 0.0;
    for (int i = 0; i < MAXN * MAXN; i++) {
        diff += (C1[i] - C2[i]) * (C1[i] - C2[i]);
    }
    std::cout << "Difference: " << diff << std::endl;
    rpc1k::aligned_free(A);
    rpc1k::aligned_free(B);
    rpc1k::aligned_free(C1);
    rpc1k::aligned_free(C2);
    return 0;
}
