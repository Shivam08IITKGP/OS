/*
22CS10072 Shivam Choudhury
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BLOCK_SIZE 3

int std_copy_out;
int current[BLOCK_SIZE][BLOCK_SIZE];
int original[BLOCK_SIZE][BLOCK_SIZE];

void draw_block()
{
    printf("\033[H\033[J");
    printf("+---+---+---+\n");

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        printf("│");
        for (int j = 0; j < BLOCK_SIZE; j++)
        {
            if (current[i][j] == 0)
                printf("   ");
            else
                printf(" %d ", current[i][j]);
            printf("│");
        }
        printf("\n");

        if (i < BLOCK_SIZE - 1)
            printf("+---+---+---+\n");
    }

    printf("+---+---+---+\n");
}

void print_error(const char *message)
{
    printf("%s\n", message);
    sleep(2); // Wait for 2 seconds
}

int check_row_conflict(int row, int digit)
{
    for (int j = 0; j < BLOCK_SIZE; j++)
    {
        if (current[row][j] == digit)
            return 1;
    }
    return 0;
}

int check_col_conflict(int col, int digit)
{
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        if (current[i][col] == digit)
            return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        fprintf(stderr, "Usage: %s read_fd write_fd rn1_fd rn2_fd cn1_fd cn2_fd\n", argv[0]);
        exit(1);
    }

    int read_fd = atoi(argv[1]);
    int write_fd = atoi(argv[2]);
    int rn1_fd = atoi(argv[3]);
    int rn2_fd = atoi(argv[4]);
    int cn1_fd = atoi(argv[5]);
    int cn2_fd = atoi(argv[6]);

    // Redirect read_fd to stdin
    close(0);
    dup(read_fd);
    std_copy_out = dup(1);
    char command;

    while (1)
    {
        scanf(" %c", &command);

        if (command == 'n')
        {
            // Read new board state
            for (int i = 0; i < BLOCK_SIZE; i++)
            {
                for (int j = 0; j < BLOCK_SIZE; j++)
                {
                    scanf("%d", &original[i][j]);
                    current[i][j] = original[i][j];
                }
            }
            draw_block();
        }
        
        else if (command == 'p')
        {
            int cell, digit;
            scanf("%d %d", &cell, &digit);

            int row = cell / 3;
            int col = cell % 3;

            // Check if cell is read-only
            if (original[row][col] != 0)
            {
                print_error("Read-only cell!");
                draw_block();
                continue;
            }

            // Check block conflict
            int has_conflict = 0;
            for (int i = 0; i < BLOCK_SIZE; i++)
            {
                for (int j = 0; j < BLOCK_SIZE; j++)
                {
                    if (current[i][j] == digit)
                    {
                        has_conflict = 1;
                        i = BLOCK_SIZE;
                        break;
                    }
                }
            }

            if (has_conflict)
            {
                print_error("Block conflict!");
                draw_block();
                continue;
            }

            // Check row conflicts
            close(1);
            dup(rn1_fd);
            printf("r %d %d %d\n", row, digit, write_fd);
            close(1);
            dup(std_copy_out);
            usleep(1000);
            scanf("%d", &has_conflict);

            if (has_conflict)
            {
                print_error("Row conflict!");
                draw_block();
                continue;
            }

            close(1);
            dup(rn2_fd);
            printf("r %d %d %d\n", row, digit, write_fd);
            close(1);
            dup(std_copy_out);
            usleep(1000);
            scanf("%d", &has_conflict);

            if (has_conflict)
            {
                print_error("Row conflict!");
                draw_block();
                continue;
            }

            // Check column conflicts
            close(1);
            dup(cn1_fd);
            printf("c %d %d %d\n", col, digit, write_fd);
            close(1);
            dup(std_copy_out);
            usleep(1000);
            scanf("%d", &has_conflict);

            if (has_conflict)
            {
                print_error("Column conflict!");
                draw_block();
                continue;
            }

            // Check the second column conflict
            close(1);
            dup(cn2_fd);
            printf("c %d %d %d\n", col, digit, write_fd);
            close(1);
            dup(std_copy_out);
            usleep(1000);
            scanf("%d", &has_conflict);

            if (has_conflict)
            {
                print_error("Column conflict!");
                draw_block();
                continue;
            }

            current[row][col] = digit;
            draw_block();
        }

        else if (command == 'r')
        {
            int row, digit, parent_fd;

            scanf("%d %d %d", &row, &digit, &parent_fd);
            int has_conflict = check_row_conflict(row, digit);

            close(1);
            dup(parent_fd);

            printf("%d\n", has_conflict);

            // Restore stdout
            close(1);
            dup(std_copy_out);
        }

        else if (command == 'c')
        {
            int col, digit, parent_fd;

            scanf("%d %d %d", &col, &digit, &parent_fd);
            int has_conflict = check_col_conflict(col, digit);

            close(1);
            dup(parent_fd);

            printf("%d\n", has_conflict);

            // Restore stdout
            close(1);
            dup(std_copy_out);
        }

        else if (command == 'q')
        {
            print_error("Bye...");
            break;
        }

        else
        {
            print_error("Invalid command!");
            draw_block();
        }
    }
    return 0;
}