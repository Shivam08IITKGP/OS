#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define DEFAULT_NF 1
#define KEY_PATH "/"
#define KEY_ID 'M'

void follower_process(int *M, int follower_id);

int main(int argc, char *argv[])
{
    int nf = DEFAULT_NF;
    if (argc > 1)
    {
        nf = atoi(argv[1]);
        if (nf <= 0)
        {
            fprintf(stderr, "Error: number of followers must be positive\n");
            exit(1);
        }
    }

    // Get shared memory
    key_t key = ftok(KEY_PATH, KEY_ID);
    if (key == -1)
    {
        perror("ftok error");
        exit(1);
    }

    int shmid = shmget(key, 0, 0666);
    if (shmid == -1)
    {
        perror("shmget error (is leader running?)");
        exit(1);
    }

    int *M = (int *)shmat(shmid, NULL, 0);
    if (M == (int *)-1)
    {
        perror("shmat error");
        exit(1);
    }

    // Fork nf children
    int i = 0;
    do
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork error");
            shmdt(M);
            exit(1);
        }

        if (pid == 0)
        { // Child process
            int follower_id = 0;
            while (1)
            {
                usleep(50000); // Sleep for 50ms
                int current = M[1];

                // Check if we can take the next spot
                if (current < M[0])
                {
                    follower_id = current + 1;
                    M[1] = follower_id;
                    break;
                }

                // If we can't get a spot, check if we should exit
                if (current >= M[0])
                {
                    printf("follower error: %d followers have already joined\n", M[0]);
                    shmdt(M);
                    exit(1);
                }
            }

            printf("follower %d joins\n", follower_id);

            // Start follower process
            follower_process(M, follower_id);

            shmdt(M);
            exit(0);
        }
        i++;
    } while (i < nf);

    // Parent waits for all children
    i = 0;
    do
    {
        wait(NULL);
    } while (i++ < nf);

    shmdt(M);
    return 0;
}

void follower_process(int *M, int follower_id)
{
    // Seed random number generator with unique value for each follower
    srand(time(NULL) ^ getpid());

    // Main loop
    while (1)
    {
        // Wait for my turn
        while (abs(M[2]) != follower_id)
        {
            usleep(1000); // Sleep for 1ms to reduce CPU usage
        }

        // Check if it's time to terminate
        if (M[2] == -follower_id)
        {
            printf("follower %d leaves\n", follower_id);
            // Set turn for next follower or leader
            if (follower_id == M[0])
            {             // If last follower
                M[2] = 0; // Give turn back to leader
            }
            else
            {
                M[2] = -(follower_id + 1); // Signal next follower to terminate
            }
            break;
        }

        // Generate random number and write to shared memory
        M[3 + follower_id] = (rand() ^ getpid()) % 9 + 1;

        // Set turn for next process
        if (follower_id == M[0])
        {             // If last follower
            M[2] = 0; // Give turn back to leader
        }
        else
        {
            M[2] = follower_id + 1; // Give turn to next follower
        }
    }
}