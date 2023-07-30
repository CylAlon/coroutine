# Coroutine Library for Microcontrollers

This is a simple, lightweight coroutine library for microcontrollers, implemented in C.

## Features

- Task creation, suspension, and resumption.
- Time handling for task waiting.
- Task states: Created, Ready, Running, Blocked, Waiting, Suspended, Terminated.
- Functions for handling mutual exclusion (mutex).

## API

Please look inside the source code

## Usage

First, call `cor_init` to initialize the coroutine library. Then, create tasks using `cor_ceate_task`. Each task is represented by a function, which can yield the CPU, suspend itself, or sleep for a certain time. Tasks can also use mutexes for mutual exclusion.

Here is a simple example:

```c
cor_handle_t handle,handle2;
void task(void *arg) {
   COR_BEGIN()
  {
    printf("task sleep\r\n");
    cor_sleep(1000);
    printf("task yield\r\n");
    cor_yield();
    printf("task suspend");
    cor_suspend();
  }
}
uint32_t child_task(void *label)
{
  COR_CHILD_BEGIN(label)
    {
        int a = 0;
        cor_child_sleep(12, label);
    }
    return 0;
}

void task2(void *arg) {
   COR_BEGIN()
  {
    printf("task\r\n");
    cor_sleep(1000);
    cor_child_task(child_label, child_task);

  }
}
int main() {
  cor_init(4, HAL_GetTick);

  cor_ceate_task(&handle, task, NULL);
  cor_ceate_task(&handle2, task2, NULL);
  cor_run();
  while(1)
  {
    // can not arrive
  }
}
```
