#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include "queue.hpp"

void benchmark_queue(int numThreads, int opsPerThread, Queue* q) {
    std::atomic<int> totalDequeued(0);

    const int totalOps = numThreads * opsPerThread;
    char *payload = static_cast<char*>(malloc(PAYLOAD_SIZE));
    

    auto startTime = std::chrono::high_resolution_clock::now();

    // Producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < numThreads; ++i) {
        producers.emplace_back([=]() {
            for (int j = 0; j < opsPerThread; ++j) {
                enqueue(q, payload, PAYLOAD_SIZE);
            }
        });
    }

    // Consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < numThreads; ++i) {
        consumers.emplace_back([&]() {
            char buf[PAYLOAD_SIZE];
            int localCount = 0;
            for (int j = 0; j < opsPerThread; ++j) {
                dequeue(q, buf);
            }
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedSec = std::chrono::duration<double>(endTime - startTime).count();

    printf("Threads: %d producers + %d consumers\n", numThreads, numThreads);
    printf("Total operations: %d\n", totalOps);
    printf("Elapsed time: %.6f seconds\n", elapsedSec);
    printf("Throughput: %.2f ops/sec\n", totalOps / elapsedSec);
}