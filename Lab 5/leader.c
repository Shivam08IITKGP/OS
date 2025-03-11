#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define DEFAULT_COUNT 10
#define MAX_COUNT 100
#define TABLE_SIZE 1000
#define SHM_KEY_PATH "/"
#define SHM_KEY_ID 'M'

// Structure for tracking sums
typedef struct
{
    int value;
    bool occupied;
} SumTracker;

// Global variables
int shm_id = -1;
int *shared_mem = NULL;
SumTracker *sum_tracker = NULL;
int total_sum;

// Function declarations
void cleanup_resources(key_t key);
int compute_hash(int value);
void initialize_shared_memory(int n);
void initialize_hash_table();
bool check_duplicate_sum(int total);
void wait_for_followers(int n);

int main(int argc, char *argv[])
{
    srand(time(NULL) ^ getpid());

    int n = DEFAULT_COUNT;
    if (argc > 1)
    {
        n = atoi(argv[1]);
        if (n <= 0 || n > MAX_COUNT)
        {
            fprintf(stderr, "Error: Count must be between 1 and %d\n", MAX_COUNT);
            exit(1);
        }
    }

    key_t key = ftok(SHM_KEY_PATH, SHM_KEY_ID);
    if (key == -1)
    {
        perror("ftok error");
        exit(1);
    }

    cleanup_resources(key);
    initialize_shared_memory(n);
    initialize_hash_table();

    printf("Wait for a moment\n");
    wait_for_followers(n);

    bool is_duplicate_sum = false;
    do
    {
        while (shared_mem[2] != 0)
        {
            usleep(1000);
        }

        shared_mem[3] = (rand() % 97) + 2;
        shared_mem[2] = 1;

        while (shared_mem[2] != 0)
        {
            usleep(1000);
        }

        total_sum = 0;
        for (int i = 3; i < 4 + n; i++)
        {
            total_sum += shared_mem[i];
        }

        printf("%d", shared_mem[3]);
        for (int i = 4; i < 4 + n; i++)
        {
            printf(" + %d", shared_mem[i]);
        }
        printf(" = %d\n", total_sum);

        is_duplicate_sum = check_duplicate_sum(total_sum);
        if (is_duplicate_sum)
        {
            shared_mem[2] = -1;
        }
    } while (!is_duplicate_sum);

    while (shared_mem[2] != 0)
    {
        usleep(500);
    }

    return 0;
}

void cleanup_resources(key_t key)
{
    if (shared_mem)
    {
        shmdt(shared_mem);
    }
    if (shm_id != -1)
    {
        shmctl(shm_id, IPC_RMID, NULL);
    }
    if (sum_tracker)
    {
        free(sum_tracker);
    }
    shmctl(shmget(key, 0, 0), IPC_RMID, NULL);
}

void initialize_shared_memory(int n)
{
    int shm_size = (4 + n) * sizeof(int);
    shm_id = shmget(ftok(SHM_KEY_PATH, SHM_KEY_ID), shm_size, IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1)
    {
        perror("shmget error (shared memory might exist)");
        exit(1);
    }

    shared_mem = (int *)shmat(shm_id, NULL, 0);
    if (shared_mem == (int *)-1)
    {
        perror("shmat error");
        shared_mem = NULL;
        exit(1);
    }

    shared_mem[0] = n;
    shared_mem[1] = 0;
    shared_mem[2] = 0;
}

void initialize_hash_table()
{
    sum_tracker = calloc(TABLE_SIZE, sizeof(SumTracker));
    if (!sum_tracker)
    {
        perror("Memory allocation error");
        exit(1);
    }
}

bool check_duplicate_sum(int total)
{
    int index = compute_hash(total);
    while (sum_tracker[index].occupied)
    {
        if (sum_tracker[index].value == total)
        {
            return true;
        }
        index = (index + 1) % TABLE_SIZE;
    }
    sum_tracker[index].value = total;
    sum_tracker[index].occupied = true;
    return false;
}

void wait_for_followers(int n)
{
    while (shared_mem[1] < n)
    {
        usleep(100000);
    }
}

int compute_hash(int value)
{
    return ((value * 29) + 11) % TABLE_SIZE;
}