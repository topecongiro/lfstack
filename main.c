#include "lfstack.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(void)
{
    uint64_t thread_num = 8;
    uintptr_t total_job = 1000000;
    bool* a;
    lfstack_t* stack;
    pthread_t thread[thread_num];
    job_arg_t* job_arg;

    initialize_globals(&a, &stack, total_job);

    job_arg = setup_job_arg(total_job, thread_num, stack, a);

    for (uint64_t i = 0; i < thread_num; i++) {
        pthread_create(&thread[i], NULL, job, job_arg);
    }
    for (uint64_t i = 0; i < thread_num; i++) {
        pthread_join(thread[i], NULL);
    }
    verify_job(a, total_job);

    free_globals(a, stack);

    return 0;
}
