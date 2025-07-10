#include "TaskHandler.h"
#include <latch>

constexpr size_t MATRIX_SIZE = 2000;
constexpr size_t BLOCK_SZIE = 20;

double A[MATRIX_SIZE * MATRIX_SIZE], B[MATRIX_SIZE * MATRIX_SIZE], 
       C[MATRIX_SIZE * MATRIX_SIZE], D[MATRIX_SIZE * MATRIX_SIZE];

std::random_device seed_gen;
std::mt19937 gen(seed_gen());
std::normal_distribution<double> ndist(0.0, 1.0);
void random_matrix(double* mat, size_t size = MATRIX_SIZE) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            mat[i * size + j] = ndist(gen);
        }
    }
}

double diff_matrix(double* A, double* B, size_t size = MATRIX_SIZE) {
    double diff = 0.0;
    for (int i = 0; i < size * size; i++) {
        diff += std::abs(A[i] - B[i]);
    }
    return diff;
}

int main() {
    putils::ThreadPool::set_global_threadpool();
    random_matrix(A);
    random_matrix(B);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            C[i * MATRIX_SIZE + j] = 0.0;
            for (int k = 0; k < MATRIX_SIZE; k++) {
                C[i * MATRIX_SIZE + j] += A[i * MATRIX_SIZE + k] * B[j * MATRIX_SIZE + k];
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Non-parallel execution time: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() 
              << "ms" << std::endl;

    std::latch task_latch{MATRIX_SIZE / BLOCK_SZIE};
    putils::ThreadPool::TaskList task_list;
    for (int i = 0; i < MATRIX_SIZE; i += BLOCK_SZIE) {
        task_list.emplace_back(putils::wrap_task(
            [&task_latch, size = MATRIX_SIZE, starting = i, ending = i + BLOCK_SZIE]() {
                for (int i = starting; i < ending; i++) {
                    for (int j = 0; j < size; j++) {
                        D[i * size + j] = 0.0;
                        for (int k = 0; k < size; k++) {
                            D[i * size + j] += A[i * size + k] * B[j * size + k];
                        }
                    }
                }
                task_latch.count_down();
            }
        ));
    }

    start = std::chrono::high_resolution_clock::now();

    auto& thread_pool = putils::ThreadPool::get_global_threadpool();
    thread_pool.submit(task_list);
    task_latch.wait();

    end = std::chrono::high_resolution_clock::now();
    std::cout << "Parallel execution time: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() 
              << "ms" << std::endl;    

    std::cout << "Difference: " << diff_matrix(C, D) << std::endl;

    return 0;
}