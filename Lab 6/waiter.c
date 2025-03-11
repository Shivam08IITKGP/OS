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
void wmain(int waiter_id);
void lock(int semid);
void unlock(int semid);

int shmid, semid, *shared_memory;

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
    key_t key = ftok("cook.c", 65); // Same key as cook.c
    pid_t waiter_pids[NUM_WAITERS];

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

    printf("Waiter processes starting...\n");

    // Create waiter processes
    for (int i = 0; i < NUM_WAITERS; i++)
    {
        waiter_pids[i] = fork();
        if (waiter_pids[i] == 0)
        {
            wmain(i); // Waiter i
            exit(0);
        }
        else if (waiter_pids[i] < 0)
        {
            perror("fork");
            exit(1);
        }
    }

    // Wait for waiter processes to finish
    for (int i = 0; i < NUM_WAITERS; i++)
    {
        waitpid(waiter_pids[i], NULL, 0);
    }

    printf("All waiter processes have ended\n");

    // Detach shared memory
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(1);
    }

    return 0;
}

void wmain(int waiter_id)
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
    int customer_id, customer_count;
    int waiter_fr_index, waiter_po_index, waiter_queue_front;
    int current_time;

    // Main waiter loop
    while (1)
    {
        // Wait for a signal (from customer placing an order or cook having food ready)
        sbuf.sem_num = WAITER_SEM_START + waiter_id;
        sbuf.sem_op = -1;
        sbuf.sem_flg = 0;
        if (semop(semid, &sbuf, 1) == -1)
        {
            perror("semop - waiter wait");
            exit(1);
        }

        lock(semid);
        current_time = shared_memory[TIME_INDEX];
        waiter_fr_index = 100 + (waiter_id * 200);
        waiter_po_index = waiter_fr_index + 1;
        int food_ready = shared_memory[waiter_fr_index];
        int pending_orders = shared_memory[waiter_po_index];
        int restaurant_closed = (shared_memory[ORDERS_PENDING_INDEX] == -1);

        // Check if restaurant is closed and no more work to do
        if (restaurant_closed && food_ready == -1 && pending_orders == 0)
        {
            unlock(semid);
            display_time(current_time);
            printf("Waiter %d: Restaurant is closed and no more orders, ending shift\n", waiter_id);
            break;
        }

        // Check if food is ready
        if (food_ready != -1)
        {
            // Food is ready, get customer ID
            customer_id = food_ready;

            // Reset food ready flag
            shared_memory[waiter_fr_index] = -1;
            unlock(semid);

            display_time(current_time);
            printf("Waiter %d serving food to customer %d\n", waiter_id, customer_id);

            // Signal customer that food is ready
            sbuf.sem_num = CUSTOMER_SEM_START + customer_id;
            sbuf.sem_op = 1; // Signal
            sbuf.sem_flg = 0;
            if (semop(semid, &sbuf, 1) == -1)
            {
                perror("semop - signal customer");
                exit(1);
            }
        }
        else if (pending_orders > 0)
        {
            // Get customer order from queue
            waiter_queue_front = 100 + (waiter_id * 200) + 2;
            int front_ptr = shared_memory[waiter_queue_front];
            customer_id = shared_memory[front_ptr];
            customer_count = shared_memory[front_ptr + 1];

            // Update queue front pointer
            shared_memory[waiter_queue_front] = front_ptr + 2;
            // Wraparound if needed
            if (shared_memory[waiter_queue_front] >= waiter_queue_front + 196)
                shared_memory[waiter_queue_front] = waiter_queue_front + 4;

            // Decrement pending orders
            shared_memory[waiter_po_index]--;
            current_time = shared_memory[TIME_INDEX];
            display_time(current_time);
            printf("Waiter %d taking order from customer %d (party of %d)\n",
                   waiter_id, customer_id, customer_count);

            // Add order to cook queue

            unlock(semid);
            usleep(SIM_MINUTE);
            lock(semid);

            if (current_time + 1 != shared_memory[TIME_INDEX])
            {
                shared_memory[TIME_INDEX] = 1 + current_time;
            }
            current_time = shared_memory[TIME_INDEX];
            int cook_queue_back_index = shared_memory[COOK_QUEUE_START + 1];
            shared_memory[cook_queue_back_index] = waiter_id;
            shared_memory[cook_queue_back_index + 1] = customer_id;
            shared_memory[cook_queue_back_index + 2] = customer_count;

            // Update back pointer
            shared_memory[COOK_QUEUE_START + 1] = cook_queue_back_index + 3;
            if (shared_memory[COOK_QUEUE_START + 1] >= COOK_QUEUE_START + COOK_QUEUE_SIZE)
                shared_memory[COOK_QUEUE_START + 1] = COOK_QUEUE_START + 3;

            unlock(semid);

            display_time(current_time);
            printf("Waiter %d sent order to kitchen for customer %d\n", waiter_id, customer_id);

            // Signal cook that there's an order to prepare
            sbuf.sem_num = COOK_SEM;
            sbuf.sem_op = 1; // Signal
            sbuf.sem_flg = 0;
            if (semop(semid, &sbuf, 1) == -1)
            {
                perror("semop - signal cook");
                exit(1);
            }
        }
        else
        {
            // No work to do currently
            unlock(semid);
        }
    }

    display_time(shared_memory[TIME_INDEX]);
    printf("Waiter %d ended shift\n", waiter_id);

    // Detach shared memory
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(1);
    }
}