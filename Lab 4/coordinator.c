/*
22CS10072 Shivam Choudhury
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include "boardgen.c" // External file for board generation logic

#define BLOCK_COUNT 9 // Total number of blocks in the game
#define BLOCK_SIZE 3  // Size of each block (3x3)

int std_copy;                  // Stores a copy of the original stdout
int pipefd[BLOCK_COUNT][2];    // Pipes for inter-process communication
pid_t child_pids[BLOCK_COUNT]; // Stores process IDs of child processes
int temp_pipe[2];
int board[BLOCK_COUNT][BLOCK_COUNT];
int solution[BLOCK_COUNT][BLOCK_COUNT];

// Function to determine the neighboring blocks
void get_neighbours(int i, int *n1, int *n2, int *n3, int *n4)
{
    int block_row = i / 3;
    int block_col = i % 3;

    if (block_row == 0)
    {
        *n3 = i + 3;
        *n4 = i + 6;
    }
    else if (block_row == 1)
    {
        *n3 = i - 3;
        *n4 = i + 3;
    }
    else
    {
        *n3 = i - 3;
        *n4 = i - 6;
    }

    if (block_col == 0)
    {
        *n1 = i + 1;
        *n2 = i + 2;
    }
    else if (block_col == 1)
    {
        *n1 = i - 1;
        *n2 = i + 1;
    }
    else
    {
        *n1 = i - 1;
        *n2 = i - 2;
    }
}

// Function to display the help menu with commands and board layout
void print_help()
{
    printf("Commands supported:\n");
    printf("\tn         Start new game\n");
    printf("\tp b c d   Put digit d [1-9] at cell c [0-8] of block b [0-8]\n");
    printf("\ts         Show solution\n");
    printf("\th         Print this help message\n");
    printf("\tq         Quit\n\n");

    printf("Numbering scheme for block and cells:\n");
    printf("\t+---+---+---+\n");
    printf("\t| 0 | 1 | 2 |\n");
    printf("\t+---+---+---+\n");
    printf("\t| 3 | 4 | 5 |\n");
    printf("\t+---+---+---+\n");
    printf("\t| 6 | 7 | 8 |\n");
    printf("\t+---+---+---+\n\n");
}

// Function to create pipes for inter-process communication
void create_pipes()
{
    temp_pipe[0] = 0;
    temp_pipe[1] = 0;
    for (int i = 0; i < BLOCK_COUNT; i++)
    {
        if (pipe(temp_pipe) < 0)
        {
            perror("pipe creation failed");
            exit(1);
        }
        pipefd[i][0] = temp_pipe[0]; // Read end
        pipefd[i][1] = temp_pipe[1]; // Write end
    }
}

// Function to create child processes for each block
void fork_child_processes()
{
    for (int block = 0; block < BLOCK_COUNT; block++)
    {
        pid_t pid = fork(); // Create child process

        if (pid < 0) // Error handling
        {
            perror("fork failed");
            exit(1);
        }

        if (pid == 0) // Child process
        {
            // Find row and column neighbors
            int row_n1, row_n2, col_n1, col_n2;
            get_neighbours(block, &row_n1, &row_n2, &col_n1, &col_n2);
            // Set up window positioning for xterm display
            int x_pos = 200 + (block % 3) * 200;
            int y_pos = 100 + (block / 3) * 200;
            char geometry[32];
            sprintf(geometry, "17x8+%d+%d", x_pos, y_pos);

            char read_fd[32], write_fd[32], rn1_fd[32], rn2_fd[32], cn1_fd[32], cn2_fd[32];
            sprintf(read_fd, "%d", pipefd[block][0]);
            sprintf(write_fd, "%d", pipefd[block][1]);
            sprintf(rn1_fd, "%d", pipefd[row_n1][1]);
            sprintf(rn2_fd, "%d", pipefd[row_n2][1]);
            sprintf(cn1_fd, "%d", pipefd[col_n1][1]);
            sprintf(cn2_fd, "%d", pipefd[col_n2][1]);

            char title[32];
            sprintf(title, "%d", block);

            // Launch child process with the required arguments
            execlp("xterm", "xterm",
                   "-T", title,
                   "-fa", "Monospace",
                   "-fs", "10",
                   "-geometry", geometry,
                   "-bg", "#331100",
                   "-e", "./block",
                   read_fd, write_fd, rn1_fd, rn2_fd, cn1_fd, cn2_fd,
                   NULL);

            perror("execlp failed");
            exit(1);
        }

        child_pids[block] = pid; // Store child process ID
    }
}

int main()
{
    char command;

    create_pipes();         // Create pipes
    fork_child_processes(); // Start child processes
    print_help();           // Show game instructions
    std_copy = dup(1);      // Backup stdout

    while (1)
    {
        printf("Foodoku> ");
        if (scanf(" %c", &command) != 1)
            continue;

        switch (command)
        {
        case 'h': // Help command
            print_help();
            break;

        case 'n': // Start new game

            newboard(board, solution);

            for (int block = 0; block < BLOCK_COUNT; block++)
            {
                close(1);
                dup(pipefd[block][1]);
                printf("n");

                int block_row = (block / 3) * 3;
                int block_col = (block % 3) * 3;

                for (int i = 0; i < BLOCK_SIZE; i++)
                {
                    for (int j = 0; j < BLOCK_SIZE; j++)
                    {
                        printf(" %d", board[block_row + i][block_col + j]);
                    }
                }
                printf("\n");
                close(1);
                dup(std_copy);
            }
            break;

        case 'p': // Place a number in a cell

            int b, c, d;
            if (scanf("%d %d %d", &b, &c, &d) != 3)
            {
                printf("Invalid input format\n");
                break;
            }

            if (b < 0 || b >= 9 || c < 0 || c >= 9 || d < 1 || d > 9)
            {
                printf("Invalid input ranges\n");
                break;
            }

            close(1);
            dup(pipefd[b][1]);
            printf("p %d %d\n", c, d);
            close(1);
            dup(std_copy);
            break;

        case 's': // Show solution
            for (int block = 0; block < BLOCK_COUNT; block++)
            {
                close(1);
                dup(pipefd[block][1]);
                printf("n");

                int block_row = (block / 3) * 3;
                int block_col = (block % 3) * 3;

                for (int i = 0; i < BLOCK_SIZE; i++)
                {
                    for (int j = 0; j < BLOCK_SIZE; j++)
                    {
                        printf(" %d", solution[block_row + i][block_col + j]);
                    }
                }
                printf("\n");
                close(1);
                dup(std_copy);
            }
            break;

        case 'q': // Quit game
            for (int i = 0; i < BLOCK_COUNT; i++)
            {
                close(1);
                dup(pipefd[i][1]);
                printf("q\n");
                close(1);
                dup(std_copy);
            }
            for (int i = 0; i < BLOCK_COUNT; i++)
            {
                waitpid(child_pids[i], NULL, 0);
            }
            printf("Game Over\n");
            exit(0);
            break;

        default:
            printf("Invalid command. Type 'h' for help.\n");
        }
    }

    return 0;
}
