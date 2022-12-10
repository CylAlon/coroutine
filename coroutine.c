#include "coroutine.h"
#include "stdlib.h"
/*
 * 轻量级协程库，基于C语言实现
 * 作者：CylAlon
 * 日期：2020-07
 * 更改日期：2020-12-15
 * 版本：1.0.0
 */
/**************************结构体申明**************************/
// 协程结构体,没有使用static修饰，可以在其他文件中使用（这是应为.h文件中一些宏会直接使用到这个变量）
// 不适用static修饰，就不会用函数get/set来访问这个变量，而是直接访问，这样会提高效率（用在单片机上）
Cor_t Coroutine;
/**************************具体实现**************************/

/**
 * @brief  协程初始化
 * @param  cap: 协程的最大数目
 */
void Cor_Init(uint8_t cap, void *context)
{
    Coroutine.Cap = cap;
    Coroutine.Id = 0;
    Coroutine.Size = 0;
    Coroutine.Context = context;
    Coroutine.Table = (CorNode_t *) malloc(sizeof(CorNode_t) * cap); //程序申请后就不会再修改大小，这里直接用malloc申请就好
}


/**
 * @brief  创建协程任务
 * @param  handle: 句柄(指向CorNode_t地址)
 * @param  coroutine: 协程函数
 * @param  flag: 协程初始状态
 * @param  heap: 协程堆栈
 * @param  heapSize: 协程堆栈大小
 * @retval true: 创建成功 false: 创建失败
 */
bool Cor_Create(CorHandle_t *handle, void (*coroutine)(CorNode_t *self, void *ctx),
                CorState_t flag, uint8_t *stack, uint16_t stackCap)
{
    if (Coroutine.Size < Coroutine.Cap && coroutine != NULL)
    {
        Coroutine.Table[Coroutine.Size].Id = Coroutine.Size;
        Coroutine.Table[Coroutine.Size].State = flag;
        Coroutine.Table[Coroutine.Size].SleepTime = 0;
        Coroutine.Table[Coroutine.Size].Tick = 0;
        Coroutine.Table[Coroutine.Size].Anchor = 0;
        Coroutine.Table[Coroutine.Size].Coroutine = coroutine;
        Coroutine.Table[Coroutine.Size].Stack = stack;
        Coroutine.Table[Coroutine.Size].StackCap = stackCap;
        memset(Coroutine.Table[Coroutine.Size].Stack, 0, Coroutine.Table[Coroutine.Size].StackCap);
        Coroutine.Table[Coroutine.Size].StackSize = 0;
        *handle = &Coroutine.Table[Coroutine.Size];
        Coroutine.Size++;
        return true;
    }
    return false;
}

void Cor_Stop(CorHandle_t *handle)
{
    if (handle != NULL)
    {
        ((CorNode_t *) handle)->SleepTime = 0;
        ((CorNode_t *) handle)->State = COR_STOP;
    }
}

_Noreturn void Cor_Run(void)
{
    for (;;)
    {
        Cor_Handle();
    }
}

static void Cor_Handle(void)
{
    for (int i = 0; i < Coroutine.Size; ++i)
    {
        // 任务调度
        Coroutine.Id = i;
        if (Coroutine.Table[i].State == COR_RUN)
        {
            Coroutine.Table[i].Coroutine(&Coroutine.Table[i], Coroutine.Context);
        }
        // 睡眠处理（这里使用轮询就足够了，如果时间要求特别精确可以替换为中断）
        if (Coroutine.Table[i].State == COR_SLEEP && Coroutine.Table[i].SleepTime > 0)
        {
            if (Cor_GetTick() - Coroutine.Table[i].Tick >= Coroutine.Table[i].SleepTime)
            {
                Coroutine.Table[i].SleepTime = 0;
                Coroutine.Table[Coroutine.Id].State = COR_RUN;
            }
        }
    }
}

/**************************下面是静态内存管理**************************/
/**
 * @brief  申请内存
 * @param  handle: 句柄
 * @param  size: 申请的大小
 * @retval 申请的内存地址
 */
uint8_t *Cor_Malloc(CorHandle_t *handle, uint16_t size)
{
    if (handle != NULL || size == 0)
    {
        if (((CorNode_t *) handle)->StackSize + size <= ((CorNode_t *) handle)->StackCap)
        {
            ((CorNode_t *) handle)->StackSize += size;
            return ((CorNode_t *) handle)->Stack + ((CorNode_t *) handle)->StackSize;;
        }
    }
    return NULL;
}

void Cor_Free(CorHandle_t *handle)
{
    if (handle != NULL)
    {
        memset(((CorNode_t *) handle)->Stack, 0, ((CorNode_t *) handle)->StackCap);
        ((CorNode_t *) handle)->StackSize = 0;

    }
}