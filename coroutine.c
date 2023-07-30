/**
 * @file coroutine.c
 * @brief Coroutine implementation.
 * @note This coroutine is used in single chip microcomputer.
 *       Coroutine concurrency is implemented here, but not true concurrency, just switching tasks in a single thread.
 *       All tasks appear to be executing simultaneously, but they are actually switching execution in a single thread.
 *       Then using this library is like using a bare shoe, only it makes complex code look simpler.
 *       Of course, you can also think of it as an upgraded state machine. In contrast to traditional state machines, there is no need to maintain a large number of task flags.
 *       Bare metal does not have resource access conflicts, so it does not need any synchronization mechanism (semaphore).
 *       Because of the high cost of RTOS, simple tasks or low-end chips can not use RTOS, especially complex logic/high-end chips directly on linux, there is no need for RTOS.
 *       Code repository:https://github.com/CylAlon/coroutine
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

#define COROUTINE_MAX_SIZE (32)
cor_handle_t CorIdleHandle = 0;
struct
{
    Coroutine_t *coroutine;
    uint32_t (*get_tick_1ms)(void);
    uint32_t tick;
    struct
    {
        uint8_t cap : 5;
        uint8_t currid : 5;
        uint8_t alreadyInit : 1;
    } bits;
} param;
/**
 * @brief Switch to the next node.
 * @param currid Current id.
 * @return Next id.
 */
static uint8_t cor_get_next_id(uint8_t currid)
{
    return (currid + 1) % param.bits.cap;
}

/**
 * @brief Initialize coroutine
 * @param cap Number of tasks
 * @param get_tick_1ms Get tick
 * @return true if success
 */
bool cor_init(uint8_t cap, uint32_t (*get_tick_1ms)(void))
{
    assert_param(get_tick_1ms != NULL);
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
    param.get_tick_1ms = get_tick_1ms;
    param.tick = 0;
    param.bits.currid = 0;
    param.bits.alreadyInit = 1;
    cor_create_task(&CorIdleHandle, cor_idle_callback, NULL);
    return true;
}
/**
 * @brief Deinitialize coroutine
 */
void cor_deinit(void)
{
    if (param.coroutine != NULL)
    {
        free(param.coroutine);
        param.coroutine = NULL;
    }
    // Reset all other states
    param.bits.alreadyInit = 0;
    // Other states reset as needed
}

/**
 * @brief Create a task
 * @param handle Task handle
 * @param callback Task callback
 * @param arg Task argument
 * @return true if success
 */
bool cor_create_task(cor_handle_t *handle, void (*callback)(void *arg), void *arg)
{
    static uint8_t id = 0;
    assert_param(handle != NULL);
    assert_param(callback != NULL);
    if (param.bits.alreadyInit == 0 || id >= param.bits.cap)
    {
        return false;
    }
    param.coroutine[id].callback = callback;
    param.coroutine[id].arg = arg;
    param.coroutine[id].bits.state = COR_CREATED;
    param.coroutine[id].bits.swstate = SW_NORMAL;
    param.coroutine[id].timeout = 0;
    param.coroutine[id].label = NULL;

    *handle = id;
    id += 1;
    return true;
}
/**
 * @brief Set switch state
 * @param state Switch state
 * @note If the task is normal, the execution of a process is SW_NORMAL. Operations interrupted in the process, such as hibernation and suspension, are SW_ABORT
 */
void cor_set_sw_state(switch_state_t state)
{
    uint8_t id = param.bits.currid;
    param.coroutine[id].bits.swstate = state;
}
void yield(void *label, cor_state_t state, uint32_t timeout)
{
    uint8_t id = param.bits.currid;
    param.coroutine[id].bits.state = state;
    param.coroutine[id].timeout = timeout;
    param.coroutine[id].label = label;
    param.coroutine[id].bits.swstate = SW_ABORT;
}
void suspend(cor_handle_t *handle)
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
void resume(cor_handle_t *handle)
{
    uint8_t id = *handle;
    if (handle == NULL)
    {
        id = param.bits.currid;
    }
    param.coroutine[id].bits.state = COR_READY;
    param.coroutine[id].timeout = 0;
}

void mutex_lock(muxtex_handle_t *handle)
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
void mutex_unlock(muxtex_handle_t *handle)
{
    assert_param(handle != NULL);
    *handle &= ~(1 << param.bits.currid);
}

void *cor_begin(void *label)
{
    uint8_t id = param.bits.currid;
    if (param.coroutine[id].bits.swstate == SW_NORMAL)
    {
        param.coroutine[id].bits.swstate = SW_ABORT;
        param.coroutine[id].label = label;
    }
    return param.coroutine[id].label;
}

static void cor_process_time(void)
{
    uint32_t tick = param.get_tick_1ms();
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

static void cor_dispatch(void)
{
    uint8_t id = param.bits.currid;
    uint8_t nextid = id;
    uint8_t number = 0;
    cor_process_time();
    while (number++ < param.bits.cap)
    {
        nextid = cor_get_next_id(nextid);
        if (nextid == 0)
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

static void cor_exec(void)
{

    uint8_t id = param.bits.currid;
    param.coroutine[id].bits.state = COR_RUNNING;
    param.coroutine[id].callback(param.coroutine[id].arg);
    if (param.coroutine[id].bits.state == COR_RUNNING)
    {
        param.coroutine[id].bits.state = COR_READY;
    }
}
bool cor_run(void)
{
    if (param.bits.alreadyInit == 0)
    {
        return false;
    }
    param.bits.currid = 0;
    for (int i = 0; i < param.bits.cap; i++)
    {
        if (param.coroutine[i].bits.state != COR_NONE)
            param.coroutine[i].bits.state = COR_READY;
    }
    param.tick = param.get_tick_1ms();
    for (;;)
    {
        cor_dispatch();
        cor_exec();
    }
    return true;
}

__attribute__((weak)) void cor_idle_callback(void *arg)
{
    // Idle task
}
