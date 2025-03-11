#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define SHM_SIZE 2000
#define TIME_INDEX 0
#define EMPTY_TABLES_INDEX 1
#define NUM_WAITERS 5
#define WAITER_QUEUE_SIZE 200
#define SIM_MINUTE 100000 // 100 ms in microseconds

// Semaphore indices
#define MUTEX_SEM 0
#define COOK_SEM 1
#define WAITER_SEM_START 2
#define CUSTOMER_SEM_START 7

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

void cmain(int customer_id, int arrival_time, int customer_count);

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
    key_t key = ftok("cook.c", 65); // Same key as cook.c and waiter.c
    int shmid, semid;
    FILE *customer_file;
    int customer_id, arrival_time, customer_count;
    pid_t customer_pid;
    int status;
    int *shared_memory;

    // Get shared memory ID
    shmid = shmget(key, SHM_SIZE * sizeof(int), 0);
    if (shmid < 0)
    {
        perror("shmget");
        exit(1);
    }

    // Get semaphore ID
    semid = semget(key, 7 + 200, 0); // Mutex, Cook, 5 Waiters and 200 Customers
    if (semid < 0)
    {
        perror("semget");
        exit(1);
    }

    // Attach shared memory to monitor time
    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // Open customer file
    customer_file = fopen("customers.txt", "r");
    if (customer_file == NULL)
    {
        perror("fopen");
        exit(1);
    }

    int customer_count_loaded = 0;

    // Read customer data and create customer processes
    while (fscanf(customer_file, "%d %d %d", &customer_id, &arrival_time, &customer_count) == 3)
    {
        if (customer_id == -1)
        {
            break; // End of file
        }

        customer_count_loaded++;

        customer_pid = fork();
        if (customer_pid == 0)
        {
            cmain(customer_id, arrival_time, customer_count);
            exit(0);
        }
        else if (customer_pid < 0)
        {
            perror("fork");
            exit(1);
        }
    }

    fclose(customer_file);

    // Simulate time passing
    int current_time = 0;
    while (current_time <= 240) // 3:30 PM (allow some extra time for cleanup)
    {
        lock(semid);
        shared_memory[TIME_INDEX] = current_time;
        unlock(semid);

        if (current_time == 240)
        {
            display_time(current_time);
            printf("Restaurant closing time: 3:00 PM\n");
        }

        usleep(SIM_MINUTE);
        current_time++;
    }

    // Wait for all customer processes to finish
    while (wait(&status) > 0)
        ;

    printf("Restaurant simulation ended.\n");

    // Clean up resources - remove shared memory and semaphores
    // Detach shared memory
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(1);
    }

    // Remove shared memory and semaphores after a delay to ensure all processes are done
    sleep(1);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}

void cmain(int customer_id, int arrival_time, int customer_count)
{
    key_t key = ftok("cook.c", 65); // Same key
    int shmid, semid;
    int *shared_memory;
    struct sembuf sbuf;
    int current_time;

    // Get shared memory ID
    shmid = shmget(key, SHM_SIZE * sizeof(int), 0);
    if (shmid < 0)
    {
        perror("shmget");
        exit(1);
    }

    // Get semaphore ID
    semid = semget(key, 7 + 200, 0); // Mutex, Cook, 5 Waiters and 200 Customers
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

    // Wait until arrival time
    while (1)
    {
        lock(semid);
        current_time = shared_memory[TIME_INDEX];
        unlock(semid);

        if (current_time >= arrival_time)
        {
            break;
        }

        usleep(SIM_MINUTE / 10); // Check more frequently than actual time
    }

    display_time(current_time);
    printf("Customer %d arrived with party of %d\n", customer_id, customer_count);

    // Lock mutex to check restaurant status
    lock(semid);

    // Check if it's after 3:00 PM
    if (shared_memory[TIME_INDEX] >= 240)
    {
        display_time(shared_memory[TIME_INDEX]);
        printf("Customer %d: Arrived after 3:00 PM, leaving.\n", customer_id);
        unlock(semid);
        if (shmdt(shared_memory) == -1)
        {
            perror("shmdt");
            exit(1);
        }
        return;
    }

    // Check if there are enough empty tables
    if (shared_memory[EMPTY_TABLES_INDEX] <= 0)
    {
        display_time(shared_memory[TIME_INDEX]);
        printf("Customer %d: Not enough tables for party of %d (available: %d), leaving.\n",
               customer_id, customer_count, shared_memory[EMPTY_TABLES_INDEX]);
        unlock(semid);
        if (shmdt(shared_memory) == -1)
        {
            perror("shmdt");
            exit(1);
        }
        return;
    }

    // Use empty tables based on party size
    shared_memory[EMPTY_TABLES_INDEX]--;

    // Find the waiter to serve
    int next_waiter = shared_memory[2];
    int waiter_base = 100 + (next_waiter * 200);
    int waiter_back_ptr_idx = waiter_base + 3; // Index storing back pointer
    int waiter_back_ptr = shared_memory[waiter_back_ptr_idx];
    int waiter_po_index = waiter_base + 1; // Pending orders index

    display_time(shared_memory[TIME_INDEX]);
    printf("Customer %d seated, assigned waiter %d\n", customer_id, next_waiter);

    // Add customer to waiter's queue
    shared_memory[waiter_back_ptr] = customer_id;
    shared_memory[waiter_back_ptr + 1] = customer_count;

    // Update waiter's back pointer
    shared_memory[waiter_back_ptr_idx] = waiter_back_ptr + 2;
    // Check for wraparound (cyclic queue)
    if (shared_memory[waiter_back_ptr_idx] >= waiter_base + 196)
    {
        shared_memory[waiter_back_ptr_idx] = waiter_base + 4;
    }

    // Increment pending orders
    shared_memory[waiter_po_index]++;

    // Update next waiter (round-robin)
    shared_memory[2] = (next_waiter + 1) % NUM_WAITERS;

    // Notify waiter about new order
    unlock(semid);

    sbuf.sem_num = WAITER_SEM_START + next_waiter;
    sbuf.sem_op = 1; // Signal
    sbuf.sem_flg = 0;
    if (semop(semid, &sbuf, 1) == -1)
    {
        perror("semop - signal waiter");
        exit(1);
    }

    // Wait for food to be served
    sbuf.sem_num = CUSTOMER_SEM_START + customer_id;
    sbuf.sem_op = -1; // Wait
    sbuf.sem_flg = 0;
    if (semop(semid, &sbuf, 1) == -1)
    {
        perror("semop - wait for food");
        exit(1);
    }

    current_time = shared_memory[TIME_INDEX];
    display_time(current_time);
    printf("Customer %d received food and is eating\n", customer_id);

    // Eat food (30 minutes)
    usleep(30 * SIM_MINUTE);

    lock(semid);
    if (current_time + 30 != shared_memory[TIME_INDEX])
    {
        shared_memory[TIME_INDEX] = current_time + 30;
    }
    current_time = shared_memory[TIME_INDEX];
    display_time(current_time);
    printf("Customer %d finished eating and leaving\n", customer_id);

    // Free up tables
    shared_memory[EMPTY_TABLES_INDEX]++;
    unlock(semid);

    // Detach shared memory
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(1);
    }
}