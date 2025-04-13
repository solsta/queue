#include <assert.h>
#include <atomic>
#include <stdio.h>
#include <string.h>
#include<jemalloc/jemalloc.h>

#define PAYLOAD_SIZE 128
constexpr uint64_t POINTER_MASK = 0x0000FFFFFFFFFFFF;  // Lower 48 bits
constexpr uint64_t COUNTER_MASK = 0xFFFF000000000000;  // Upper 16 bits

class Node;
uint64_t pack(Node* ptr, uint16_t counter);
Node* unpack_ptr(uint64_t packed);
uint16_t unpack_counter(uint64_t packed);

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
private:
    char nodePayload[PAYLOAD_SIZE];
    
public:
    std::atomic<uint64_t> next;
    Node(char *payload, int payloadSize){
        assert(payloadSize <= PAYLOAD_SIZE);
        memcpy(payload, nodePayload, payloadSize);
        next = pack(NULL, 0);
    }
    void setNext(Node *n){
        uint16_t counter = unpack_counter(next);
        counter+=1;
        next = pack(n,counter);
    }
    Node *getNext(){
        return unpack_ptr(next);
    }
    Pointer *getPtrNext(){
        Node *nodePtr = unpack_ptr(next);
        uint16_t counter = unpack_counter(next);
        Pointer *p = new Pointer(nodePtr, counter);
        return p;
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


int main() {
    Queue *q = new Queue();

    printf("Enqueu done!\n");
    return 0;
}
