#ifndef LOTTO_HANDLER_RACE_H
#define LOTTO_HANDLER_RACE_H
#include <stdbool.h>

#include <lotto/base/task_id.h>

struct race_loc {
    task_id id;
    uintptr_t pc;
    bool readonly;
};

typedef struct race {
    uintptr_t addr;
    struct race_loc loc1;
    struct race_loc loc2;
} race_t;

#endif
