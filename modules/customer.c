#include "../include/mutual.h"

void customer (SharedMemory shm, int K, int L, float l, int id) {

    pid_t pid = getpid();
	srand(pid);     // seed with unique pid

    // create a log file named processI inside log folder.
    char filename[20];
    sprintf(filename, "log/process%d.txt", id);
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("fopen"); exit(1); }
    
    // write initial info to log
    fprintf(file, "This is the log file of process number %d, with Pid: %d\n", id, pid);
    fprintf(file, "There will be %d requests in total.\n\n", L);

    // array to keep track of number of different files requested
    int files_requested[TOTAL_FILES];
    for(int i = 0; i < TOTAL_FILES; i++) 
        files_requested[i] = 0;

    // create an array to store the number of lines returned totally for all requests 
    int lines_returned = 0;
    struct timeval requestTime; // request placed in queue at time:
    struct timeval finishTime;  // request served at time:
    struct timeval delayTime;   // delay time for each request
    double average_time = 0;      // average time between requests
    
    // loop to create L requests and place them in the queue
    for(int i = 0; i < L; i++ ) { 

        // print request counter to log
        fprintf(file, "# %d ----------------------------------", i+1);
        fprintf(file, "---------------------------------------\n\n");

        // form a request                    
        Request request;
        request.fileNum = rand() % K;               // random file number (0-9)
        files_requested[request.fileNum]++;         // increment the number of times this file has been requested
        request.start = rand() % TOTAL_LINES;       // random start line
        request.stop = rand() % TOTAL_LINES;        // random stop line
        if(request.start >= request.stop) {         // if start > stop, swap them
            int temp = request.start;
            request.start = request.stop;
            request.stop = temp;
        }

        // create temporary shared memory to hold the buffer
        int shmTempId = shmget(IPC_PRIVATE, sizeof(TempSharedMemory), IPC_CREAT | 0666);
        if (shmTempId == -1) {
            perror("shmget"); exit(1);}

        // Attach shared memory segment
        TempSharedMemory shmTemp = (TempSharedMemory)shmat(shmTempId, NULL, 0);
        if (shmTemp ==(TempSharedMemory)(-1)) {
            perror("shmat"); exit(1);}
        
        // add shmTempId to request so the serving thread can attach to the new temp shared memory
        request.shmTempId = shmTempId;
        if(sem_init(&shmTemp->dataReady, 1, 0) == -1) {
            perror("sem_init"); exit(1);}
        if(sem_init(&shmTemp->dataEaten, 1, 0) == -1) {
            perror("sem_init"); exit(1);}

        // Wait for empty slot in shared memory buffer
        sem_wait(&shm->empty);
        sem_wait(&shm->mutex);      //exclusive access to queue
        
        // Put request in shared memory
        shm->queue[shm->in] = request;
        shm->in = (shm->in + 1) % QUEUE_SIZE;
        gettimeofday(&requestTime, NULL); // set the request time.
        sem_post(&shm->full);
        sem_post(&shm->mutex);

        // print request info to log file
        fprintf(file, "<File Number>: %d\n", request.fileNum);
        fprintf(file, "<Start Line>: %d\n", request.start);
        fprintf(file, "<Stop Line>: %d\n", request.stop);
        fprintf(file, "\n<CONTENT>\n");

        // start retrieving data from temp shared memory
        int num_blocks = request.stop-request.start;
        lines_returned += num_blocks;
        for (int block = 0; block <= num_blocks; block++) {
            
            sem_wait(&shmTemp->dataReady);  // Wait for server thread to write new block
            char data_block[BLOCK_SIZE];    // Read data_block from shared memory segment
            memcpy(data_block, shmTemp->block, BLOCK_SIZE); // save data_block to data_block
            fprintf(file, "%s", data_block);    //  print data block to log file
            sem_post(&shmTemp->dataEaten);  // Notify the server process that the block has been read
        
        }
        // request has been served completely
        gettimeofday(&finishTime, NULL);

        // calculate delay time and print it to log file
        fprintf(file, "\n<Delay Time>: %ldms\n\n\n", finishTime.tv_usec-requestTime.tv_usec);

        // destroy semaphores, detach from shared memory segment and delete it
        sem_destroy(&shmTemp->dataReady);
        sem_destroy(&shmTemp->dataEaten);
        shmdt(shmTemp);
        shmctl(shmTempId, IPC_RMID, NULL);

        // simulation of: exponential time between requests
        double u = (double)rand() / RAND_MAX;
        double time_between = -log(1.0 - u) / (double)l;
        time_between *= 1000.0;             // Convert time_between_requests to milliseconds
        average_time += time_between;       // add time_between_requests to average_time
        usleep((unsigned int)time_between); // Sleep for exponential time between requests

    }

    // print final total stats to log file
    fprintf(file, "--------------------------------------------------");
    fprintf(file, "\n\nTOTAL LINES returned for all requests: %d\n\n", lines_returned);
    fprintf(file, "AVERAGE TIME between requests: %dms\n\n", (unsigned int) average_time/L);
    fprintf(file, "TOTAL TIMES that a file was requested by process:\n");
    for(int i = 0; i < TOTAL_FILES; i++) 
        fprintf(file, "File %d was requested %d times.\n", i, files_requested[i]);

}