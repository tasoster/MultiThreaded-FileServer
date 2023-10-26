#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#define TOTAL_FILES 10   // fileserver has exactly TOTAL_FILES files 
#define TOTAL_LINES 100  // each file has exactly TOTAL_LINES lines
#define BLOCK_SIZE 100   // block size equals to max line size
#define QUEUE_SIZE 30    // queue size 

// STRUCT FOR A REQUEST
struct request {
    int shmTempId;       // Threads will use this id to attach to shm
    int fileNum;         // file number (0-9)
    int start, stop;     // start and stop lines
};
typedef struct request Request;

// STRUCT FOR SHARED MEMORY (file server and customers)
struct shared_memory {
    sem_t mutex;        // mutual exclusion for queue
    sem_t empty;        
    sem_t full;
    int in, out;
    Request queue[QUEUE_SIZE];  // fifo queue to store requests
};
typedef struct shared_memory* SharedMemory;

struct temp_shared_memory {
    char block[BLOCK_SIZE];     // buffer to hold a single line each time
    sem_t dataReady;            // server has written a new block
    sem_t dataEaten;            // customer has read the block
};
typedef struct temp_shared_memory* TempSharedMemory;

// customer function prototype
void customer (SharedMemory, int, int, float, int);

// server function prototype
void* serverThread (void*);