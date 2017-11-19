#ifndef LF_STACK_H_
#define LF_STACK_H_

#include <stdalign.h>
#include <stdatomic.h>
#include <stdint.h>

typedef struct lfstack_node_s lfstack_node_t;
typedef struct lfstack_s lfstack_t;

struct lfstack_node_s {
    void* value;
    lfstack_node_t* next;
};

struct lfstack_s {
    _Atomic uint64_t size;
    alignas(64) _Atomic (lfstack_node_t*) top;
};

int lfstack_init(lfstack_t* stack);
int lfstack_push(lfstack_t* stack, void* value);
void* lfstack_pop(lfstack_t* stack);
uint64_t lfstack_size(lfstack_t* stack);

#endif // LF_STACK_H_
