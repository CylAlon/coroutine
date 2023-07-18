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

#ifndef COROUTINE_H
#define COROUTINE_H

#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "main.h"
typedef uint8_t CorHandle_t;
typedef uint32_t MutexHandle_t;
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
} CorState;
typedef enum{
    SW_NORMAL,
    SW_ABORT,
}SwState;

typedef struct
{
    void (*func)(void *);
    void *arg;
    void *label;
    uint32_t timeout;
    struct
    {
        CorState state : 4;
        uint8_t swstate:1;

    } bits;
} Coroutine_t;

bool CorInit(uint8_t cap, uint32_t (*getTick1ms)(void));
void CorDeinit(void);
bool CorCreateTask(CorHandle_t *handle, void (*func)(void *), void *arg);
void CorDispatch(void);

void CorIdleFunc(void *arg);
void *CorBegin(void *label);

/**
 * @brief Pause the execution of the coroutine and yield the CPU.
 * @param label Coroutine execution label.
 */
void Yield(void *label,CorState state, uint32_t timeout);
/**
 * @brief Suspend the specified coroutine.
 * @param handle Pointer to the coroutine handle. If NULL, suspends the current coroutine.
 */
void Suspend(CorHandle_t *handle);
/**
 * @brief Resume the execution of the specified coroutine.
 * @param handle Pointer to the coroutine handle.
 */

#define JOINT(x, y) x##y
#define CONCAT(x, y) JOINT(x, y)
#define UNIQUE_LABEL CONCAT(label_, __LINE__)

#define RESUMESTATE() UNIQUE_LABEL:CorSetSwState(SW_NORMAL)



bool CorRun(void);
void Resume(CorHandle_t *handle);
void CorSetSwState(SwState state);
void MuxLock(MutexHandle_t *handle);
void MuxUnlock(MutexHandle_t *handle);


#define Begin()  goto *CorBegin(&&label_0);label_0:
#define CorYield() do{Yield(&&UNIQUE_LABEL,COR_READY,0);return;RESUMESTATE();}while(0)
#define CorSuspend(handle) do{Suspend(handle); if(handle==NULL){return; RESUMESTATE();}}while(0)
#define CorResume(handle) Resume(handle)
#define CorSleep(ms) do{Yield(&&UNIQUE_LABEL,COR_WAITING,(ms)<100?(ms):(ms)-1);return;RESUMESTATE();}while(0)
#define CorMutexLock(handle) do{MuxLock(handle); if(*handle!=0){return; RESUMESTATE();}}while(0)
#define CorMutexUnlock(handle) MuxUnlock(handle)


#endif
