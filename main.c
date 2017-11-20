#include "ftebr.h"
#include "lfstack.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct job_arg_s {
    _Atomic uint64_t* counter;
    uint64_t loop_num;
    lfstack_t* stack;
    bool* a;
} job_arg_t;

void*
job(void* arg)
{
    job_arg_t job_arg = *(job_arg_t*)arg;
    lfstack_t* stack = job_arg.stack;

    ftebr_register();

    for (uint64_t i = 0; i < job_arg.loop_num; i++) {
        uint64_t value = atomic_fetch_add(job_arg.counter, 1);
        lfstack_push(stack, (void*)value);
    }

    for (uint64_t i = 0; i < job_arg.loop_num; i++) {
        void* value = lfstack_pop(stack);
        job_arg.a[(uint64_t)value] = true;
    }

    return NULL;
}

job_arg_t*
setup_job_arg(uint64_t total_job, uint64_t thread_num, lfstack_t* stack, bool* a)
{
    job_arg_t* job_arg = malloc(sizeof(job_arg_t));

    job_arg->counter = malloc(sizeof(uint64_t));
    job_arg->loop_num = total_job / thread_num;
    job_arg->stack = stack;
    job_arg->a = a;

    return job_arg;
}

bool
verify_job(bool* a, uint64_t total_job)
{
    for (uint64_t i = 0; i < total_job; i++) {
        if (!a[i])
            return false;
    }

    return true;
}

void
initialize_globals(bool** a, lfstack_t** stack, uint64_t total_job)
{
    *stack = malloc(sizeof(lfstack_t));
    *a = malloc(sizeof(bool) * total_job);
    for (uint64_t i = 0; i < total_job; i++)
        (*a)[i] = false;
}

void
free_globals(bool* a, lfstack_t* stack)
{
    free(a);
    free(stack);
}


typedef struct multi_job_arg_s multi_job_arg_t;
struct multi_job_arg_s {
    uint64_t id;
    uint64_t thread_num;
    uint64_t loop_num;
    lfstack_t* lfstack;
    _Atomic uint64_t* counter;
    _Atomic uint64_t* done;
};

void* multi_job(void* arg) {
    multi_job_arg_t multi_job_arg = *(multi_job_arg_t*)arg;
    uint64_t id = multi_job_arg.id;
    uint64_t loop_num = multi_job_arg.loop_num;
    uint64_t thread_num = multi_job_arg.thread_num;
    lfstack_t* lfstack = multi_job_arg.lfstack;
    _Atomic uint64_t* counter = multi_job_arg.counter;
    _Atomic uint64_t* done = multi_job_arg.done;
    assert(lfstack != NULL);
    assert(counter != NULL);
    assert(done != NULL);
    printf("id = %lu, loop_num = %lu, thread_num = %lu, stack = %p, counter(%p) = %lu, done(%p) = %lu\n",
           id, loop_num, thread_num, lfstack, counter, *counter, done, *done);

    ftebr_register();

    if (id % thread_num == 0) {
        // Victim! Spin for a while
        while (atomic_load_explicit(done, memory_order_relaxed) != thread_num - 1) {}
        ftebr_print();
    } else {
        for (uint64_t i = 0; i < loop_num; i++) {
            lfstack_push(lfstack, (void*)atomic_fetch_add(counter, 1));
        }

        for (uint64_t i = 0; i < loop_num; i++) {
            lfstack_pop(lfstack);
        }

        atomic_fetch_add(done, 1);
    }

    ftebr_unregister();

    return NULL;
}

void
multi_test() {
    uint64_t id = 0;
    uint64_t thread_num = 4;
    uintptr_t total_job = 20000;
    _Atomic uint64_t counter = 0;
    _Atomic uint64_t done = 0;
    lfstack_t stack;
    multi_job_arg_t job_arg[thread_num];
    pthread_t thread[thread_num];

    lfstack_init(&stack);

    for (uint64_t i = 0; i < thread_num; i++) {
        job_arg[i].id = id++;
        job_arg[i].thread_num = thread_num;
        job_arg[i].loop_num = total_job / thread_num;
        job_arg[i].lfstack = &stack;
        job_arg[i].counter = &counter;
        job_arg[i].done = &done;
    }

    for (uint64_t i = 0; i < thread_num; i++) {
        pthread_create(&thread[i], NULL, multi_job, &job_arg[i]);
    }

    for (uint64_t i = 0; i < thread_num; i++) {
        pthread_join(thread[i], NULL);
    }
}

void
simple_test() {
    uint64_t thread_num = 4;
    uintptr_t total_job = 16388;
    bool* a;
    lfstack_t* stack;
    pthread_t thread[thread_num];
    job_arg_t* job_arg;

    initialize_globals(&a, &stack, total_job);

    job_arg = setup_job_arg(total_job, thread_num, stack, a);

    puts("start");
    for (uint64_t i = 0; i < thread_num; i++) {
        pthread_create(&thread[i], NULL, job, job_arg);
    }
    for (uint64_t i = 0; i < thread_num; i++) {
        pthread_join(thread[i], NULL);
    }
    puts("done");
    assert(verify_job(a, total_job));

    free_globals(a, stack);
}

int main(void)
{
    // simple_test();
    multi_test();

    return 0;
}
