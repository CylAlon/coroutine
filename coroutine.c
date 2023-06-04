/**
 * @file coroutine.c
 * @brief Coroutine implementation.
 
                                                                                                    
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

static void corSetLabelFlag(bool labelFlag);

static bool corGetLabelFlag(void);

static void corSetLabel(uint32_t *label);

static uint32_t *corGetLabel(void);


CorHandle_t CorIdleHandle=0;
uint32_t (*getTick)(void)=NULL;
struct
{
    int8_t cap;
    int8_t index;
    Coroutine_t *coroutines;
} corSys;

#define COR_MAX_CAP 32

#define INDEX corSys.index
#define CAP corSys.cap
#define COR(handle) corSys.coroutines[handle]
#define COR_STATE(handle) COR(handle).state
#define COR_LABEL_FLAG(handle) COR(handle).labelFlag
#define COR_LABEL(handle) COR(handle).label
#define COR_FUNC(handle) COR(handle).func
#define COR_ARG(handle) COR(handle).arg
#define COR_TIMEOUT(handle) COR(handle).timeout
#define COR_LAST_TICK(handle) COR(handle).lastTick

#define CHANGE_STATE(handle, status) COR_STATE(handle) = status

#define toReady(handle) CHANGE_STATE(handle, COR_READY)
#define toRunning(handle) CHANGE_STATE(handle, COR_RUNNING)
#define toBlocked(handle) CHANGE_STATE(handle, COR_BLOCKED)
#define toWaiting(handle) CHANGE_STATE(handle, COR_WAITING)
#define toSupend(handle) CHANGE_STATE(handle, COR_SUSPEND)
#define toTerminated(handle) CHANGE_STATE(handle, COR_TERMINATED)
#define toCreated(handle) CHANGE_STATE(handle, COR_CREATED)
#define toNone(handle) CHANGE_STATE(handle, COR_NONE)

#define isCreated(handle) (COR_STATE(handle) == COR_CREATED)
#define isReady(handle) (COR_STATE(handle) == COR_READY)
#define isRunning(handle) (COR_STATE(handle) == COR_RUNNING)
#define isBlocked(handle) (COR_STATE(handle) == COR_BLOCKED)
#define isWaiting(handle) (COR_STATE(handle) == COR_WAITING)
#define isSupend(handle) (COR_STATE(handle) == COR_SUSPEND)
#define isTerminated(handle) (COR_STATE(handle) == COR_TERMINATED)
#define isNone(handle) (COR_STATE(handle) == COR_NONE)
#define isIdle(handle) ((handle) == CorIdleHandle)
#define isNotNull(handle) ((handle)!=NULL&&(*handle)!=-1)


bool CorInit(int8_t cap,uint32_t (*getTick1ms)(void))
{
    if (cap <= 1 || cap > COR_MAX_CAP||getTick1ms==NULL)
    {
        return false;
    }
    corSys.coroutines = (Coroutine_t *)malloc(sizeof(Coroutine_t) * cap);
    if (corSys.coroutines == NULL)
    {
        return false;
    }
    CAP = cap;
    INDEX = -1;
    memset(corSys.coroutines, 0, sizeof(Coroutine_t) * cap);
    getTick = getTick1ms;
    // 初始化idle协程
    COR_FUNC(CorIdleHandle) = CorIdleFunc;
    COR_ARG(CorIdleHandle) = NULL;
    toReady(CorIdleHandle);
    return true;
}

void CorDestroy(void)
{
    free(corSys.coroutines);
    corSys.coroutines = NULL;
    CAP = 0;
    INDEX = -1;
}

bool CorCreate(CorHandle_t *handle, void (*func)(void *), void *arg)
{
    if (handle == NULL||*handle!=-1 || func == NULL)
    {
        return false;
    }
    for (int i = 1; i < corSys.cap; ++i)
    {
        if (isNone(i))
        {
            COR_FUNC(i) = func;
            COR_ARG(i) = arg;
            toReady(i);
            *handle = i;
            return true;
        }
    }
    return false;
}


bool CorDelete(CorHandle_t *handle)
{
    if (isNotNull(handle)|| !isIdle(*handle) || isNone(*handle))
    {
        return false;
    }
    toNone(*handle);
    COR_FUNC(*handle) = NULL;
    COR_ARG(*handle) = NULL;
    COR_LABEL(*handle) = NULL;
    COR_LABEL_FLAG(*handle) = false;
    COR_TIMEOUT(*handle) = 0;
    COR_LAST_TICK(*handle) = 0;
    *handle = -1;
    return true;
}

static int8_t switchNext(void)
{
    int8_t next = INDEX + 1;
    if (next >= CAP)
    {
        next = 0;
    }
    INDEX = next;
    return next;
}

void Yield(void *label)
{
    if (isNone(INDEX))
    {
        return;
    }
    toWaiting(INDEX);
    CorSetTimeout(0);
    corSetLabel(label);
}

void Suspend(CorHandle_t *handle)
{
    if (handle == NULL)
    {
        toSupend(INDEX);
        return;
    }
    if (*handle==-1||isNone(*handle) || isTerminated(*handle) || isCreated(*handle))
    {
        return;
    }
    toSupend(*handle);
}

void CorResume(CorHandle_t *handle)
{
    if (*handle==-1||handle == NULL || isNone(*handle) || isTerminated(*handle))
    {
        return;
    }
    toReady(*handle);
    CorSetTimeout(0);
}

void CorRestart(CorHandle_t *handle)
{
    CorResume(handle);
    corSetLabelFlag(false);
}

static void corSetLabelFlag(bool labelFlag)
{
    COR_LABEL_FLAG(INDEX) = labelFlag;
}

static bool corGetLabelFlag(void)
{
    return COR_LABEL_FLAG(INDEX);
}

static void corSetLabel(uint32_t *label)
{
    COR_LABEL(INDEX) = label;
}

static uint32_t *corGetLabel(void)
{
    return COR_LABEL(INDEX);
}

void CorSetTimeout(uint32_t timeout)
{
    COR_TIMEOUT(INDEX) = timeout;
    COR_LAST_TICK(INDEX) = getTick();
}


void *CorBegin(void *label)
{
    if (!corGetLabelFlag())
    {
        corSetLabel(label);
        corSetLabelFlag(true);
    }
    return corGetLabel();
}

void CorEnd(void *label)
{
    corSetLabel(label);
    corSetLabelFlag(false);
}

static void corTimeoutDisp(void)
{
    for (int i = 0; i < corSys.cap; i++)
    {
        if (isNone(i) || isTerminated(i) || isCreated(i) || !isWaiting(i))
        {
            continue;
        }
        if (COR_TIMEOUT(i) > 0)
        {
            uint32_t tick = getTick();
            if (tick - COR_LAST_TICK(i) >= COR_TIMEOUT(i))
            {
                COR_LAST_TICK(i) = tick;
                COR_TIMEOUT(i) = 0;
                toReady(i);
            }
        }
        else
        {
            toReady(i);
        }
    }
}
static int8_t corGetNextTaskId(void)
{
    static uint32_t tick = 0;
    uint32_t mTick = getTick();
    int8_t index = INDEX;
    if (index!=-1&&isReady(index) && mTick != tick)
    {
        tick = mTick;
        toRunning(index);
        return index;
    }
    corTimeoutDisp();
    index = switchNext();
    // 循环一圈找到raday的任务
    while (!isReady(index))
    {
        index = switchNext();
    }
    tick = mTick;
    toRunning(index);
    return index;
}

void CorStart(void)
{
    for (;;)
    {
        int8_t index = corGetNextTaskId();
        COR_FUNC(index)
        (COR_ARG(index));
        if (isRunning(index))
        {
            toReady(index);
        }
    }
}

__attribute__((weak)) void CorIdleFunc(void *arg)
{
    Begin()
    {
        printf("Function [>void CorIdleFunc(void *arg)<] is a default idle coroutine, please override this method externally.\r\n");
        Suspend(NULL);
    }
    End();
}
