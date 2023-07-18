/**
 * @file coroutine.c
 * @brief Coroutine implementation.
 * @note This coroutine is used in single chip microcomputer.
 *       Coroutine concurrency is implemented here, but not true concurrency, just switching tasks in a single thread.
 *       All tasks appear to be executing simultaneously, but they are actually switching execution in a single thread.
 *       Then using this library is like using a bare shoe, only it makes complex code look simpler.
 *       Bare metal does not have resource access conflicts, so it does not need any synchronization mechanism (mutex, semaphore).
 *       Because of the high cost of RTOS, simple tasks or low-end chips can not use RTOS, especially complex logic/high-end chips directly on linux, there is no need for RTOS.

                                      .                            .
         .......  .                  ...             .           ....
       ...      ...                   ..            ..            ...
      ..         ..                   ..            ...            ..
     ...          .                   ..           ....            ..
    ...           .                   ..           ....            ..
    ...                               ..           . ...           ..
   ...               .....     ...    ..          ..  ..           ..       .....        .   ...
   ...                ....      ..    ..          .   ...          ..      ..   ...    ...  .....
   ...                 ..       .     ..         ..   ...          ..     ..     ...    ....   ...
   ...                 ...     ..     ..         .     ...         ..    ..       ..    ...     ..
   ...                  ..     .      ..        ..     ...         ..    ..       ...   ..      ..
   ...                  ...    .      ..        ..     ...         ..    ..       ...   ..      ..
   ...                   ..   ..      ..        ...........        ..    ..       ...   ..      ..
   ...                   ...  .       ..       ..       ...        ..    ..       ...   ..      ..
    ...                   ..  .       ..       .         ...       ..    ..       ...   ..      ..
    ...                   ....        ..      ..         ...       ..    ...      ..    ..      ..
     ...          .        ...        ..      .           ...      ..     ..      ..    ..      ..
      ....      ..         ...        ..     ..           ...      ..     ...    ..     ...     ..
        ........            .        ....   ....         .....    ....     .......     ....    ....
           ..               .                                                 .
                           .
                           .
                          .
                      .....
                      ....


*/

#include "coroutine.h"

static void CorExe(void);

#define COROUTINE_MAX_SIZE (32)
CorHandle_t CorIdleHandle = 0;
struct
{
    Coroutine_t *coroutine;
    uint32_t (*getTick1ms)(void);
    uint32_t tick;
    struct
    {
        uint8_t cap : 5;
        uint8_t currid : 5;
        uint8_t alreadyInit : 1;
    } bits;
} param;

static uint8_t CorGetNextId(uint8_t currid)
{
    return (currid + 1) % param.bits.cap;
}

bool CorInit(uint8_t cap, uint32_t (*getTick1ms)(void))
{
    assert_param(getTick1ms != NULL);
    if (cap > COROUTINE_MAX_SIZE - 1)
    {
        return false;
    }
    param.bits.cap = cap + 1;
    param.coroutine = (Coroutine_t *)malloc(sizeof(Coroutine_t) * param.bits.cap);
    if (param.coroutine == NULL)
    {
        return false;
    }
    memset(param.coroutine, 0, sizeof(Coroutine_t) * param.bits.cap);
    param.getTick1ms = getTick1ms;
    param.tick = 0;
    param.bits.currid = 0;
    param.bits.alreadyInit = 1;
    CorCreateTask(&CorIdleHandle, CorIdleFunc, NULL);
    return true;
}
void CorDeinit(void) {
    if (param.coroutine != NULL) {
        free(param.coroutine);
        param.coroutine = NULL;
    }
    // Reset all other states
    param.bits.alreadyInit = 0;
    // Other states reset as needed
}

bool CorCreateTask(CorHandle_t *handle, void (*func)(void *), void *arg)
{
    static uint8_t id = 0;
    assert_param(handle != NULL);
    assert_param(func != NULL);
    if (param.bits.alreadyInit == 0 || id >= param.bits.cap)
    {
        return false;
    }
    param.coroutine[id].func = func;
    param.coroutine[id].arg = arg;
    param.coroutine[id].bits.state = COR_CREATED;
    param.coroutine[id].bits.swstate = SW_NORMAL;
    param.coroutine[id].timeout = 0;
    param.coroutine[id].label = NULL;

    *handle = id;
    id += 1;
    return true;
}

