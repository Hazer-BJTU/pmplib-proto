#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <mutex>
#include "LockFreeQueue.hpp"

using namespace putils;

constexpr size_t QUEUE_SIZE = 4096;
constexpr size_t PRODUCER_COUNT = 4;
constexpr size_t CONSUMER_COUNT = 4;
constexpr size_t ITEMS_PER_PRODUCER = 10000;

struct TestData {
    int producer_id;
    int item_id;
    double value;
};

std::atomic<int> produced_count(0);
std::atomic<int> consumed_count(0);
std::atomic<int> push_failures(0);
std::atomic<int> pop_failures(0);

std::vector<std::vector<int>> consumed_items(PRODUCER_COUNT);
std::mutex consumed_items_mutex;

std::mutex cout_mutex;

void producer_thread(LockFreeQueue<TestData>& queue, int producer_id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
        TestData data;
        data.producer_id = producer_id;
        data.item_id = i;
        data.value = dis(gen);

        while (!queue.try_enqueue(data.producer_id, data.item_id, data.value)) {
            push_failures++;
            std::this_thread::yield();
        }

        produced_count++;
    }

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Producer " << producer_id << " finished. Produced: " << ITEMS_PER_PRODUCER << std::endl;
}

void consumer_thread(LockFreeQueue<TestData>& queue, int consumer_id) {
    std::shared_ptr<TestData> data;
    int local_consumed_count = 0;

    while (consumed_count < PRODUCER_COUNT * ITEMS_PER_PRODUCER) {
        if (queue.try_pop(data)) {
            if (data) {
                std::lock_guard<std::mutex> lock(consumed_items_mutex);
                
                if (data->producer_id >= 0 && data->producer_id < PRODUCER_COUNT &&
                    data->item_id >= 0 && data->item_id < ITEMS_PER_PRODUCER) {
                    consumed_items[data->producer_id].push_back(data->item_id);
                } else {
                    std::lock_guard<std::mutex> cout_lock(cout_mutex);
                    std::cerr << "Consumer " << consumer_id << ": Invalid data consumed!" << std::endl;
                }
                consumed_count++;
                local_consumed_count++;
            }
        } else {
            pop_failures++;
            std::this_thread::yield();
        }
    }

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Consumer " << consumer_id << " finished. Consumed: " << local_consumed_count << std::endl;
}

int main() {
    try {
        LockFreeQueue<TestData> queue(QUEUE_SIZE);

        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> producers;
        for (int i = 0; i < PRODUCER_COUNT; ++i) {
            producers.emplace_back(producer_thread, std::ref(queue), i);
        }

        std::vector<std::thread> consumers;
        for (int i = 0; i < CONSUMER_COUNT; ++i) {
            consumers.emplace_back(consumer_thread, std::ref(queue), i);
        }

        for (auto& producer : producers) {
            producer.join();
        }

        for (auto& consumer : consumers) {
            consumer.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        bool all_correct = true;
        for (int i = 0; i < PRODUCER_COUNT; ++i) {
            if (consumed_items[i].size() != ITEMS_PER_PRODUCER) {
                std::cerr << "Producer " << i << ": missing items! Expected " 
                          << ITEMS_PER_PRODUCER << ", got " << consumed_items[i].size() << std::endl;
                all_correct = false;
                continue;
            }

            std::vector<bool> seen(ITEMS_PER_PRODUCER, false);
            for (int item : consumed_items[i]) {
                if (item < 0 || item >= ITEMS_PER_PRODUCER) {
                    std::cerr << "Producer " << i << ": invalid item id " << item << std::endl;
                    all_correct = false;
                } else if (seen[item]) {
                    std::cerr << "Producer " << i << ": duplicate item " << item << std::endl;
                    all_correct = false;
                }
                seen[item] = true;
            }
        }

        std::cout << "\nTest completed in " << duration.count() << " ms" << std::endl;
        std::cout << "Produced items: " << produced_count << std::endl;
        std::cout << "Consumed items: " << consumed_count << std::endl;
        std::cout << "Push failures (queue full): " << push_failures << std::endl;
        std::cout << "Pop failures (queue empty): " << pop_failures << std::endl;
        std::cout << "Result verification: " << (all_correct ? "PASSED" : "FAILED") << std::endl;

        return all_correct ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
