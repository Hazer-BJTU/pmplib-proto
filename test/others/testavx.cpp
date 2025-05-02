#include <immintrin.h>  // AVX2指令集头文件
#include <iostream>
#include <vector>
#include <numeric>
#include <chrono>

float normal_sum(const float* array, size_t size) {
    float sum = 0.0f;
    for (size_t i = 0; i < size; ++i) {
        sum += array[i];
    }
    return sum;
}

// 辅助函数：水平求和
float horizontal_sum(__m256 vec) {
    // 将高128位和低128位相加
    __m128 low128 = _mm256_extractf128_ps(vec, 0);
    __m128 high128 = _mm256_extractf128_ps(vec, 1);
    __m128 sum128 = _mm_add_ps(low128, high128);
    
    // 继续水平相加
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    return _mm_cvtss_f32(sum128);
}

float avx2_sum(const float* array, size_t size) {
    // 每个AVX2寄存器可以容纳8个float(256位/32位=8)
    constexpr size_t avx2_float_num = 8;
    
    // 初始化累加器
    __m256 sum_vec = _mm256_setzero_ps();
    
    // 处理能被8整除的部分
    size_t i = 0;
    for (; i + avx2_float_num <= size; i += avx2_float_num) {
        __m256 data = _mm256_loadu_ps(array + i);  // 加载8个float
        sum_vec = _mm256_add_ps(sum_vec, data);    // 向量加法
    }
    
    // 水平求和：将8个float相加
    float sum = horizontal_sum(sum_vec);
    
    // 处理剩余不足8个的元素
    for (; i < size; ++i) {
        sum += array[i];
    }
    
    return sum;
}

int main() {
    const size_t size = 1024 * 1024 * 64;  // 64M个float
    std::vector<float> data(size, 1.0f);   // 初始化全1数组
    
    // 测试普通求和
    auto start = std::chrono::high_resolution_clock::now();
    float sum_normal = normal_sum(data.data(), size);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Normal sum: " << sum_normal 
              << " Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
    
    // 测试AVX2求和
    start = std::chrono::high_resolution_clock::now();
    float sum_avx2 = avx2_sum(data.data(), size);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "AVX2 sum: " << sum_avx2 
              << " Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
    
    return 0;
}
