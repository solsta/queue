#include <assert.h>
#include <atomic>
#include <stdio.h>
#include <string.h>
#include<jemalloc/jemalloc.h>
#include <thread>
#include <vector>

#define PAYLOAD_SIZE 128
constexpr uint64_t POINTER_MASK = 0x0000FFFFFFFFFFFF;  // Lower 48 bits
constexpr uint64_t COUNTER_MASK = 0xFFFF000000000000;  // Upper 16 bits

class Node;
class Queue;

uint64_t pack(Node* ptr, uint16_t counter);
Node* unpack_ptr(uint64_t packed);
uint16_t unpack_counter(uint64_t packed);
void enqueue(Queue *q, char *payload, int payloadSize);
bool dequeue(Queue *q, char *response);

class Pointer{
public:
Node *ptr;
uint16_t counter;


    Pointer(Node *ptr, int counter){
       this->ptr = ptr;
       this->counter = counter;
    }
    bool operator==(const Pointer& other) const {
        return ptr == other.ptr && counter == other.counter;
    }
    void setPointer(Node *ptr) const{
        ptr = reinterpret_cast<Node*>(reinterpret_cast<uint64_t>(ptr) & POINTER_MASK);
    }
    void setCounter(uint16_t newValue) const{
        
    }
};


class Node{

public:
    std::atomic<uint64_t> next;
    char nodePayload[PAYLOAD_SIZE];
    Node(char *payload, int payloadSize){
        assert(payloadSize <= PAYLOAD_SIZE);
        memcpy(nodePayload, payload, payloadSize);
        next = pack(NULL, 0);
    }
};


class Queue{
public:
    std::atomic<uint64_t> head; // This is actually a packed Pointer
    std::atomic<uint64_t> tail;

    Queue(){
        Node *node = new Node("", strlen(""));
        uint64_t nodePtr = pack(node,0);
        head = tail = nodePtr;
    }
};