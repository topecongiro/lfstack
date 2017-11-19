#include "lfstack.h"
#include "ftebr.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static lfstack_node_t*
lfstack_node_new(void* value)
{
    lfstack_node_t* new_node = malloc(sizeof(lfstack_node_t));
    new_node->value = value;
    return new_node;
}

int
lfstack_init(lfstack_t* stack)
{
    stack->top = NULL;
    return 0;
}

int
lfstack_push(lfstack_t* stack, void* value)
{
    lfstack_node_t* old_top;
    lfstack_node_t* new_node = lfstack_node_new(value);

    if ((int)value % 1000 == 0)
        printf("thread %lu: lfstack_push(%d)\n", pthread_self(), (int)value);
    ftebr_enter();
    do {
        old_top = atomic_load(&stack->top);
        new_node->next = old_top;
    } while (!atomic_compare_exchange_weak(&stack->top, &old_top, new_node));
    ftebr_exit();

    return 0;
}

void*
lfstack_pop(lfstack_t* stack)
{
    lfstack_node_t* old_top;
    lfstack_node_t* expected;
    void* value = NULL;

    ftebr_enter();
    do {
        old_top = atomic_load(&stack->top);
        if (old_top == NULL)
            break;
        expected = old_top->next;
        value = old_top->value;
    } while (!atomic_compare_exchange_weak(&stack->top, &old_top, expected));
    ftebr_free(old_top);
    ftebr_exit();

    return value;
}

uint64_t
lfstack_size(lfstack_t* stack)
{
    return atomic_load_explicit(&stack->size, memory_order_relaxed);
}
