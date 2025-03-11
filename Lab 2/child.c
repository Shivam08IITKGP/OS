#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int my_index;
pid_t *pids;
int my_status;
int n;
pid_t parent_pid;

void signal_handler(int sig);

int main()
{
    sleep(1);
    pid_t my_pid = getpid();
    srand(time(NULL) ^ my_pid);
    parent_pid = getppid();
    FILE *file = fopen("childpid.txt", "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d", &n) != 1)
    {
        perror("Error reading child count");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    pids = malloc(n * sizeof(pid_t));
    for (int i = 0; i < n; i++)
    {
        if (fscanf(file, "%d", &pids[i]) != 1)
        {
            perror("Error reading child PID");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);

    for (int i = 0; i < n; i++)
    {
        if (pids[i] == my_pid)
        {
            my_index = i;
            break;
        }
    }

    my_status = 3;

    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGINT, signal_handler);

    while (1)
    {
        pause();
    }

    return 0;
}
void signal_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        if (my_index == 0)
        {
            printf("| ");
        }
        if (my_status == 1)
        {
            printf("CATCH ");
            my_status = 3;
        }
        else if (my_status == 2)
        {
            printf("MISS  ");
            my_status = 4;
        }
        else if (my_status == 3)
        {
            printf("..... ");
        }
        else if (my_status == 4)
        {
            printf("      ");
        }
        fflush(stdout);

        if (my_index != n - 1)
        {
            kill(pids[my_index + 1], SIGUSR1);
        }
        else
        {
            printf(" |\n");
            fflush(stdout);
            FILE *file = fopen("dummycpid.txt", "r");
            if (file == NULL)
            {
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }
            pid_t dummy_pid;

            fscanf(file, "%d", &dummy_pid);
            kill(dummy_pid, SIGINT);
        }
    }
    else if (sig == SIGUSR2)
    {
        fflush(stdout);
        int caught = rand() % 5;

        if (caught > 0 && my_status == 3)
        {
            my_status = 1;
            kill(parent_pid, SIGUSR1);
        }

        else if (my_status == 3)
        {
            my_status = 2;
            kill(parent_pid, SIGUSR2);
        }
    }

    else
    {
        free(pids);
        exit(EXIT_SUCCESS);
    }
}