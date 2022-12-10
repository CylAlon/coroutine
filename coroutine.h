#ifndef COROUTINE_H__
#define COROUTINE_H__

#include "main.h"
#include "stdbool.h"

#ifndef ALIGN_1
#define ALIGN_1 __attribute__((packed, aligned(1))) // aligned(1)：1字节对齐
#endif
typedef void *CorHandle_t;
struct ALIGN_1 CorStruct;
struct ALIGN_1 CorNode;
typedef struct CorStruct Cor_t;
typedef struct CorNode CorNode_t;


#define Cor_GetTick() HAL_GetTick() // 获取定时器滴答计数
#define Cor_Period(ms) ({ms==1?1:ms-1;}) // 定时器周期



typedef enum {
    COR_RUN = 0, // 运行
    COR_SLEEP, // 休眠
    COR_STOP, // 停止
} CorState_t;

struct CorNode {
    uint8_t Id; // 协程自己的ID
    __IO CorState_t State;// 协程状态
    __IO uint8_t Anchor; // 跳转锚点
    __IO uint32_t Tick; // 记录当前时间
    __IO uint32_t SleepTime; // 休眠时间
    uint8_t *Stack; // 协程堆栈
    uint16_t StackSize; // 堆栈大小
    uint16_t StackCap; // 堆栈容量
    void (*Coroutine)(CorNode_t *self, void *context); //任务
};

struct CorStruct {
    __IO uint8_t Id; // 协程当前处理的ID
    uint8_t Cap; // 协程数量
    __IO uint8_t Size;//协程数量
    void *Context; // 协程上下文,可用协程公用的标志指针
    CorNode_t *Table; // 协程表
};


#define Cor_SetAnchor(anchor) goto__addr = &&COR_ANCHOR_##anchor;  return;   COR_ANCHOR_##anchor:
//这个函数用于协程函数中的挂起协程
#define Cor_Suspend(handle, anchor)do{                      \
    Coroutine.Table[Coroutine.Id].SleepTime = 0;            \
    if (handle == NULL)                                     \
    {                                                       \
        Coroutine.Table[Coroutine.Id].State = COR_STOP;     \
        Cor_SetAnchor(anchor);                              \
    } else                                                  \
    {                                                       \
        ((CorNode_t *) handle)->State = COR_STOP;          \
    }                                                       \
}while(0)


// 恢复/启动协程
#define Cor_Resume(handle)do{                                       \
    if (handle == NULL)                                             \
    {                                                               \
        Coroutine.Table[Coroutine.Id].State = COR_RUN;              \
        Coroutine.Table[Coroutine.Id].SleepTime = 0;                \
    } else                                                          \
    {                                                               \
        ((CorNode_t *) handle)->State = COR_RUN;                   \
        ((CorNode_t *) handle)->SleepTime = 0;                     \
    }                                                               \
}while(0)
#define Cor_Restart(handle) do{                                     \
    if (handle == NULL)                                             \
    {                                                               \
        Coroutine.Table[Coroutine.Id].State = COR_RUN;              \
        Coroutine.Table[Coroutine.Id].SleepTime = 0;                \
        Coroutine.Table[Coroutine.Id].Anchor = 0;                   \
        Cor_Free(Coroutine.Table[Coroutine.Id]);\
    } else                                                          \
    {                                                               \
        ((CorNode_t *) handle)->State = COR_RUN;                   \
        ((CorNode_t *) handle)->SleepTime = 0;                     \
        ((CorNode_t *) handle)->Anchor = 0;                         \
        Cor_Free(handle);                                    \
    }                                                               \
}while(0)

//睡眠协程 该函数必须用在协程函数中的While中
#define Cor_Sleep(time, anchor)do{                                      \
    if(time>0)                                                          \
    {                                                                   \
        Coroutine.Table[Coroutine.Id].Anchor = anchor;                  \
        Coroutine.Table[Coroutine.Id].SleepTime = Cor_Period(time);      \
        Coroutine.Table[Coroutine.Id].Tick = Cor_GetTick();             \
        Coroutine.Table[Coroutine.Id].State = COR_SLEEP;                 \
        Cor_SetAnchor(anchor);                                          \
    }                                                                   \
}while(0)
#define sleep Cor_Sleep


// 该宏必须在协程函数中使用，且协程函数的参数必须取名self
#define While(func) do{                                                 \
    static void* goto__addr =&&COR_ANCHOR_0;                            \
     if(Coroutine.Table[Coroutine.Id].Anchor!=0&&goto__addr!=NULL)      \
     {                                                                  \
         goto  *goto__addr;                                             \
     }                                                                  \
    COR_ANCHOR_0:                                                       \
    func                                                                \
    Coroutine.Table[Coroutine.Id].Anchor = 0;                           \
    goto__addr = &&COR_ANCHOR_0;                                        \
}while(0)

extern Cor_t Coroutine;

void Cor_Init(uint8_t cap, void *context);

bool Cor_Create(CorHandle_t *handle, void (*coroutine)(CorNode_t *self, void *ctx),
                CorState_t flag, uint8_t *stack, uint16_t stackCap);

_Noreturn void Cor_Run(void);

void Cor_Stop(CorHandle_t *handle);

static void Cor_Handle(void);

uint8_t *Cor_Malloc(CorHandle_t *handle, uint16_t size);

void Cor_Free(CorHandle_t *handle);

#endif
