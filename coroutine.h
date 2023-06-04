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


#ifndef COROUTINE_H
#define COROUTINE_H



#include "stdio.h"
#include "stdbool.h"
#include  "stdlib.h"
#include "string.h"
#include "main.h"


typedef int8_t CorHandle_t;
// Idle coroutine handle
extern CorHandle_t CorIdleHandle;
typedef enum {
    COR_NONE = 0,
    COR_CREATED,  
    COR_READY,    
    COR_RUNNING,  
    COR_BLOCKED,  
    COR_WAITING,  
    COR_SUSPEND,   
    COR_TERMINATED  
} CoroutineState;


typedef struct {
    void (*func)(void *);

    void *arg;
    CoroutineState state;
    bool labelFlag;
    void *label;
    uint32_t timeout;
    uint32_t lastTick;
    
} Coroutine_t;

/**
 * @brief Default idle coroutine function. If not overridden externally, it performs default idle operations.
 * @param arg Coroutine function argument.
 */
void CorIdleFunc(void *arg);
/**
 * @brief Initialize the coroutine library.
 * @param cap Coroutine capacity.
 * @param getTick1ms Pointer to a function that retrieves a 1-millisecond timestamp.
 * @return Whether initialization is successful.
 */
bool CorInit(int8_t cap,uint32_t (*getTick1ms)(void));
/**
 * @brief Create a coroutine.
 * @param handle Pointer to the coroutine handle.
 * @param func Coroutine function pointer.
 * @param arg Coroutine function argument.
 * @return Whether creation is successful.
 */
bool CorCreate(CorHandle_t *handle, void (*func)(void *), void *arg);
/**
 * @brief Delete a coroutine.
 * @param handle Pointer to the coroutine handle.
 * @return Whether deletion is successful.
 */
bool CorDelete(CorHandle_t *handle);
/**
 * @brief Pause the execution of the coroutine and yield the CPU.
 * @param label Coroutine execution label.
 */
void Yield(void *label);
/**
 * @brief Suspend the specified coroutine.
 * @param handle Pointer to the coroutine handle. If NULL, suspends the current coroutine.
 */
void Suspend(CorHandle_t *handle);
/**
 * @brief Resume the execution of the specified coroutine.
 * @param handle Pointer to the coroutine handle.
 */
void CorResume(CorHandle_t *handle);
/**
 * @brief Restart the specified coroutine.
 * @param handle Pointer to the coroutine handle.
 */
void CorRestart(CorHandle_t *handle);
/**
 * @brief Start the coroutine scheduler and begin executing coroutines.
 */
void CorStart(void);

/**
 * @brief Set the timeout for a coroutine.
 * @param timeout Timeout duration in milliseconds.
 */
void CorSetTimeout(uint32_t timeout);




void *CorBegin(void *label);

void CorEnd(void *label);

#define JOINT(x, y) x##y
#define CONCAT(x, y) JOINT(x, y)
#define UNIQUE_LABEL CONCAT(label_, __LINE__)

/**
 * @note:
 * 1. Begin() and End() must appear in pairs, or neither should appear. Otherwise, the coroutine will only be executed once.
 * 2. "&&" in &&label is an extension syntax in GCC, representing the address of label.
 * 3. "&&" may be highlighted with a red wavy underline in VSCode, but it doesn't affect compilation.
 * 4. To resolve the red wavy underline in VSCode, you can add "compilerPath": "gcc" in .vscode/c_cpp_properties.json.
 * 5. Alternatively, you can disable the red wavy underline prompt in VSCode, but it's not recommended.
 * 6. No errors will occur in CLion.
 * 7. Goto is used here to implement task switching.
 * 8. Task switching can also be achieved using setjmp and longjmp, but they have lower efficiency than goto.
 * 9. Switch/case can also be used for task switching.
 * 10. However, when I previously used switch/case, formatting the code in CLion caused misalignment due to case indentation, resulting in messy code. Hence, I switched to using goto.
 */

#define Begin() goto *CorBegin(&&label_0);label_0:
#define CorYield() Yield(&&UNIQUE_LABEL);return;UNIQUE_LABEL:
#define End() CorEnd(&&label_0)

/**
 * @brief Pause for a specified duration within the coroutine.
 * @param t Pause duration in milliseconds.
 */
#define CorSleep(t) CorSetTimeout(t);CorYield()



#endif //COROUTINE_H
