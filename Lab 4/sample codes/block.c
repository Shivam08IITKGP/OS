#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SIZE 3

int std_copy;
int block[SIZE][SIZE];
int block_number;
int read_fd, write_fd;
int rn[2], rc[2];

void draw()
{
    printf("\033[H\033[J");
    printf("+---+---+---+\n");
    for (int i = 0; i < SIZE; i++)
    {
        printf("│");
        for (int j = 0; j < SIZE; j++)
        {
            if (block[i][j] == 0)
                printf("   ");
            else
                printf(" %d ", block[i][j]);
            printf("│");
        }
        printf("\n");
        if (i < SIZE - 1)
            printf("+---+---+---+\n");
    }
    printf("└───┴───┴───┘\n");
    fflush(stdout);
}

void process_command(char *command)
{
    if (command[0] == 'q')
    {
        printf("Block exiting...\n");
        exit(0);
    }
    else if (command[0] == 'r')
    {
        int r, d;
        sscanf("%d %d", &r, &d);
    }
    else if (command[0] == 'p')
    {
        int c, d;
        sscanf("%d %d", &c, &d);
        int row = c / SIZE;
        int col = c % SIZE;
        if (block[row][col] != 0)
        {
            printf("Read-only cell\n");
            draw();
            return;
        }
        else
        {
            for (int i = 0; i < SIZE; i++)
            {
                for (int j = 0; j < SIZE; j++)
                {
                    if (block[i][j] == d)
                    {
                        printf("Block conflict\n");
                        draw();
                        return;
                    }
                }
            }

            int response = 0;

            close(1);
            dup(rn[0]);
            printf("r %d %d\n", row, d);
            fflush(stdout);
            close(1);
            dup(std_copy);
            scanf("%d", &response);

            if (response != 0)
            {
                printf("Row conflict\n");
                draw();
                return;
            }

            close(1);
            dup(rn[1]);
            printf("r %d %d\n", row, d);
            fflush(stdout);
            close(1);
            dup(std_copy);
            scanf("%d", &response);

            if (response != 0)
            {
                printf("Row conflict\n");
                draw();
                return;
            }

            close(1);
            dup(rc[0]);
            printf("c %d %d\n", col, d);
            fflush(stdout);
            close(1);
            dup(std_copy);
            scanf("%d", &response);

            if (response != 0)
            {
                printf("Column conflict\n");
                draw();
                return;
            }

            close(1);
            dup(rc[1]);
            printf("c %d %d\n", col, d);
            fflush(stdout);
            close(1);
            dup(std_copy);
            scanf("%d", &response);

            if (response != 0)
            {
                printf("Column conflict\n");
                draw();
                return;
            }

            block[row][col] = d;
        }
    }
    else if (command[0] == 'n')
    {
        int b[SIZE * SIZE];
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                sscanf("%d", &b[i * SIZE + j]);
                block[i][j] = b[i * SIZE + j];
            }
        }
    }
    else if (command[0] == 'c')
    {
        int c, d;
        sscanf("%d %d", &c, &d);
        for (int i = 0; i < SIZE; i++)
        {
            if (block[i][c] == d)
            {
                printf("1\n");
                fflush(stdout);
                draw();
                return;
            }
        }
        printf("0\n");
        fflush(stdout);
    }
    else if(command[0] == 'r')
    {
        int r, d;
        sscanf("%d %d", &r, &d);
        for (int i = 0; i < SIZE; i++)
        {
            if (block[r][i] == d)
            {
                printf("1\n");
                fflush(stdout);
                draw();
                return;
            }
        }
        printf("0\n");
        fflush(stdout);
    }
    draw();
}

int main(int argc, char *argv[])
{
    if (argc != 8)
    {
        fprintf(stderr, "Usage: %s block_num read_fd write_fd row_n1_fd row_n2_fd col_n1_fd col_n2_fd\n", argv[0]);
        exit(1);
    }

    block_number = atoi(argv[1]);
    read_fd = atoi(argv[2]);
    write_fd = atoi(argv[3]);
    rn[0] = atoi(argv[3]);
    rn[1] = atoi(argv[4]);
    rc[0] = atoi(argv[5]);
    rc[1] = atoi(argv[6]);

    std_copy = dup(1);

    close(0);
    dup(read_fd);
    close(read_fd);

    // Initialize block to 0
    memset(block, 0, sizeof(block));

    char command[256];
    while (fgets(command, sizeof(command), stdin))
    {
        // Trim newlines
        command[strcspn(command, "\n")] = 0;
        process_command(command);
    }

    return 0;
}
