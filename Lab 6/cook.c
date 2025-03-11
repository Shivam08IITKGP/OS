#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define SHM_SIZE 2000
#define COOK_QUEUE_START 1100
#define COOK_QUEUE_SIZE 900
#define TIME_INDEX 0
#define EMPTY_TABLES_INDEX 1
#define NEXT_WAITER_INDEX 2
#define ORDERS_PENDING_INDEX 3
#define NUM_WAITERS 5
#define SIM_MINUTE 100000 // 100 ms in microseconds

// Semaphore indices
#define MUTEX_SEM 0
#define COOK_SEM 1
#define WAITER_SEM_START 2
#define CUSTOMER_SEM_START 7

// Function prototypes
void cmain(int cook_id);
void lock(int semid);
void unlock(int semid);

// Global variables
int shmid, semid, *shared_memory;

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Print time function (converts simulation time to clock time)
void display_time(int sim_time)
{
    int hour = 11 + sim_time / 60;
    int minute = sim_time % 60;
    char am_pm = (hour >= 12) ? 'P' : 'A';
    if (hour > 12)
        hour -= 12;
    printf("[%02d:%02d %cM] ", hour, minute, am_pm);
}

// Lock function
void lock(int semid)
{
    struct sembuf sbuf;
    sbuf.sem_num = MUTEX_SEM;
    sbuf.sem_op = -1; // Lock
    sbuf.sem_flg = 0;
    if (semop(semid, &sbuf, 1) == -1)
    {
        perror("semop - lock mutex");
        exit(1);
    }
}

// Unlock function
void unlock(int semid)
{
    struct sembuf sbuf;
    sbuf.sem_num = MUTEX_SEM;
    sbuf.sem_op = 1; // Unlock
    sbuf.sem_flg = 0;
    if (semop(semid, &sbuf, 1) == -1)
    {
        perror("semop - unlock mutex");
        exit(1);
    }
}

int main()
{
    key_t key = ftok("cook.c", 65); // Use consistent key across all programs
    pid_t cook_c_pid, cook_d_pid;

    // Create shared memory
    shmid = shmget(key, SHM_SIZE * sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        exit(1);
    }

    // Create semaphores
    semid = semget(key, 7 + 200, IPC_CREAT | 0666); // Mutex, Cook, 5 Waiters and 200 Customers
    if (semid < 0)
    {
        perror("semget");
        exit(1);
    }

    // Initialize semaphores
    union semun arg;

    // 1. Mutex
    arg.val = 1;
    if (semctl(semid, MUTEX_SEM, SETVAL, arg) == -1)
    {
        perror("semctl - mutex");
        exit(1);
    }

    // 2. Cook
    arg.val = 0;
    if (semctl(semid, COOK_SEM, SETVAL, arg) == -1)
    {
        perror("semctl - cook");
        exit(1);
    }

    // 3. Waiters
    for (int i = 0; i < NUM_WAITERS; i++)
    {
        arg.val = 0;
        if (semctl(semid, WAITER_SEM_START + i, SETVAL, arg) == -1)
        {
            perror("semctl - waiter");
            exit(1);
        }
    }

    // 4. Customers
    for (int i = 0; i < 200; ++i)
    {
        arg.val = 0;
        if (semctl(semid, CUSTOMER_SEM_START + i, SETVAL, arg) == -1)
        {
            perror("semctl - customers");
            exit(1);
        }
    }

    // Attach shared memory
    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // Initialize shared memory
    shared_memory[TIME_INDEX] = 0;           // Time = 0 (11:00 AM)
    shared_memory[EMPTY_TABLES_INDEX] = 20;  // 20 empty tables
    shared_memory[NEXT_WAITER_INDEX] = 0;    // Next waiter to serve = 0
    shared_memory[ORDERS_PENDING_INDEX] = 0; // No orders pending

    // Initialize cook queue pointers
    shared_memory[COOK_QUEUE_START] = COOK_QUEUE_START + 3;     // Front index
    shared_memory[COOK_QUEUE_START + 1] = COOK_QUEUE_START + 3; // Back index

    // Initialize waiter queues
    for (int i = 0; i < NUM_WAITERS; i++)
    {
        int waiter_base = 100 + (i * 200);
        shared_memory[waiter_base] = -1;                  // FR index (food ready flag) (-1 means no food ready)
        shared_memory[waiter_base + 1] = 0;               // PO index (pending orders)
        shared_memory[waiter_base + 2] = waiter_base + 4; // Front pointer
        shared_memory[waiter_base + 3] = waiter_base + 4; // Back pointer
    }

    // Create cook processes
    cook_c_pid = fork();
    if (cook_c_pid == 0)
    {
        cmain(0); // Cook C (ID 0)
        exit(0);
    }
    else if (cook_c_pid < 0)
    {
        perror("fork");
        exit(1);
    }

    cook_d_pid = fork();
    if (cook_d_pid == 0)
    {
        cmain(1); // Cook D (ID 1)
        exit(0);
    }
    else if (cook_d_pid < 0)
    {
        perror("fork");
        exit(1);
    }

    printf("Cook processes created. Waiting for simulation to complete...\n");

    // Wait for cook processes to finish
    waitpid(cook_c_pid, NULL, 0);
    waitpid(cook_d_pid, NULL, 0);

    printf("Cook processes have ended\n");

    // Detach shared memory but don't remove it yet
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(1);
    }

    return 0;
}

