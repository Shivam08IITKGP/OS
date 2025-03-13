/*
Shivam Choudhury
22CS10072
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
bool exited[MAX_BOATS] = {0};
pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t EOS;
int total_riders;
int BA[MAX_BOATS] = {0};
int BC[MAX_BOATS] = {-1};
int BT[MAX_BOATS] = {0};
pthread_barrier_t BB[MAX_BOATS];

void *boat_thread(void *arg)
{
    int boat_id = *((int *)arg);
    free(arg);
    printf("Boat\t%d\tReady\n", boat_id);

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
        if (total_riders <= 0)
        {
            pthread_mutex_unlock(&bmtx);
            break;
        }
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

        printf("Boat\t%d\tStart ride for visitor %d (ride time = %d min)\n", boat_id, current_rider, ride_time);
        usleep(100000 * ride_time);
        printf("Boat\t%d\tEnd ride for visitor %d\n", boat_id, current_rider);

        pthread_mutex_lock(&bmtx);
        total_riders--;

        int remaining = total_riders;
        if (remaining == 0)
        {
            pthread_mutex_unlock(&bmtx);
            pthread_barrier_wait(&EOS); // Only one boat reaches EOS
            break;
        }
        pthread_mutex_unlock(&bmtx);
    }
    return NULL;
}

void *rider_thread(void *arg)
{
    int rider_id = *((int *)arg);
    free(arg);

    int visit_time = rand() % 91 + 30;
    printf("Visitor\t%d\tStarts sightseeing (%dmin)\n", rider_id, visit_time);
    usleep(visit_time * 100000);
    int rider_time = rand() % 46 + 15;
    printf("Visitor\t%d\tReady to ride a boat (ride time = %d min)\n", rider_id, rider_time);
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
                BT[i] = rider_time;
                BA[i] = 0;
                break;
            }
        }
        pthread_mutex_unlock(&bmtx);

        if (boat_id != -1)
        {
            break;
        }
    }
    printf("Visitor\t%d\tFinds boat %d\n", rider_id, boat_id);
    pthread_barrier_wait(&BB[boat_id]); // Sync with boat

    usleep(rider_time * 100000);
    printf("Visitor\t%d\tLeaving\n", rider_id);
    exited[rider_id] = 1;
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
    pthread_barrier_init(&EOS, NULL, 2);

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
    for (int i = 0; i < m; i++)
    {
        if (!exited[i])
        {
            pthread_join(boats[i], NULL);
        }
    }

    return 0;
}
