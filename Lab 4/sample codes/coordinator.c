#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define NUM_BLOCKS 9

int pipes[NUM_BLOCKS][2];     // Pipes for communication with child processes
pid_t child_pids[NUM_BLOCKS]; // Track child PIDs
int A[9][9], S[9][9];
int std_copy;

void print_help_message()
{
    printf("\nFoodoku Commands:\n");
    printf("\tn - Start new game\n");
    printf("\tp b c d - Pur digit d [1-9] at cell c [0-8] of block b [0-8]\n");
    printf("\ts - Show solution\n");
    printf("\th - Print this help message\n");
    printf("\tq - Quit game\n\n");
    printf("Numbering scheme for blocks and cells\n");
    printf("\t+---+---+---+\n");
    printf("\t| 0 | 1 | 2 |\n");
    printf("\t+---+---+---+\n");
    printf("\t| 3 | 4 | 5 |\n");
    printf("\t+---+---+---+\n");
    printf("\t| 6 | 7 | 8 |\n");
    printf("\t+---+---+---+\n\n");

}

void launch_blocks()
{
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            // Child process
            char block_index_str[3], read_fd_str[3];
            sprintf(block_index_str, "%d", i);
            sprintf(read_fd_str, "%d", pipes[i][0]);

            // Find neighbor indices
            int neighbour_rows[2], neighbour_cols[2];
            int block_row = i / 3;
            int block_col = i % 3;

            if (block_row == 0)
            {
                neighbour_cols[0] = i + 3;
                neighbour_cols[1] = i + 6;
            }
            else if (block_row == 1)
            {
                neighbour_cols[0] = i - 3;
                neighbour_cols[1] = i + 3;
            }
            else
            {
                neighbour_cols[0] = i - 3;
                neighbour_cols[1] = i - 6;
            }

            if (block_col == 0)
            {
                neighbour_rows[0] = i + 1;
                neighbour_rows[1] = i + 2;
            }
            else if (block_col == 1)
            {
                neighbour_rows[0] = i - 1;
                neighbour_rows[1] = i + 1;
            }
            else
            {
                neighbour_rows[0] = i - 1;
                neighbour_rows[1] = i - 2;
            }

            char rn1_str[3], rn2_str[3], cn1_str[3], cn2_str[3];
            sprintf(rn1_str, "%d", neighbour_rows[0]);
            sprintf(rn2_str, "%d", neighbour_rows[1]);
            sprintf(cn1_str, "%d", neighbour_cols[0]);
            sprintf(cn2_str, "%d", neighbour_cols[1]);

            execlp("xterm", "xterm", "-T", "Block Window", "-e", "./block",
                   block_index_str, read_fd_str, rn1_str, rn2_str, cn1_str, cn2_str, NULL);

            perror("execlp failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            // Parent process
            child_pids[i] = pid; // Store child PID
            close(pipes[i][0]);  // Close read end in parent
        }
    }
}

void send_command_to_block(int block, const char *cmd)
{
    write(pipes[block][1], cmd, strlen(cmd));
    write(pipes[block][1], "\n", 1); // Ensure newline
}

void process_input()
{
    char command[256];
    while (1)
    {
        printf("Enter command: ");
        if (fgets(command, sizeof(command), stdin) == NULL)
            continue;

        if (command[0] == 'q')
        {
            for (int i = 0; i < NUM_BLOCKS; i++)
            {
                send_command_to_block(i, "q");
            }
            break;
        }
        else if (command[0] == 'p')
        {
            int b, c, d;
            if (sscanf(command + 1, "%d %d %d", &b, &c, &d) == 3)
            {
                send_command_to_block(b, command);
            }
            else
            {
                printf("Invalid input\n");
            }
        }
        else if (command[0] == 'n')
        {
            newboard(A, S); // Generate new puzzle

            for (int i = 0; i < NUM_BLOCKS; i++)
            {
                char msg[50];

                // Extract 3×3 block from the 9×9 grid
                int block_row = (i / 3) * 3; // Row start index
                int block_col = (i % 3) * 3; // Column start index

                // Format the message with the 9 numbers for the block
                sprintf(msg, "n %d %d %d %d %d %d %d %d %d",
                        A[block_row][block_col], A[block_row][block_col + 1], A[block_row][block_col + 2],
                        A[block_row + 1][block_col], A[block_row + 1][block_col + 1], A[block_row + 1][block_col + 2],
                        A[block_row + 2][block_col], A[block_row + 2][block_col + 1], A[block_row + 2][block_col + 2]);

                send_command_to_block(i, msg);
            }
        }
        else if (command[0] == 's')
        {
            for (int i = 0; i < NUM_BLOCKS; i++)
            {
                char msg[50];

                // Extract 3×3 block from the 9×9 grid
                int block_row = (i / 3) * 3; // Row start index
                int block_col = (i % 3) * 3; // Column start index

                // Format the message with the 9 numbers for the block
                sprintf(msg, "s %d %d %d %d %d %d %d %d %d",
                        S[block_row][block_col], S[block_row][block_col + 1], S[block_row][block_col + 2],
                        S[block_row + 1][block_col], S[block_row + 1][block_col + 1], S[block_row + 1][block_col + 2],
                        S[block_row + 2][block_col], S[block_row + 2][block_col + 1], S[block_row + 2][block_col + 2]);

                send_command_to_block(i, msg);                
            }
        }
        else
        {
            printf("Invalid command\n");
        }
    }
}

int main()
{
    print_help_message();

    std_copy = dup(1); // Save stdout
    launch_blocks();
    process_input();

    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        close(pipes[i][1]);              // Close write end
        waitpid(child_pids[i], NULL, 0); // Wait for each child
    }

    return 0;
}