void cmain(int cook_id)
{
    key_t key = ftok("cook.c", 65);

    // Get shared memory ID
    shmid = shmget(key, SHM_SIZE * sizeof(int), 0);
    if (shmid < 0)
    {
        perror("shmget");
        exit(1);
    }

    // Get semaphore ID
    semid = semget(key, 7 + 200, 0);
    if (semid < 0)
    {
        perror("semget");
        exit(1);
    }

    // Attach shared memory
    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }

    struct sembuf sbuf;
    int waiter_id, customer_id, customer_count;
    int cook_queue_front_index, cook_queue_back_index;
    int current_time;

    display_time(0);
    printf("Cook %d started work\n", cook_id);

    // Main cook loop
    while (1)
    {
        // Wait for a cooking request
        sbuf.sem_num = COOK_SEM;
        sbuf.sem_op = -1; // Wait
        sbuf.sem_flg = 0;
        if (semop(semid, &sbuf, 1) == -1)
        {
            perror("semop - wait cook");
            exit(1);
        }

        // Lock mutex before accessing the cook queue
        lock(semid);

        // Check if it's after 3:00 PM and the cook queue is empty
        current_time = shared_memory[TIME_INDEX];
        cook_queue_front_index = shared_memory[COOK_QUEUE_START];
        cook_queue_back_index = shared_memory[COOK_QUEUE_START + 1];

        if ((current_time >= 240 || shared_memory[ORDERS_PENDING_INDEX] == -1) &&
            cook_queue_front_index == cook_queue_back_index)
        {
            unlock(semid);
            display_time(current_time);
            printf("Cook %d: Restaurant is closed, ending shift\n", cook_id);
            break;
        }

        // Read a cooking request from the queue
        if (cook_queue_front_index != cook_queue_back_index)
        {
            waiter_id = shared_memory[cook_queue_front_index];
            customer_id = shared_memory[cook_queue_front_index + 1];
            customer_count = shared_memory[cook_queue_front_index + 2];

            // Update the front pointer
            shared_memory[COOK_QUEUE_START] = cook_queue_front_index + 3;
            if (shared_memory[COOK_QUEUE_START] >= COOK_QUEUE_START + COOK_QUEUE_SIZE)
                shared_memory[COOK_QUEUE_START] = COOK_QUEUE_START + 3;

            unlock(semid);

            display_time(current_time);
            printf("Cook %d: Preparing food for customer %d (waiter %d, party of %d)\n",
                   cook_id, customer_id, waiter_id, customer_count);

            // Prepare food (simulated), 5 minutes per person
            usleep(customer_count * 5 * SIM_MINUTE);

            // Lock to update food ready status
            lock(semid);
            if (current_time + 5 * customer_count != shared_memory[TIME_INDEX])
            {
                shared_memory[TIME_INDEX] = current_time + 5 * customer_count;
            }
            current_time = shared_memory[TIME_INDEX];

            // Notify the waiter that the food is ready
            int waiter_fr_index = 100 + (waiter_id * 200);
            shared_memory[waiter_fr_index] = customer_id;
            unlock(semid);

            display_time(current_time);
            printf("Cook %d: Food ready for customer %d, notifying waiter %d\n",
                   cook_id, customer_id, waiter_id);

            // Signal the waiter
            sbuf.sem_num = WAITER_SEM_START + waiter_id;
            sbuf.sem_op = 1; // Signal
            sbuf.sem_flg = 0;
            if (semop(semid, &sbuf, 1) == -1)
            {
                perror("semop - signal waiter");
                exit(1);
            }
        }
        else
        {
            // This should not happen - wake up but no order
            unlock(semid);
        }
    }

    display_time(shared_memory[TIME_INDEX]);
    printf("Cook %d ended shift\n", cook_id);

    // Detach shared memory
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(1);
    }
}