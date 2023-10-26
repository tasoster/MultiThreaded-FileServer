#include "../include/mutual.h"

void* serverThread(void* arg) {

    // this is the request that the thread will serve
    Request request = *(Request*)arg;

    // Retrieve id of the shared memory from the request, to attach to client's shared memory
    TempSharedMemory shmTemp = (TempSharedMemory)shmat(request.shmTempId, NULL, 0);
    if ((void*)shmTemp == (void*)-1) {
        perror("shmat"); pthread_exit(NULL); }

    // Client wants to read lines from start to stop
    int num_blocks = request.stop-request.start;
    
    // open the requested file. All found in the files folder
    char filename[20];
    sprintf(filename, "files/file%d.txt", request.fileNum);
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen"); pthread_exit(NULL); }

    // find the starting line
    int current_line = 0;
    char data_block[BLOCK_SIZE];
    while (current_line < request.start-1) {
        if (fgets(data_block, BLOCK_SIZE, file) == NULL) {
            perror("fgets"); pthread_exit(NULL); }
        current_line++;
    }

    // start sending. one line at a time
    for (int block = 0; block <= num_blocks; block++) {
        
        // wait for client to consume the previous block
        if(block>0)
            sem_wait(&shmTemp->dataEaten);
        
        // get line from file
        char* line = fgets(data_block, BLOCK_SIZE, file);
        if (line == NULL) {
            perror("fgets"); pthread_exit(NULL);}

        // pass line to shared memory
        memcpy(shmTemp->block, data_block, BLOCK_SIZE);

        // signal client that data is ready
        sem_post(&shmTemp->dataReady);

    }
    
    // normal exit
    fclose(file);
    shmdt(shmTemp);
    pthread_exit(NULL);

}