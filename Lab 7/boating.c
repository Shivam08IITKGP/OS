#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define SIM_MINUTE 100000
#define MAX_BOATS 10
#define MAX_VISITORS 100

typedef struct
{
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} semaphore;

void wait_sim(int time)
{
    struct timespec ts = {
        .tv_sec = (time * SIM_MINUTE) / 1000000,
        .tv_nsec = ((time * SIM_MINUTE) % 1000000) * 1000};
    nanosleep(&ts, NULL);
}

void P(semaphore *s)
{
    pthread_mutex_lock(&s->mtx);
    while (s->value <= 0)
    {
        pthread_cond_wait(&s->cv, &s->mtx);
    }
    s->value--;
    pthread_mutex_unlock(&s->mtx);
}

void V(semaphore *s)
{
    pthread_mutex_lock(&s->mtx);
    s->value++;
    pthread_cond_signal(&s->cv);
    pthread_mutex_unlock(&s->mtx);
}

// Global variables
semaphore rider;
pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t EOS;
volatile int total_riders;
int BA[MAX_BOATS] = {0};
int BC[MAX_BOATS] = {-1};
int BT[MAX_BOATS] = {0};
pthread_barrier_t BB[MAX_BOATS];

void *boat_thread(void *arg)
{
    int boat_id = *((int *)arg);
    free(arg);
    printf("Boat %d Ready\n", boat_id);

    while (1)
    {
        pthread_mutex_lock(&bmtx);
        if (total_riders <= 0)
        {
            pthread_mutex_unlock(&bmtx);
            break;
        }

        BA[boat_id] = 1;
        BC[boat_id] = -1;
        pthread_mutex_unlock(&bmtx);

        V(&rider);                          // Signal availability
        pthread_barrier_wait(&BB[boat_id]); // Wait for rider assignment

        pthread_mutex_lock(&bmtx);
        if (BC[boat_id] == -1)
        { // No rider assigned
            BA[boat_id] = 0;
            pthread_mutex_unlock(&bmtx);
            continue;
        }

        int ride_time = BT[boat_id];
        int current_rider = BC[boat_id];
        BA[boat_id] = 0;
        pthread_mutex_unlock(&bmtx);

        printf("Boat %d Start ride for visitor %d (%dmin)\n",
               boat_id, current_rider, ride_time);
        wait_sim(ride_time);
        printf("Boat %d Finish ride for visitor %d\n", boat_id, current_rider);

        pthread_mutex_lock(&bmtx);
        total_riders--;
        int remaining = total_riders;
        pthread_mutex_unlock(&bmtx);

        if (remaining == 0)
        {
            pthread_barrier_wait(&EOS);
            break;
        }
    }
    return NULL;
}

void *rider_thread(void *arg)
{
    int rider_id = *((int *)arg);
    free(arg);

    int visit_time = rand() % 91 + 30;
    printf("Visitor %d Starts sightseeing (%dmin)\n", rider_id, visit_time);
    wait_sim(visit_time);

    int boat_id = -1;
    while (boat_id == -1)
    {
        P(&rider); // Wait for available boat

        pthread_mutex_lock(&bmtx);
        for (int i = 0; i < MAX_BOATS; i++)
        {
            if (BA[i] && BC[i] == -1)
            {
                boat_id = i;
                BC[i] = rider_id;
                BT[i] = rand() % 46 + 15;
                BA[i] = 0;
                break;
            }
        }
        pthread_mutex_unlock(&bmtx);

        if (boat_id != -1)
        {
            pthread_barrier_wait(&BB[boat_id]); // Sync with boat
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <boats> <visitors>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));
    int m = atoi(argv[1]);
    int n = atoi(argv[2]);

    if (m > MAX_BOATS || n > MAX_VISITORS)
    {
        printf("Exceeded limits (Max boats: %d, Max visitors: %d)\n",
               MAX_BOATS, MAX_VISITORS);
        return 1;
    }

    total_riders = n;

    // Initialize synchronization primitives
    rider = (semaphore){0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    pthread_barrier_init(&EOS, NULL, 1);

    for (int i = 0; i < m; i++)
    {
        pthread_barrier_init(&BB[i], NULL, 2);
    }

    pthread_t boats[m], riders[n];

    // Create boats
    for (int i = 0; i < m; i++)
    {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&boats[i], NULL, boat_thread, id);
    }

    // Create visitors
    for (int i = 0; i < n; i++)
    {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&riders[i], NULL, rider_thread, id);
    }

    // Wait for all visitors
    for (int i = 0; i < n; i++)
    {
        pthread_join(riders[i], NULL);
    }

    // Wake all boats
    for (int i = 0; i < m; i++)
    {
        V(&rider);
    }

    // Wait for final completion
    pthread_barrier_wait(&EOS);

    // Cleanup
    // for (int i = 0; i < m; i++)
    // {
    //     pthread_join(boats[i], NULL);
    //     pthread_barrier_destroy(&BB[i]);
    // }
    // pthread_barrier_destroy(&EOS);

    printf("All rides completed successfully!\n");
    return 0;
}
