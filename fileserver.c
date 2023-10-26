#include "./include/mutual.h"

int main(int argc, char** argv) {
    
    if (argc != 5) {
        printf("Usage: %s <N> <K> <L> <l>\n", argv[0]); exit(1); }

    int N = atoi(argv[1]); // N customers customers
    int K = atoi(argv[2]); // K available files in system
    int L = atoi(argv[3]); // L requests per customers
    float l = atof(argv[4]); // l: Exponential time between SharedMemory
    if(K > TOTAL_FILES) {
        printf("K must be less than or equal to %d\n", TOTAL_FILES); exit(1); }

    // delete all existing log files
    system("rm -rf log/*");

    // Create shared memory segment
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget"); exit(1);}

    // Attach shared memory segment
    SharedMemory shm = (SharedMemory)shmat(shmid, NULL, 0);
    if (shm ==(SharedMemory)(-1)) {
        perror("shmat"); exit(1);}

    // initialize shared memory's variables and semaphores
    shm->in = shm->out = 0;
    if (sem_init(&shm->mutex, 1, 1) == -1) {
        perror("sem_init"); exit(1);}
    if (sem_init(&shm->empty, 1, QUEUE_SIZE) == -1) {
        perror("sem_init"); exit(1);}
    if (sem_init(&shm->full, 1, 0) == -1) {
        perror("sem_init"); exit(1);}

    // N customers -> fork N child processes
    pid_t* customers = (pid_t *)malloc(N * sizeof(pid_t));
    if (customers == NULL) {
        perror("malloc"); return 1; }
    
    for (int i = 0; i < N; i++) {    
        pid_t pid = fork();
        if (pid == 0) {        
            customer(shm, K, L, l, i);  // child processes have different code
            exit(0);
        } 
        else if (pid > 0)  
            customers[i] = pid;         // parent process stores child's pid
        else {
            perror("fork"); exit(1); }
    }

    int count=0;    // requests served up till now
    pthread_t* thread_ids = (pthread_t *)malloc(N*L * sizeof(pthread_t));
    Request* requests = malloc(N*L*sizeof(Request));
    
    // This loop retrieves requests from shared memory queue and creates a new thread to handle each one
    while(1) {

        // Wait if queue is empty
        sem_wait(&shm->full);

        // need for exclusive access to queue
        sem_wait(&shm->mutex);
        // Get request from queue
        requests[count] = shm->queue[shm->out];

        // create a new thread to handle the request.
        pthread_t tid;
        pthread_create(&tid, NULL, serverThread, &requests[count]);
        thread_ids[count] = tid; 

        // remove request from queue
        shm->out = (shm->out + 1) % QUEUE_SIZE;
        count++;
        
        // check if all requests have been served
        if(count == N*L) {
            sem_post(&shm->mutex);
            sem_post(&shm->empty);
            break;  // exit loop
        }
        sem_post(&shm->mutex);
        sem_post(&shm->empty);

    }

    // Wait for all threads to finish
    for (int i = 0; i < count; i++) 
        pthread_join(thread_ids[i], NULL);

    // wait for all child customers to finish
    for (int i = 0; i < N; i++) 
        waitpid(customers[i], NULL, 0);

    // free memory    
    free(thread_ids);
    free(requests);
    free(customers);

    // destroy semaphores, detach shared memory segment and exit
    sem_destroy(&shm->mutex);
    sem_destroy(&shm->empty);
    sem_destroy(&shm->full);
    shmdt(shm);
    return 0;
}
