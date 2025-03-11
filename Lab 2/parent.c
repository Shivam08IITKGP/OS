#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int current_child_index;
int remaining_children;
void sigusr_handler(int sig);
int *child_status;
// Status of each child processes
// 1 -> CATCH
// 2 -> MISS
// 3 -> PLAYING
// 4 -> Out of the game

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <number_of_children>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    if (n <= 0)
    {
        fprintf(stderr, "Number of children must be a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen("childpid.txt", "w");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int *child_pids = malloc(n * sizeof(pid_t));
    if (child_pids == NULL)
    {
        perror("Memory allocation failed");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%d\n", n);
    fflush(file);

    for (int i = 0; i < n; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("Fork failed");
            fclose(file);
            free(child_pids);
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            execl("./child", "./child", NULL);
            perror("Exec failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            child_pids[i] = pid;
            fprintf(file, "%d\n", pid);
            fflush(file);
        }
    }
    printf("Parent: %d child processes created\n", n);

    fclose(file);

    printf("Parent: Waiting for child processes to read child database\n");
    sleep(2);

    child_status = malloc(n * sizeof(int));

    for (int i = 0; i < n; i++)
    {
        child_status[i] = 3;
    }

    current_child_index = 0;
    remaining_children = n;

    signal(SIGUSR1, sigusr_handler);
    signal(SIGUSR2, sigusr_handler);

    printf("    ");
    for (int i = 1; i <= n; i++)
    {
        printf("%d     ", i);
    }
    printf("\n");
    printf("+");
    for (int j = 0; j < 6 * n + 2; j++)
    {
        printf("-");
    }
    printf("+");

    printf("\n");

    while (remaining_children != 1)
    {
        kill(child_pids[current_child_index], SIGUSR2);
        pid_t dummy_pid = fork();
        if (dummy_pid > 0)
        {
            FILE *file = fopen("dummycpid.txt", "w");

            if (file == NULL)
            {
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }

            fprintf(file, "%d\n", dummy_pid);
            fflush(file);
            fclose(file);

            kill(child_pids[0], SIGUSR1);

            waitpid(dummy_pid, NULL, 0);

            if (child_status[current_child_index] == 1)
            {
                child_status[current_child_index] = 3;
            }
            else if (child_status[current_child_index] == 2)
            {
                child_status[current_child_index] = 4;
            }

            for (int i = 1; i < n; i++)
            {
                if (i + current_child_index == n)
                {
                    printf("+");
                    for (int j = 0; j < 6 * n + 2; j++)
                    {
                        printf("-");
                    }
                    printf("+");
                    printf("\n");
                }
                if (child_status[(i + current_child_index) % n] == 3)
                {

                    current_child_index = (i + current_child_index) % n;
                    break;
                }
            }
        }
        else
        {
            execl("./dummy", "./dummy", NULL);
        }
    }

    printf("+");
    for (int j = 0; j < 6 * n + 2; j++)
    {
        printf("-");
    }
    printf("+");
    printf("\n");

    printf("    ");
    for (int i = 1; i <= n; i++)
    {
        printf("%d     ", i);
    }
    printf("\n");
    printf("+++ Child %d: Yay! I am the winner!\n", current_child_index + 1);

    for (int i = 0; i < n; i++)
    {
        kill(child_pids[i], SIGINT);
    }

    free(child_status);
    free(child_pids);

    return 0;
}

void sigusr_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        child_status[current_child_index] = 1;
    }
    else if (sig == SIGUSR2)
    {
        child_status[current_child_index] = 2;
        remaining_children--;
    }
}