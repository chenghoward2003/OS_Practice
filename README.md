# Operating Systems Practice Project

This repository contains a collection of practical exercises demonstrating key operating system concepts including process management, thread synchronization, and inter-process communication.

## Project Components

### 1. Matrix Multiplication using Processes (`Matrix_Process.c`)
- Implements matrix multiplication using multiple processes
- Demonstrates fork(), wait(), and process synchronization
- Features:
  - Dynamic matrix size input
  - Parallel computation using child processes
  - Error handling and input validation

### 2. Matrix Multiplication using Threads (`Matrix_Thread.c`)
- Alternative implementation of matrix multiplication using threads
- Demonstrates thread-based parallel computation

### 3. Producer-Consumer Problem (`Produce_Consume.c`)
- Implements the classic producer-consumer problem using threads
- Uses semaphores for synchronization
- Features:
  - Bounded buffer implementation
  - Performance metrics (waiting time, burst time, turnaround time)
  - File-based output logging


## Requirements

- Linux/Unix operating system or Windows Subsystem for Linux (WSL)
- GCC compiler
- POSIX threads library
- Standard C libraries

## Compilation

To compile the programs, use the following commands:

```bash
# Compile Matrix Process version
gcc Matrix_Process.c -o matrix_process

# Compile Producer-Consumer
gcc Produce_Consume.c -o produce_consume -pthread

# Compile Matrix Thread version
gcc Matrix_Thread.c -o matrix_thread -pthread
```

## Objectives

This project demonstrates:
1. Process creation and management
2. Thread creation and synchronization
3. Inter-process communication
4. Shared memory management
5. Producer-Consumer problem
6. Matrix multiplication parallelization
