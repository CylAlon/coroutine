/**
 * @file coroutine.c
 * @brief Coroutine implementation.
 * @note This coroutine is used in single chip microcomputer.
 *       Coroutine concurrency is implemented here, but not true concurrency, just switching tasks in a single thread.
 *       All tasks appear to be executing simultaneously, but they are actually switching execution in a single thread.
 *       Then using this library is like using a bare shoe, only it makes complex code look simpler.
 *       Of course, you can also think of it as an upgraded state machine. In contrast to traditional state machines, there is no need to maintain a large number of task flags
 *       Bare metal does not have resource access conflicts, so it does not need any synchronization mechanism (mutex, semaphore).
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

#ifndef COROUTINE_H
#define COROUTINE_H

#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "main.h"
typedef uint8_t cor_handle_t;
typedef uint32_t muxtex_handle_t;
typedef enum
{
    COR_NONE = 0,
    COR_CREATED,
    COR_READY,
    COR_RUNNING,
    COR_BLOCKED,
    COR_WAITING,
    COR_SUSPEND,
    COR_TERMINATED
} cor_state_t;
typedef enum
{
    SW_NORMAL,
    SW_ABORT,
} switch_state_t;

typedef struct
{
    void (*callback)(void *arg);
    void *arg;
    void *label;
    uint32_t timeout;
    struct
    {
        cor_state_t state : 4;
        uint8_t swstate : 1;

    } bits;
} Coroutine_t;

void cor_idle_callback(void *arg);
void *cor_begin(void *label);

/**
 * @brief Pause the execution of the coroutine and yield the CPU.
 * @param label Coroutine execution label.
 */
void yield(void *label, cor_state_t state, uint32_t timeout);
/**
 * @brief Suspend the specified coroutine.
 * @param handle Pointer to the coroutine handle. If NULL, suspends the current coroutine.
 */
void suspend(cor_handle_t *handle);
/**
 * @brief Resume the execution of the specified coroutine.
 * @param handle Pointer to the coroutine handle.
 */

#define JOINT(x, y) x##y
#define CONCAT(x, y) JOINT(x, y)
#define UNIQUE_LABEL CONCAT(label_, __LINE__)
#define UNIQUE_VAR CONCAT(var_, __LINE__)

#define RESUMESTATE() \
    UNIQUE_LABEL:     \
    cor_set_sw_state(SW_NORMAL)

bool cor_run(void);
void resume(cor_handle_t *handle);
void cor_set_sw_state(switch_state_t state);
void mutex_lock(muxtex_handle_t *handle);
void mutex_unlock(muxtex_handle_t *handle);

/*------The following is the exported user api. Please do not call the functions above this location.------*/

/**
 * @brief Initialize coroutine
 * @param cap Number of tasks,Usually Hal GetTick
 * @param get_tick_1ms Get tick
 * @return true if success
 */
bool cor_init(uint8_t cap, uint32_t (*get_tick_1ms)(void));
/**
 * @brief Deinitialize coroutine
 */
void cor_deinit(void);
/**
 * @brief Create a task
 * @param handle Task handle
 * @param callback Task callback
 * @param arg Task argument
 * @return true if success
 */
bool cor_create_task(cor_handle_t *handle, void (*callback)(void *arg), void *arg);

#define COR_BEGIN()             \
    goto *cor_begin(&&label_0); \
    label_0:
#define cor_yield()                          \
    do                                       \
    {                                        \
        yield(&&UNIQUE_LABEL, COR_READY, 0); \
        return;                              \
        RESUMESTATE();                       \
    } while (0)
#define cor_suspend(handle) \
    do                      \
    {                       \
        suspend(handle);    \
        if (handle == NULL) \
        {                   \
            return;         \
            RESUMESTATE();  \
        }                   \
    } while (0)
#define cor_resume(handle) resume(handle)
#define cor_sleep(ms)                                                   \
    do                                                                  \
    {                                                                   \
        yield(&&UNIQUE_LABEL, COR_WAITING, (ms) < 100 ? (ms) : (ms)-1); \
        return;                                                         \
        RESUMESTATE();                                                  \
    } while (0)
#define cor_mutex_lock(handle) \
    do                         \
    {                          \
        mutex_lock(handle);    \
        if (*handle != 0)      \
        {                      \
            return;            \
            RESUMESTATE();     \
        }                      \
    } while (0)
#define cor_mutex_unlock(handle) mutex_unlock(handle)

#define COR_CHILD_BEGIN(label) \
    if (label != NULL)         \
    {                          \
        goto *label;           \
    }

#define cor_child_task(name, callback)           \
    do                                           \
    {                                            \
        static void *name = 0;                   \
        CONCAT(child_label_, name) :             \
        {                                        \
            uint32_t t = callback(name);         \
            if (t != 0)                          \
            {                                    \
                cor_sleep(t);                    \
                goto CONCAT(child_label_, name); \
            }                                    \
        }                                        \
    } while (0)

#define cor_child_sleep(ms, label) \
    do                             \
    {                              \
        label = &&UNIQUE_LABEL;    \
        return (ms);               \
    UNIQUE_LABEL:                  \
        label = NULL;              \
    } while (0)



#endif