void CorSetSwState(SwState state)
{
    uint8_t id = param.bits.currid;
    param.coroutine[id].bits.swstate = state;
}
void Yield(void *label, CorState state, uint32_t timeout)
{
    uint8_t id = param.bits.currid;
    param.coroutine[id].bits.state = state;
    param.coroutine[id].timeout = timeout;
    param.coroutine[id].label = label;
    param.coroutine[id].bits.swstate = SW_ABORT;
}
void Suspend(CorHandle_t *handle)
{
    uint8_t id = *handle;
    if (handle == NULL)
    {
        id = param.bits.currid;
    }
    param.coroutine[id].bits.state = COR_SUSPEND;
    param.coroutine[id].timeout = 0;
    param.coroutine[id].bits.swstate = SW_ABORT;
}
void Resume(CorHandle_t *handle)
{
    uint8_t id = *handle;
    if (handle == NULL)
    {
        id = param.bits.currid;
    }
    param.coroutine[id].bits.state = COR_READY;
    param.coroutine[id].timeout = 0;
}

void MuxLock(MutexHandle_t *handle)
{
    assert_param(handle != NULL);
    if (*handle == 0)
    {
        *handle |= 1 << param.bits.currid;
    }
    else
    {
        param.coroutine[param.bits.currid].bits.state = COR_BLOCKED;
        param.coroutine[param.bits.currid].bits.swstate = SW_ABORT;
    }
}
void MuxUnlock(MutexHandle_t *handle)
{
    assert_param(handle != NULL);
    *handle &= ~(1 << param.bits.currid);
}


void *CorBegin(void *label)
{
    uint8_t id = param.bits.currid;
    if (param.coroutine[id].bits.swstate == SW_NORMAL)
    {
        param.coroutine[id].bits.swstate = SW_ABORT;
        param.coroutine[id].label = label;
    }
    return param.coroutine[id].label;
}

// CorState CorGetState(CorHandle_t *handle)
// {
//     uint8_t id = *handle;
//     if (handle == NULL)
//     {
//         id = param.bits.currid;
//     }
//     return param.coroutine[id].bits.state;
// }
// uint8_t CorGetId(void)
// {
//     return param.bits.currid;
// }

bool CorRun(void)
{
    if(param.bits.alreadyInit == 0)
    {
        return false;
    }
    param.bits.currid = 0;
    for (int i = 0; i < param.bits.cap; i++)
    {
        if (param.coroutine[i].bits.state != COR_NONE)
            param.coroutine[i].bits.state = COR_READY;
    }
    param.tick = param.getTick1ms();
    for (;;)
    {
        CorDispatch();
        CorExe();
    }
    return true;
}

void CorProcessTime(void)
{
    uint32_t tick = param.getTick1ms();
    uint32_t counter = tick - param.tick;
    for (int i = 0; i < param.bits.cap; i++)
    {
        if (param.coroutine[i].bits.state != COR_WAITING)
        {
            continue;
        }
        
        if (param.coroutine[i].timeout > counter)
        {
            param.coroutine[i].timeout -= counter;
        }
        else
        {
            param.coroutine[i].timeout = 0;
            param.coroutine[i].bits.state = COR_READY;
        }
    }
    param.tick = tick;
}

void CorDispatch(void)
{
    uint8_t id = param.bits.currid;
    uint8_t nextid = id;
    uint8_t number=0;
    CorProcessTime();
    while (number++ < param.bits.cap)
    {
        nextid = CorGetNextId(nextid);
        if(nextid == 0)
        {
            continue;
        }

        if (param.coroutine[nextid].bits.state == COR_READY)
        {
            param.bits.currid = nextid;
            return;
        }
    }
    param.bits.currid = 0;
}

static void CorExe(void)
{

    uint8_t id = param.bits.currid;
    param.coroutine[id].bits.state = COR_RUNNING;
    param.coroutine[id].func(param.coroutine[id].arg);
    if (param.coroutine[id].bits.state == COR_RUNNING)
    {
        param.coroutine[id].bits.state = COR_READY;
    }
}

__attribute__((weak)) void CorIdleFunc(void *arg)
{
    // Idle task
}
