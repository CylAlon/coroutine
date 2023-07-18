# Coroutine Library for Microcontrollers

This is a simple, lightweight coroutine library for microcontrollers, implemented in C.

## Features

- Task creation, suspension, and resumption.
- Time handling for task waiting.
- Task states: Created, Ready, Running, Blocked, Waiting, Suspended, Terminated.
- Functions for handling mutual exclusion (mutex).

## API

### Initialization

- `bool CorInit(uint8_t cap, uint32_t (*getTick1ms)(void))`: Initialize the coroutine library.

### Task Management

- `bool CorCreateTask(CorHandle_t *handle, void (*func)(void *), void *arg)`: Create a new task.
- `void CorDispatch(void)`: Dispatch tasks.
- `bool CorRun(void)`: Run tasks.

### Task Control

- `void Yield(void *label, CorState state, uint32_t timeout)`: Yield the CPU.
- `void Suspend(CorHandle_t *handle)`: Suspend a task.
- `void Resume(CorHandle_t *handle)`: Resume a task.

### Mutex

- `void MuxLock(MutexHandle_t *handle)`: Lock a mutex.
- `void MuxUnlock(MutexHandle_t *handle)`: Unlock a mutex.

## Macros

- `Begin()`: Begin a coroutine.
- `CorYield()`: Yield the CPU.
- `CorSuspend(handle)`: Suspend a coroutine.
- `CorResume(handle)`: Resume a coroutine.
- `CorSleep(ms)`: Sleep for a certain time.
- `CorMutexLock(handle)`: Lock a mutex.
- `CorMutexUnlock(handle)`: Unlock a mutex.

## Usage

First, call `CorInit` to initialize the coroutine library. Then, create tasks using `CorCreateTask`. Each task is represented by a function, which can yield the CPU, suspend itself, or sleep for a certain time. Tasks can also use mutexes for mutual exclusion.

Here is a simple example:

```c
CorHandle_t handle;
void task(void *arg) {
   Begin()
  {
    printf("task\r\n");
    CorSleep(1000);
  }
}

int main() {
  CorInit(4, HAL_GetTick);

  CorCreateTask(&handle, task, NULL);
  CorRun();
  while(1)
  {
    // can not arrive
  }
}
