#include <assert.h>
#include <stdio.h>
#include <string.h>
#include<jemalloc/jemalloc.h>

#define PAYLOAD_SIZE 128
constexpr uint64_t POINTER_MASK = 0x0000FFFFFFFFFFFF;  // Lower 48 bits
constexpr uint64_t COUNTER_MASK = 0xFFFF000000000000;  // Upper 16 bits


struct Node;


typedef struct Pointer
{
    Node *ptr;
    uint16_t counter;

    bool operator==(const Pointer& other) const {
        return ptr == other.ptr && counter == other.counter;
    }
    void setPointer(Node *ptr) const{
        ptr = reinterpret_cast<Node*>(reinterpret_cast<uint64_t>(ptr) & POINTER_MASK);
    }
    void setCounter(uint16_t newValue) const{
        counter = static_cast<uint64_t>(counter) << 48;
    }
}Pointer;


typedef struct Node
{
    char payload[PAYLOAD_SIZE];
    Pointer next;
}Node;


typedef struct Queue
{
    Pointer head;
    Pointer tail;
}Queue;


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


Node *new_node(char *payload, int payloadSize){
    assert(payloadSize <= PAYLOAD_SIZE);
    Node *node = (Node*)malloc(sizeof(node));
    node->next.counter = 0;
}

void initialise(Queue *q){
   Node *node = new_node("", strlen(""));
   node->next.ptr = NULL;
   q->head.ptr = q->tail.ptr = node;
}

void enqueue(Queue *q, char *payload, int payloadSize){
    Node *node = new_node(payload, payloadSize);
    node->next.ptr = NULL;

    while(true){
        Pointer tail = q->tail;
        Pointer next = tail.ptr->next;
        if(tail == q->tail){
            if(next.ptr == NULL){ //Was tail pointing to the last node?
                pack(node, tail.counter+1);

            } else{

            }
        }
    }

}


int main() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    initialise(q);
    return 0;
}
