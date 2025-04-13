#include <assert.h>
#include <atomic>
#include <stdio.h>
#include <string.h>
#include<jemalloc/jemalloc.h>
#include <thread>
#include <vector>
#include "benchmark.hpp"



uint64_t pack(Node* ptr, uint16_t counter) {
    uint64_t ptrBits = reinterpret_cast<uint64_t>(ptr) & POINTER_MASK;
    uint64_t countBits = static_cast<uint64_t>(counter) << 48;
    return ptrBits | countBits;

}

Node* unpack_ptr(uint64_t packed) {
    return reinterpret_cast<Node*>(packed & POINTER_MASK);

}

uint16_t unpack_counter(uint64_t packed) {
    return static_cast<uint16_t>(packed >> 48);

}

void enqueue(Queue *q, char *payload, int payloadSize){
    Node *node = new Node(payload, payloadSize);
    uint16_t cachedTailCounter;
    uint64_t cachedTail;
    Node *cachedTailPtr;
    while(true){
        cachedTail = q->tail; // Cache the pointer and counter
        cachedTailPtr = unpack_ptr(cachedTail);
        cachedTailCounter = unpack_counter(cachedTail);

        uint64_t packedNext =  cachedTailPtr->next;

        Node *nextNode = unpack_ptr(packedNext);

        if(cachedTail == q->tail){
            if(nextNode == NULL){ //Was tail pointing to the last node?
                bool res = cachedTailPtr->next.compare_exchange_strong(packedNext, pack(node, cachedTailCounter+1));
                if(res){
                    break;
                }
            } else{
                q->tail.compare_exchange_strong(cachedTail, pack(nextNode, cachedTailCounter+1));
            }
        }
    }
    q->tail.compare_exchange_strong(cachedTail, pack(node, cachedTailCounter+1));

}

bool dequeue(Queue *q, char *response){
    Node *cachedHeadPointer;
    while (true)
    {
        uint64_t cachedHead = q->head;
        uint64_t cachedTail = q->tail;

        cachedHeadPointer = unpack_ptr(cachedHead);
        Node *cachedTailPointer = unpack_ptr(cachedTail);

        uint64_t packedNext =  cachedHeadPointer->next;
        Node *nextNode = unpack_ptr(packedNext);

        uint64_t cachedTailCounter = unpack_counter(cachedTail);
        uint64_t cachedHeadCounter = unpack_counter(cachedHead);

        if(cachedHead == q->head){
            if(cachedHeadPointer == cachedTailPointer){
                if(nextNode == NULL){
                    return false;
                }
                q->tail.compare_exchange_strong(cachedTail,pack(nextNode,cachedTailCounter+1)); 
            } else{
#ifdef RETRUN_RESULT_FOR_DEQUEUE
                memcpy(response,nextNode->nodePayload, PAYLOAD_SIZE);
#endif
                if(q->head.compare_exchange_strong(cachedHead,pack(nextNode,cachedHeadCounter+1))){
                    break;
                }
            }
        }
    }
    delete cachedHeadPointer;
    return true;

}

void run_tests(Queue* q){
    
    char buffer[PAYLOAD_SIZE];

    // Test: dequeue from empty queue
    assert(dequeue(q, buffer) == false);

    // Test: enqueue one element, then dequeue
    enqueue(q, (char*)"OnlyOne", strlen("OnlyOne"));
    assert(dequeue(q, buffer));
    assert(strcmp(buffer, "OnlyOne") == 0);
    assert(dequeue(q, buffer) == false);

    // Test: payload size at PAYLOAD_SIZE boundary
    char longPayload[PAYLOAD_SIZE];
    for (int i = 0; i < PAYLOAD_SIZE - 1; ++i)
        longPayload[i] = 'x';
    longPayload[PAYLOAD_SIZE - 1] = '\0';
    enqueue(q, longPayload, strlen(longPayload));
    assert(dequeue(q, buffer));
    assert(strcmp(buffer, longPayload) == 0);

    // Test: enqueue and dequeue 1000 elements
    for (int i = 0; i < 1000; ++i) {
        char msg[32];
        sprintf(msg, "Message %d", i);
        enqueue(q, msg, strlen(msg));
    }

    for (int i = 0; i < 1000; ++i) {
        assert(dequeue(q, buffer));
        char expected[32];
        sprintf(expected, "Message %d", i);
        assert(strcmp(buffer, expected) == 0);
    }

    assert(dequeue(q, buffer) == false);
    printf("Functional tests passed!\n");
}

void threaded_test(Queue* q) {
    const int N = 100000;
    const int THREADS = 30;
    std::atomic<uint64_t> receivedCount;
    receivedCount = 0;
    // Launch producer threads
    std::vector<std::thread> producers;
    for (int t = 0; t < THREADS; ++t) {
        producers.emplace_back([=]() {
            for (int i = 0; i < N; ++i) {
                char msg[64];
                sprintf(msg, "Thread %d Msg %d", t, i);
                enqueue(q, msg, strlen(msg));
            }
        });
    }

    // Launch consumer thread

    std::thread consumer([&]() {
        char buffer[PAYLOAD_SIZE];
        while (receivedCount < N * THREADS) {
            if (dequeue(q, buffer)) {
                receivedCount++;
                // Optionally verify content
            }
        }
    });

    for (auto& p : producers) p.join();
    consumer.join();
    char buffer[PAYLOAD_SIZE];
    assert(receivedCount == N * THREADS);
    assert(!dequeue(q,buffer));
    printf("Multi-threaded test passed!\n");
}

int main() {
    Queue *q = new Queue();
    //run_tests(q);
    //threaded_test(q);

    //benchmark_queue(30, 1000000, q);
    return 0;
}
